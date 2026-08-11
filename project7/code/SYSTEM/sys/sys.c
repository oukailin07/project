#include "sys.h"

/*
 * 系统中断函数备用
 * THUMB 指令不支持汇编内联, 使用如下方法实现执行汇编指令 WFI
 */
void WFI_SET(void)
{
	__ASM volatile("wfi");
}

/* 关闭所有中断 */
void INTX_DISABLE(void)
{
	__ASM volatile("cpsid i");
}

/* 开启所有中断 */
void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");
}

/* 设置栈顶地址
 * addr: 栈顶地址
 */
__asm void MSR_MSP(u32 addr)
{
    MSR MSP, r0 			//set Main Stack value
    BX r14
}
