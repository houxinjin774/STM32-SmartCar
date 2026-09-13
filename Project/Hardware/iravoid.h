/**
  ******************************************************************************
  * @file    iravoid.h
  * @brief   红外避障模块 x3（水平探头）+ 避障行驶逻辑
  ******************************************************************************
  */
#ifndef __IRAVOID_H
#define __IRAVOID_H

#include "stm32f10x.h"

void IrAvoid_Init(void);
void Ir_Read(uint8_t *l, uint8_t *m, uint8_t *r);  /* 读3个探头，1=有障碍 */
void Avoid_Task(void);                              /* 每10ms调用一次，避障 */

#endif /* __IRAVOID_H */
