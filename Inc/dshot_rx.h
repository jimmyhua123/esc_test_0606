/*
 * dshot_rx.h
 *
 *  Created on: May 20, 2025
 *      Author: Jimmy1_Hua
 */

#ifndef INC_DSHOT_RX_H_
#define INC_DSHOT_RX_H_

#include "tim.h"
#include <stdint.h>
#include <stdbool.h>
#include "dshot.h"

#define Dshot_TIM             (&htim2)
#define Dshot_TIM_Rise_CH     TIM_CHANNEL_1
#define Dshot_TIM_Fail_CH     TIM_CHANNEL_2
#define RX_EDGE_MAX   50   /* 50 筆足夠觀察一個 16-bit frame */

void dshot_rx_init(void);
bool dshot_rx_available(void);
uint16_t *dshot_rx_get_edges(uint16_t *len);
int dshot_rx_read(uint16_t *packet_out);
void dshot_rx_service(UART_HandleTypeDef *huart);

#endif /* INC_DSHOT_RX_H_ */
