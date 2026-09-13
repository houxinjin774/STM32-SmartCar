/**
  ******************************************************************************
  * @file    bluetooth.c
  * @brief   HC-05 蓝牙串口驱动 + printf重定向
  *
  * 原理（小白版）：
  *   HC-05其实就是一个"无线串口线"：手机APP发的字，从HC-05的TXD脚出来，
  *   进单片机的串口1(PA10)；单片机往串口1(PA9)写的字，从HC-05发到手机。
  *
  *   收：串口收到1个字节就触发中断，把字节放进"环形缓冲区"存着，
  *       主循环慢慢取（这样不占用主循环时间，也不会丢数据）。
  *   发：把printf重定向到串口1，直接printf就能发消息到手机。
  ******************************************************************************
  */
#include "bluetooth.h"
#include "config.h"

/* 环形缓冲区：手机一次发来好几个字时先存起来 */
#define RX_BUF_SIZE  32
static volatile uint8_t s_rxBuf[RX_BUF_SIZE];
static volatile uint8_t s_rxHead = 0;    /* 写位置 */
static volatile uint8_t s_rxTail = 0;    /* 读位置 */
static volatile uint32_t s_lastRxMs = 0; /* 最后一次收到数据的时刻（失联保护用） */

void Bluetooth_Init(uint32_t baud)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef  nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* PA9 = TX（单片机 -> HC-05 RXD）：复用推挽输出 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* PA10 = RX（HC-05 TXD -> 单片机）：浮空输入 */
    gpio.GPIO_Pin  = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    /* 串口1：9600波特率，8数据位，1停止位，无校验（HC-05默认格式） */
    USART_StructInit(&usart);
    usart.USART_BaudRate            = baud;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &usart);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);   /* 收到1字节就中断 */
    USART_Cmd(USART1, ENABLE);

    nvic.NVIC_IRQChannel                   = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 3;   /* 串口优先级最低 */
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);
}

/* 中断服务程序（stm32f10x_it.c）调用：把收到的字节放进缓冲区 */
void BT_RxPush(uint8_t d)
{
    uint8_t next = (uint8_t)((s_rxHead + 1) % RX_BUF_SIZE);
    s_lastRxMs = g_ms;
    if (next != s_rxTail) {                  /* 缓冲区没满才存 */
        s_rxBuf[s_rxHead] = d;
        s_rxHead = next;
    }
}

/* 最后一次收到蓝牙数据的时刻（毫秒时间戳） */
uint32_t BT_GetLastRxMs(void)
{
    return s_lastRxMs;
}

/* 主循环调用：取一个字符。返回1=取到了，0=没有 */
uint8_t BT_GetChar(char *c)
{
    if (s_rxTail == s_rxHead) return 0;      /* 空的 */
    *c = (char)s_rxBuf[s_rxTail];
    s_rxTail = (uint8_t)((s_rxTail + 1) % RX_BUF_SIZE);
    return 1;
}

/* printf 重定向：printf的内容全部从蓝牙发出去 */
int fputc(int ch, FILE *f)
{
    (void)f;
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
        /* 等待发送寄存器空 */
    }
    return ch;
}

void BT_PrintHelp(void)
{
    printf("\r\n===== STM32 智能小车 就绪 =====\r\n");
    printf("模式: 1=遥控 2=循迹 3=避障 4=传感器 0=停止\r\n");
    printf("遥控: F前 B后 L左 R右 S停\r\n");
    printf("+/- = 加速/减速   D=测距 V=测速 T=传感器\r\n");
    printf("==============================\r\n");
}
