/**
  ******************************************************************************
  * @file    bluetooth.h
  * @brief   HC-05 蓝牙串口 —— 手机遥控小车的"遥控器接收器"
  ******************************************************************************
  */
#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f10x.h"
#include <stdio.h>

void    Bluetooth_Init(uint32_t baud);   /* 初始化串口1 */
void    BT_RxPush(uint8_t d);            /* 供串口中断调用，存收到的字节 */
uint8_t BT_GetChar(char *c);             /* 取出手机发来的1个字符，没有返回0 */
uint32_t BT_GetLastRxMs(void);           /* 最后一次收到蓝牙数据的时刻 */
void    BT_PrintHelp(void);              /* 打印指令帮助 */

#endif /* __BLUETOOTH_H */
