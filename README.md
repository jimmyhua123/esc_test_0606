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

使用 stm32 motorControl workbench_6.3.2 生成的code ，加上我寫的dshot.c dshot_rx.c 使用pa15 的pwn c
```
