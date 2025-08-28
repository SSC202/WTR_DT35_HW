/**
 * @file    回调函数相关定义
 */
#include "tim.h"

#include "app.h"
#include "led.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        STATE_LEDA_TOGGLE();
        if (state == 1) {
            STATE_LEDB_TOGGLE();
        } else {
            STATE_LEDB_OFF();
        }
    } else if (htim->Instance == TIM2) {
        if (flag == 0) {
            flag = 1;
        }
    }
}