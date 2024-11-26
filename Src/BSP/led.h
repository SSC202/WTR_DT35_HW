/**
 * @file    LED 相关函数
 */
#ifndef __LED_H
#define __LED_h

#include "stm32f1xx.h"

// LED 相关定义

// 串口转接板状态指示脚
#define STATE_LEDA_GPIO_PIN  GPIO_PIN_13
#define STATE_LEDA_GPIO_PORT GPIOB

#define STATE_LEDA_TOGGLE()                                            \
    do {                                                               \
        HAL_GPIO_TogglePin(STATE_LEDA_GPIO_PORT, STATE_LEDA_GPIO_PIN); \
    } while (0)

// 串口转接板初始化指示脚
#define STATE_LEDB_GPIO_PIN  GPIO_PIN_12
#define STATE_LEDB_GPIO_PORT GPIOB

#define STATE_LEDB_TOGGLE()                                            \
    do {                                                               \
        HAL_GPIO_TogglePin(STATE_LEDB_GPIO_PORT, STATE_LEDB_GPIO_PIN); \
    } while (0)
#define STATE_LEDB_OFF()                                                              \
    do {                                                                              \
        HAL_GPIO_WritePin(STATE_LEDB_GPIO_PORT, STATE_LEDB_GPIO_PIN, GPIO_PIN_RESET); \
    } while (0)

extern int state;

#endif