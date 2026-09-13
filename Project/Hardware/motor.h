/**
  ******************************************************************************
  * @file    motor.h
  * @brief   TB6612 电机驱动 —— 控制4个轮子的方向和速度
  ******************************************************************************
  */
#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

void    Motor_Init(void);                        /* 初始化（上电调用一次） */
void    Motor_SetSpeed(int16_t left, int16_t right); /* 设置左右侧速度，-6800~+6800 */
void    Motor_Stop(void);                        /* 停车 */
int8_t  Motor_GetDir(uint8_t side);              /* 查询方向：0停 1前进 -1后退 */

#endif /* __MOTOR_H */
