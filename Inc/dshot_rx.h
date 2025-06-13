/*
 * dshot_rx.h
 *
 *  Created on: May 20, 2025
 *      Author: Jimmy1_Hua
 */

#ifndef INC_DSHOT_RX_H_
#define INC_DSHOT_RX_H_


#include <stdint.h>
#include <stdbool.h>
#include "stm32g4xx_hal.h"  // for UART_HandleTypeDef, TIM_HandleTypeDef


#include "dshot.h"

#define Dshot_TIM             (&htim2)
#define Dshot_TIM_Rise_CH     TIM_CHANNEL_1
#define Dshot_TIM_Fail_CH     TIM_CHANNEL_2
#define RX_EDGE_MAX   50   /* 50 筆足夠觀察一個 16-bit frame */

extern UART_HandleTypeDef huart2;

extern TIM_HandleTypeDef htim2;

extern volatile uint16_t g_throttle;

void dshot_rx_init(void);
bool dshot_rx_available(void);
int dshot_rx_read(uint16_t *packet_out);
void dshot_rx_service(UART_HandleTypeDef *huart);
int dshot_rx_get_throttle(uint16_t *throttle_out);
#endif /* INC_DSHOT_RX_H_ */
