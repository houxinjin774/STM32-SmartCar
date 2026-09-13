/**
  ******************************************************************************
  * @file    track.h
  * @brief   TCRT5000 循迹模块 x3 —— 沿着地上的黑线跑
  ******************************************************************************
  */
#ifndef __TRACK_H
#define __TRACK_H

#include "stm32f10x.h"

void Track_Init(void);
void Track_Read(uint8_t *l, uint8_t *m, uint8_t *r);  /* 读3个探头，1=踩在黑线上 */
void LineFollow_Task(void);                            /* 每10ms调用一次，循迹 */

#endif /* __TRACK_H */
