/**
  ******************************************************************************
  * @file    motor.c
  * @brief   TB6612 电机驱动
  *
  * 原理（小白版）：
  *   TB6612有两个通道：A通道管左边两个电机，B通道管右边两个电机。
  *   每个通道3根控制线：
  *     xIN1、xIN2 决定转向：  1,0=正转   0,1=反转   0,0=滑行停止
  *     PWMA/PWMB 是"油门"：  单片机输出PWM脉冲，脉冲越密(占空比越大)转得越快
  *   STBY 是总开关：高电平才允许电机工作（防止上电瞬间乱跑）
  *
  * 转向对照表：
  *   左转 = 左轮慢/停，右轮快
  *   右转 = 右轮慢/停，左轮快
  *   原地转 = 一侧反转，一侧正转
  ******************************************************************************
  */
#include "motor.h"
#include "config.h"

/* 记录当前左右轮的转向（给编码器算速度方向用）：0停 1正 -1反 */
static int8_t s_dirL = 0;
static int8_t s_dirR = 0;

/* 把速度限制到安全范围，并且保证不为0时大于最小启动占空比 */
static int16_t Speed_Clamp(int16_t v)
{
    if (v > PWM_MAX)        v = PWM_MAX;
    if (v < -PWM_MAX)       v = -PWM_MAX;
    if (v > 0 && v < MIN_RUN_PWM)   v = MIN_RUN_PWM;   /* 太慢直接给到能转动的值 */
    if (v < 0 && v > -MIN_RUN_PWM)  v = -MIN_RUN_PWM;
    return v;
}

/* 设置A通道（左侧） */
static void LeftMotor_Set(int16_t speed)
{
    if (speed > 0) {                       /* 正转 */
        GPIO_SetBits(M_AIN1_PORT, M_AIN1_PIN);
        GPIO_ResetBits(M_AIN2_PORT, M_AIN2_PIN);
        TIM_SetCompare1(TIM3, (uint16_t)speed);
        s_dirL = 1;
    } else if (speed < 0) {                /* 反转 */
        GPIO_ResetBits(M_AIN1_PORT, M_AIN1_PIN);
        GPIO_SetBits(M_AIN2_PORT, M_AIN2_PIN);
        TIM_SetCompare1(TIM3, (uint16_t)(-speed));
        s_dirL = -1;
    } else {                               /* 停止（两个输入都拉低=滑行） */
        GPIO_ResetBits(M_AIN1_PORT, M_AIN1_PIN);
        GPIO_ResetBits(M_AIN2_PORT, M_AIN2_PIN);
        TIM_SetCompare1(TIM3, 0);
        s_dirL = 0;
    }
}

/* 设置B通道（右侧），原理同上 */
static void RightMotor_Set(int16_t speed)
{
    if (speed > 0) {
        GPIO_SetBits(M_BIN1_PORT, M_BIN1_PIN);
        GPIO_ResetBits(M_BIN2_PORT, M_BIN2_PIN);
        TIM_SetCompare2(TIM3, (uint16_t)speed);
        s_dirR = 1;
    } else if (speed < 0) {
        GPIO_ResetBits(M_BIN1_PORT, M_BIN1_PIN);
        GPIO_SetBits(M_BIN2_PORT, M_BIN2_PIN);
        TIM_SetCompare2(TIM3, (uint16_t)(-speed));
        s_dirR = -1;
    } else {
        GPIO_ResetBits(M_BIN1_PORT, M_BIN1_PIN);
        GPIO_ResetBits(M_BIN2_PORT, M_BIN2_PIN);
        TIM_SetCompare2(TIM3, 0);
        s_dirR = 0;
    }
}

void Motor_Init(void)
{
    GPIO_InitTypeDef   gpio;
    TIM_TimeBaseInitTypeDef tim;
    TIM_OCInitTypeDef  oc;

    /* 1. 打开相关外设的时钟（不开钟外设就是"断电"状态） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 2. 方向控制脚：普通推挽输出 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    gpio.GPIO_Pin = M_AIN1_PIN | M_AIN2_PIN | M_STBY_PIN;
    GPIO_Init(M_AIN1_PORT, &gpio);
    gpio.GPIO_Pin = M_BIN1_PIN | M_BIN2_PIN;
    GPIO_Init(M_BIN1_PORT, &gpio);

    /* 3. PWM输出脚（PA6/PA7）：复用推挽，让TIM3来控制这个脚 */
    gpio.GPIO_Pin  = M_PWMA_PIN | M_PWMB_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(M_PWMA_PORT, &gpio);

    /* 4. 配置TIM3：产生10kHz的PWM */
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler     = 0;                    /* 不分频，72MHz */
    tim.TIM_Period        = PWM_PERIOD - 1;       /* 7200计一个周期 -> 10kHz */
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tim);

    /* 5. 配置PWM输出通道1(A)和通道2(B) */
    TIM_OCStructInit(&oc);
    oc.TIM_OCMode      = TIM_OCMode_PWM1;         /* 计数值<比较值时输出高 */
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 0;                       /* 初始占空比0（电机不动） */
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &oc);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM3, &oc);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);                        /* 启动定时器 */

    /* 6. 先拉低STBY（关闭电机），等初始化全部完成再打开 */
    GPIO_ResetBits(M_STBY_PORT, M_STBY_PIN);

    /* 7. 确保所有控制脚为"停止"状态，然后打开总开关 */
    LeftMotor_Set(0);
    RightMotor_Set(0);
    GPIO_SetBits(M_STBY_PORT, M_STBY_PIN);
}

void Motor_SetSpeed(int16_t left, int16_t right)
{
    LeftMotor_Set(Speed_Clamp(left));
    RightMotor_Set(Speed_Clamp(right));
}

void Motor_Stop(void)
{
    LeftMotor_Set(0);
    RightMotor_Set(0);
}

int8_t Motor_GetDir(uint8_t side)
{
    return (side == 0) ? s_dirL : s_dirR;
}
