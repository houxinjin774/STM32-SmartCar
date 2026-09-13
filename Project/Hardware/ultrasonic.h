/**
  ******************************************************************************
  * @file    ultrasonic.h
  * @brief   HC-SR04 超声波测距
  ******************************************************************************
  */
#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f10x.h"

void     Ultra_Init(void);
uint16_t Ultra_GetDistanceCm(void);   /* 测一次距离(厘米)，测不到返回999 */
void     Ultra_EchoISR(void);         /* 供外部中断调用，不要自己调用 */

#endif /* __ULTRASONIC_H */
