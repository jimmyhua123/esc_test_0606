#include "dshot_rx.h"
//#include "tim.h"
//#include "usart.h"
#include "dshot.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>


/* ---------- 參數 ---------- */
#define RX_BITS     16 //
#define RX_EDGES     (RX_BITS+1) /* 17 */
static volatile uint16_t rx_period[RX_EDGES];   /* CCR1 */
static volatile uint16_t rx_duty  [RX_BITS];   /* CCR2 */
static volatile bool     frame_ready = false;

/* ---------- DMA 完成 ---------- */
static void dshot_rx_dma_complete(DMA_HandleTypeDef *hdma)
{

    /* 停兩路 DMA，避免覆寫 */
    __HAL_TIM_DISABLE_DMA(Dshot_TIM, TIM_DMA_CC1 | TIM_DMA_CC2);
    HAL_TIM_IC_Stop_DMA(Dshot_TIM, Dshot_TIM_Rise_CH);
    HAL_TIM_IC_Stop_DMA(Dshot_TIM, Dshot_TIM_Fail_CH);
    frame_ready = true;

}

/* ---------- 初始化 ---------- */
void dshot_rx_init(void)
{
    frame_ready = false;

    /* 1. 同時開 CH1 / CH2 兩條 DMA */
    HAL_TIM_IC_Start_DMA(Dshot_TIM, Dshot_TIM_Rise_CH,          /* CCR1→period */
                         (uint32_t*)rx_period, RX_EDGES);
    HAL_TIM_IC_Start_DMA(Dshot_TIM, Dshot_TIM_Fail_CH,          /* CCR2→duty   */
                         (uint32_t*)rx_duty,   RX_BITS);

    /* 2. 再掛 callback（**順序非常重要**） */
    htim2.hdma[TIM_DMA_ID_CC1]->XferCpltCallback = dshot_rx_dma_complete;
    htim2.hdma[TIM_DMA_ID_CC2]->XferCpltCallback = NULL;   // or 不設定

    __HAL_TIM_ENABLE_DMA(Dshot_TIM, TIM_DMA_CC1 | TIM_DMA_CC2);
}

/* ---------- 狀態 ---------- */
bool dshot_rx_available(void)
{

    return frame_ready;
}

/* ---------- 取封包並驗 CRC ---------- */
int dshot_rx_read(uint16_t *packet_out)
{
    if (!frame_ready) return -1;


    uint32_t sum = 0;
    for (int i = 1; i < RX_BITS; ++i)   // 只加 index 1-15，共 15 筆，不含 gap
        sum += rx_period[i];
    uint32_t avg = sum / (RX_BITS - 1); // 平均 bit 週期


    uint16_t pkt = 0;
    for (int i = 0; i < RX_BITS; ++i)
    {
        uint16_t period = rx_period[i+1 ];
        uint16_t duty   = rx_duty  [i ];
        uint8_t  bit    = (duty > (avg  >> 1)) ? 1 : 0;
        pkt = (pkt << 1) | bit;
    }

    /* CRC */
    uint8_t crc = 0;
    uint16_t tmp = pkt >> 4;
    for (int i = 0; i < 3; ++i) { crc ^= tmp & 0xF; tmp >>= 4; }
    crc &= 0xF;

    /* 重新開 DMA 接下一包 */
    frame_ready = false;
    HAL_TIM_IC_Start_DMA(Dshot_TIM, Dshot_TIM_Rise_CH,
                         (uint32_t*)rx_period, RX_EDGES);
    HAL_TIM_IC_Start_DMA(Dshot_TIM, Dshot_TIM_Fail_CH,
                         (uint32_t*)rx_duty,   RX_BITS);
    __HAL_TIM_ENABLE_DMA(Dshot_TIM, TIM_DMA_CC1 | TIM_DMA_CC2);

    Dshot_TIM->hdma[TIM_DMA_ID_CC1]->XferCpltCallback = dshot_rx_dma_complete;

    if (crc != (pkt & 0xF)) return -1;  /* CRC 錯 */

    *packet_out = pkt;
    return 0;
}

/* ---------------------------------------------------------------
 * 新增：對外服務函式
 * 作用：每次被呼叫就檢查是否有新封包，
 *       若有且 CRC 正確 → 先列印原始 16-bit，再列印解碼後的 TH/TEL
 *       若無封包 / CRC 錯 → 什麼都不做
 * ------------------------------------------------------------- */
void dshot_rx_service(UART_HandleTypeDef *huart)
{
//    char debug_msg[] = ">>> enter dshot_rx_service\n";
//    HAL_UART_Transmit(huart, (uint8_t*)debug_msg, sizeof(debug_msg)-1, HAL_MAX_DELAY);
    if (!dshot_rx_available()) {
//        char no_frame[] = "in dshot_rx_service No frame ready  \n";
//        HAL_UART_Transmit(&huart2, (uint8_t*)no_frame, sizeof(no_frame)-1, HAL_MAX_DELAY);
//        HAL_Delay(1);
        return;
    }



    uint16_t pkt;
    if (dshot_rx_read(&pkt) != 0) return;        /* CRC 錯 */

    /* ---------- 計算平均週期 (tick 數) ---------- */


        uint32_t sum = 0;
        for (int i = 1; i < RX_BITS; ++i)
            sum += rx_period[i];
        uint32_t avg = sum / (RX_BITS - 1);   /* avg = 124 */

    /* ---------- 判斷 DSHOT 等級 ---------- */
    const char *rate;
    if      (avg > 180) rate = "150";
    else if (avg > 90)  rate = "300";
    else if (avg > 45)  rate = "600";
    else                rate = "1200";


    /* ---------- 產生 16-bit 二進位字串 ---------- */
    char bin[17];
    for (int i = 0; i < 16; ++i)
        bin[15 - i] = (pkt & (1u << i)) ? '1' : '0';
    bin[16] = '\0';

    /* ---------- 組字串並送出 ---------- */
    char buf[64];
    int  n;

    /* ① 原始封包 */
//    n = sprintf(buf, "PKT=   HEX: 0x%04X \nDEC:(%u) \nBIN:%s\r\n", pkt, pkt, bin);
//    n = sprintf(buf, "in dshot_rx_service PKT= BIN:%s\r\n",  bin);
//    HAL_UART_Transmit(huart, (uint8_t*)buf, n, HAL_MAX_DELAY);
//    HAL_Delay(1);


    /* ② 解碼結果 */
    uint16_t throttle = pkt >> 5;            /* 11-bit throttle */
    uint8_t  tel_flag = (pkt >> 4) & 0x1;

//    n = sprintf(buf, "in dshot_rx_service DSHOT%s TH=%u TEL=%u\r\n", rate,throttle, tel_flag);
//    HAL_UART_Transmit(huart, (uint8_t*)buf, n, HAL_MAX_DELAY);
//    HAL_Delay(1);

}

/* ---------------------------------------------------------------
 *  dshot_rx_get_throttle()
 *  作用：如果有新封包且 CRC 正確 → 解析 11-bit Throttle
 *        throttle_out 取值範圍 0-2047，成功回傳 0
 *        若無新封包 / CRC 錯 / 其他錯誤 → 回傳 -1
 * ------------------------------------------------------------- */
int dshot_rx_get_throttle(uint16_t *throttle_out)
{
    if (throttle_out == NULL) return -1;
    if (!dshot_rx_available()) return -1;      /* 還沒收滿一 frame */

    uint16_t pkt;
    if (dshot_rx_read(&pkt) != 0) return -1;   /* CRC 錯 */

    *throttle_out = pkt >> 5;                  /* 取 11-bit TH */
    return 0;
}
