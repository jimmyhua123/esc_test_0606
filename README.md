# DShot 接收實作說明（適用 STM32G431）

本專案使用 stm32 motorControl workbench_6.3.2 生成的 code 加上我寫的 `dshot_rx.c` 用於接收 DShot 數位訊號（例如來自 STM32F103 發送），搭配 TIM2 + DMA 雙通道捕捉輸入寬度，實現解碼後控制 BLDC 馬達轉速。

## 檔案結構

- `dshot.c` / `dshot.h`：負責 DShot 指令封包生成與發送（Tx）
- `dshot_rx.c` / `dshot_rx.h`：實作 DShot 訊號解析（Rx），使用 TIM2_CH1（上升緣）+ TIM2_CH2（下降緣）捕捉寬度 (PA15)
- `main.c`：呼叫 `dshot_rx_service()` 解析封包，或使用簡易 API 取得油門值

## 新增函數說明

### `int dshot_rx_get_throttle(uint16_t *throttle_out);`

提供簡單 API，讓主程式直接取得解析後的 DShot 油門值（11-bit）

#### 傳入參數

- `uint16_t *throttle_out`：指向用來儲存輸出油門的變數指標

#### 回傳值

- `0`：成功，`*throttle_out` 中為 0~2047 油門值
- `-1`：失敗（可能是尚未接收到封包或 CRC 驗證失敗）

#### 使用範例（建議放在主迴圈中）

```c
uint16_t val;
if (dshot_rx_get_throttle(&val) == 0) {
    g_throttle = val;  // 成功取得解析後油門
}

```

### Code

```
DMA_HandleTypeDef hdma_tim2_ch2;
void DMA1_Channel4_IRQHandler(void) //for dma tim2_ch2
{
    HAL_DMA_IRQHandler(&hdma_tim2_ch2);
}
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

in MX_TIM2_Init

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	hdma_tim2_ch1.Init.Request  = DMA_REQUEST_TIM2_CH1;  //g4xx 使用DMAMUX 必須加上這行
	hdma_tim2_ch2.Init.Request  = DMA_REQUEST_TIM2_CH2;


  /* 把tim2ch2掛上dma1ch4 */
  hdma_tim2_ch2.Instance                 = DMA1_Channel4;
  hdma_tim2_ch2.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_tim2_ch2.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim2_ch2.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim2_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim2_ch2.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  hdma_tim2_ch2.Init.Mode                = DMA_NORMAL;//!
  hdma_tim2_ch2.Init.Priority            = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma_tim2_ch2) != HAL_OK)
      Error_Handler();
  /* 將 Channel4 掛到 TIM2 的 CC2  */
  __HAL_LINKDMA(&htim2, hdma[TIM_DMA_ID_CC2], hdma_tim2_ch2);


  // === TIM2_CH1 對應 DMA1_Channel3 (Rise Edge 用) ===
  hdma_tim2_ch1.Instance                 = DMA1_Channel3;
  hdma_tim2_ch1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_tim2_ch1.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim2_ch1.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim2_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim2_ch1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
  hdma_tim2_ch1.Init.Mode                = DMA_NORMAL;
  hdma_tim2_ch1.Init.Priority            = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma_tim2_ch1) != HAL_OK)
      Error_Handler();

  // 連結 TIM2_CH1 → DMA1_Channel3
  __HAL_LINKDMA(&htim2, hdma[TIM_DMA_ID_CC1], hdma_tim2_ch1);
```
