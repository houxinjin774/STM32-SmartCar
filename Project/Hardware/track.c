/**
  ******************************************************************************
  * @file    track.c
  * @brief   循迹逻辑（三路TCRT5000）
  *
  * 原理（小白版）：
  *   TCRT5000朝地面发射红外光：照到白纸反射回来=输出一种电平，
  *   照到黑线被吸收=输出另一种电平。这样就知道"这个探头下面是不是黑线"。
  *
  * 走线策略（最经典的"三路循迹"）：
  *   只有中间探头看到线  -> 车正对着线，直行
  *   左探头看到线        -> 线偏到左边了（车偏右），向左修正
  *   右探头看到线        -> 线偏到右边了（车偏左），向右修正
  *   全都看不到线        -> 丢线，停车（防跑飞）
  ******************************************************************************
  */
#include "track.h"
#include "config.h"
#include "motor.h"

void Track_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 配成上拉输入：兼容各种输出方式的模块 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

    gpio.GPIO_Pin = TRACK_L_PIN;
    GPIO_Init(TRACK_L_PORT, &gpio);
    gpio.GPIO_Pin = TRACK_M_PIN;
    GPIO_Init(TRACK_M_PORT, &gpio);
    gpio.GPIO_Pin = TRACK_R_PIN;
    GPIO_Init(TRACK_R_PORT, &gpio);
}

/* 读3个探头。返回值1表示"下面是黑线" */
void Track_Read(uint8_t *l, uint8_t *m, uint8_t *r)
{
    *l = (GPIO_ReadInputDataBit(TRACK_L_PORT, TRACK_L_PIN) == LINE_LEVEL) ? 1 : 0;
    *m = (GPIO_ReadInputDataBit(TRACK_M_PORT, TRACK_M_PIN) == LINE_LEVEL) ? 1 : 0;
    *r = (GPIO_ReadInputDataBit(TRACK_R_PORT, TRACK_R_PIN) == LINE_LEVEL) ? 1 : 0;
}

/* 每10ms调用一次 */
void LineFollow_Task(void)
{
    uint8_t l, m, r;
    uint8_t lost = 0;
    static uint8_t  lastCmd = 'F';   /* 最后一次的行进方向：F直行 L左 R右 */
    static uint32_t lostAt  = 0;     /* 从什么时候开始丢线的 */

    Track_Read(&l, &m, &r);

    if (m) {                                       /* 中间在线上 */
        if (l && !r) {
            /* 线偏左 -> 左转修正：左轮减速 */
            Motor_SetSpeed(TRACK_BASE - TRACK_DIFF, TRACK_BASE);
            lastCmd = 'L';
        } else if (r && !l) {
            /* 线偏右 -> 右转修正：右轮减速 */
            Motor_SetSpeed(TRACK_BASE, TRACK_BASE - TRACK_DIFF);
            lastCmd = 'R';
        } else {
            /* 正常直行（包括左中右都在线上，比如路口） */
            Motor_SetSpeed(TRACK_BASE, TRACK_BASE);
            lastCmd = 'F';
        }
    } else if (l && !r) {                          /* 中间丢线、左探头还有线 */
        Motor_SetSpeed(0, TRACK_BASE);             /* 大幅左转：左轮停 */
        lastCmd = 'L';
    } else if (r && !l) {                          /* 中间丢线、右探头还有线 */
        Motor_SetSpeed(TRACK_BASE, 0);             /* 大幅右转：右轮停 */
        lastCmd = 'R';
    } else {
        lost = 1;                                  /* 三个都看不到线 */
    }

    if (lost) {
        if (lostAt == 0) {
            lostAt = g_ms;                         /* 刚丢线，记下时间 */
        }
        if ((g_ms - lostAt) < 600) {
            /* 0.6秒内：按原来的方向继续走，把线"找"回来（过急弯很有用） */
            if (lastCmd == 'L') {
                Motor_SetSpeed(0, TRACK_BASE);
            } else if (lastCmd == 'R') {
                Motor_SetSpeed(TRACK_BASE, 0);
            } else {
                Motor_SetSpeed(TRACK_BASE, TRACK_BASE);
            }
        } else {
            /* 找了0.6秒还没找到：停车，防止跑飞 */
            Motor_Stop();
        }
    } else {
        lostAt = 0;                                /* 看到线了，清丢线计时 */
    }
}
