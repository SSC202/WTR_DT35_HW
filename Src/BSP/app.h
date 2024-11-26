#ifndef __APP_H
#define __APP_H

#include "stm32f1xx.h"

#include "usart.h"

extern int flag;                     // 上传任务标志位
extern uint8_t uart_send_buffer[24]; // 串口发送数据

void int32_to_4uint8(int32_t input_num, uint8_t output_num[]);
void uart_send(uint8_t channel1[], uint8_t channel2[], uint8_t channel3[], uint8_t channel4[]);

#endif