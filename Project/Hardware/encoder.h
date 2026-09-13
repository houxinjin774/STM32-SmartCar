/**
  ******************************************************************************
  * @file    encoder.h
  * @brief   编码器测速模块 x4 —— 测量轮子转多快、走了多远
  ******************************************************************************
  */
#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

/* 4个编码器的累计脉冲数（中断里+1）。0左前 1右前 2左后 3右后 */
extern volatile uint32_t g_encTotal[4];

void     Encoder_Init(void);
void     Encoder_Task(void);              /* 每100ms调用一次，刷新速度 */
uint32_t Encoder_GetTotal(uint8_t idx);   /* 第idx个编码器累计脉冲数 0左前 1右前 2左后 3右后 */
int32_t  Encoder_GetSpeedPps(uint8_t side); /* 左(0)/右(1)侧速度：脉冲数/秒 */
int32_t  Encoder_GetDistanceMm(void);     /* 总里程，单位毫米 */

#endif /* __ENCODER_H */
