#include "delay.h"

/* SysTick 时钟源配置函数 (APM32 在 misc 驱动中) */
#include "apm32f10x_misc.h"

/*
 * 使用 SysTick 普通(非 OS)模式延时
 * 适用于 APM32F10x 系列
 * 由 STM32 移植到 APM32F103: SysTick_CLKSourceConfig → SysTick_ConfigCLKSource
 */

static u8  fac_us = 0;	/* us 延时倍乘数 */
static u16 fac_ms = 0;	/* ms 延时倍乘数, 非 OS 时表示每 ms 的 systick 时钟数 */

/*
 * 初始化延时函数
 * SYSTICK 的时钟固定为 HCLK 时钟的 1/8
 * SYSCLK: 系统时钟
 */
void delay_init(void)
{
    SysTick_ConfigCLKSource(SYSTICK_CLK_SOURCE_HCLK_DIV8);  /* 选择外部时钟 HCLK/8 */
    fac_us = SystemCoreClock / 8000000;                     /* 为系统时钟的1/8 */
    fac_ms = (u16)fac_us * 1000;                            /* 非 OS 下, 每 ms 需要的 systick 时钟数 */
}

/*
 * 延时 nus
 * nus 为要延时的 us 数.
 */
void delay_us(u32 nus)
{
    u32 temp;
    SysTick->LOAD = nus * fac_us;               /* 时间加载 */
    SysTick->VAL  = 0x00;                       /* 清空计数器 */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   /* 开始倒数 */
    do
    {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));  /* 等待时间到达 */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;      /* 关闭计数器 */
    SysTick->VAL = 0x00;                            /* 清空计数器 */
}

/*
 * 延时 nms
 * 注意 nms 的范围:
 * SysTick->LOAD 为 24 位寄存器, 所以最大延时为:
 * nms <= 0xffffff * 8 * 1000 / SYSCLK
 * SYSCLK 单位为 Hz, nms 单位为 ms
 * 在 96M 条件下, nms <= 1398
 */
void delay_ms(u16 nms)
{
    u32 temp;
    SysTick->LOAD = (u32)nms * fac_ms;          /* 时间加载 (SysTick->LOAD 为 24bit) */
    SysTick->VAL  = 0x00;                       /* 清空计数器 */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   /* 开始倒数 */
    do
    {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));  /* 等待时间到达 */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;      /* 关闭计数器 */
    SysTick->VAL = 0x00;                            /* 清空计数器 */
}
