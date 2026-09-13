/**
  ******************************************************************************
  * @file    iravoid.c
  * @brief   避障逻辑（三路红外探头 + 超声波）
  *
  * 原理（小白版）：
  *   红外探头水平朝前发射红外光，前方一定距离内有东西就会反射回来，
  *   模块检测到反射就拉低/拉高输出脚（电位器可调检测距离）。
  *
  * 避障策略（状态机）：
  *   平时直行；
  *   左边有障碍 -> 原地右转绕开
  *   右边有障碍 -> 原地左转绕开
  *   正前方有障碍（中间探头 或 超声波太近）-> 停一下，后退，再挑个空的方向转过去
  ******************************************************************************
  */
#include "iravoid.h"
#include "config.h"
#include "motor.h"
#include "ultrasonic.h"

/* 避障状态机的4个状态 */
#define AV_ST_FORWARD   0    /* 前进 */
#define AV_ST_BACK      1    /* 后退 */
#define AV_ST_TURN_L    2    /* 原地左转 */
#define AV_ST_TURN_R    3    /* 原地右转 */

void IrAvoid_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

    gpio.GPIO_Pin = OBS_L_PIN;
    GPIO_Init(OBS_L_PORT, &gpio);
    gpio.GPIO_Pin = OBS_M_PIN;
    GPIO_Init(OBS_M_PORT, &gpio);
    gpio.GPIO_Pin = OBS_R_PIN;
    GPIO_Init(OBS_R_PORT, &gpio);
}

/* 读3个探头。返回1表示"有障碍" */
void Ir_Read(uint8_t *l, uint8_t *m, uint8_t *r)
{
    *l = (GPIO_ReadInputDataBit(OBS_L_PORT, OBS_L_PIN) == OBST_LEVEL) ? 1 : 0;
    *m = (GPIO_ReadInputDataBit(OBS_M_PORT, OBS_M_PIN) == OBST_LEVEL) ? 1 : 0;
    *r = (GPIO_ReadInputDataBit(OBS_R_PORT, OBS_R_PIN) == OBST_LEVEL) ? 1 : 0;
}

/* 每10ms调用一次。g_frontCm由主程序定期用超声波刷新 */
void Avoid_Task(void)
{
    static uint8_t  state = AV_ST_FORWARD;
    static uint32_t tEnd  = 0;               /* 当前动作做到什么时刻 */
    uint8_t l, m, r;

    Ir_Read(&l, &m, &r);

    switch (state) {

    case AV_ST_FORWARD:                      /* --- 正常前进 --- */
        if ((g_frontCm < AVOID_DIST_CM) || m) {
            /* 正前方堵死：先停再退（只用超声波判断也行，加中间探头更保险） */
            Motor_Stop();
            state = AV_ST_BACK;
            tEnd  = g_ms + 400;              /* 停顿+后退开始时刻 */
        } else if (l) {
            /* 左边有障碍：原地右转 */
            Motor_SetSpeed(-AVOID_TURN_PWM, AVOID_TURN_PWM);
        } else if (r) {
            /* 右边有障碍：原地左转 */
            Motor_SetSpeed(AVOID_TURN_PWM, -AVOID_TURN_PWM);
        } else {
            /* 前方安全：直行 */
            Motor_SetSpeed(AVOID_PWM, AVOID_PWM);
        }
        break;

    case AV_ST_BACK:                         /* --- 后退一小段 --- */
        Motor_SetSpeed(-AVOID_PWM, -AVOID_PWM);
        if (g_ms >= tEnd) {
            /* 后退完，挑个没障碍的方向转 */
            state = l ? AV_ST_TURN_R : AV_ST_TURN_L;
            tEnd  = g_ms + 500;              /* 大约转90度，可按需调整 */
        }
        break;

    case AV_ST_TURN_L:                       /* --- 原地左转一小段 --- */
        Motor_SetSpeed(-AVOID_TURN_PWM, AVOID_TURN_PWM);
        if (g_ms >= tEnd) state = AV_ST_FORWARD;
        break;

    case AV_ST_TURN_R:                       /* --- 原地右转一小段 --- */
        Motor_SetSpeed(AVOID_TURN_PWM, -AVOID_TURN_PWM);
        if (g_ms >= tEnd) state = AV_ST_FORWARD;
        break;

    default:
        state = AV_ST_FORWARD;
        break;
    }
}
