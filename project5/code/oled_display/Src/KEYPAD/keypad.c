/**
 * keypad.c - 4×4 矩阵按键模块驱动
 *
 * 行线(ROW): PB4, PB5, PB6, PB7  — 推挽输出
 * 列线(COL): PB8, PB9, PB14, PB15 — 上拉输入
 *
 * 逐行扫描: 依次将每行拉低，读取列状态，
 *           若某列为低电平则该键按下。
 *           带有简单消抖处理。
 */

#include "keypad.h"
#include "delay.h"

/* ── 消抖参数 ────────────────────────────────────────── */
#define DEBOUNCE_MS     20U   /* 消抖延时(ms) */
#define SCAN_DELAY_MS    2U   /* 行切换延时(ms) */

/* ── 按键映射表（4行 × 4列）─────────────────────────── */
static const char key_map[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' },
};

/* ── 行引脚数组 ──────────────────────────────────────── */
static const u16 row_pins[4] = {
    KP_ROW1_PIN,
    KP_ROW2_PIN,
    KP_ROW3_PIN,
    KP_ROW4_PIN,
};

/* ── 列引脚数组 ──────────────────────────────────────── */
static const u16 col_pins[4] = {
    KP_COL1_PIN,
    KP_COL2_PIN,
    KP_COL3_PIN,
    KP_COL4_PIN,
};

/**
 * @brief  初始化 4×4 矩阵按键 GPIO
 *         行线: 推挽输出, 默认高电平
 *         列线: 上拉输入
 */
void KEYPAD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 行线: PB4~PB7, 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = KP_ROW_PINS;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KP_ROW_PORT, &GPIO_InitStructure);

    /* 列线: PB8,PB9,PB14,PB15, 上拉输入 */
    GPIO_InitStructure.GPIO_Pin  = KP_COL_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KP_COL_PORT, &GPIO_InitStructure);

    /* 所有行初始拉高 */
    GPIO_SetBits(KP_ROW_PORT, KP_ROW_PINS);
}

/**
 * @brief  扫描按键，返回按下的键值
 *
 * 逐行拉低 → 读取列 → 若某列为低则该键按下。
 * 检测到按键后延时消抖并等待释放。
 *
 * @return 按键字符, 无按键返回 '\0'
 */
char KEYPAD_Scan(void)
{
    u8 row, col;

    for (row = 0; row < 4; row++)
    {
        /* 将当前行拉低，其余行保持高 */
        GPIO_ResetBits(KP_ROW_PORT, row_pins[row]);

        /* 短暂延时让电平稳定 */
        delay_ms(SCAN_DELAY_MS);

        /* 扫描列 */
        for (col = 0; col < 4; col++)
        {
            if (GPIO_ReadInputDataBit(KP_COL_PORT, col_pins[col]) == 0)
            {
                /* 消抖: 延时后再次确认 */
                delay_ms(DEBOUNCE_MS);
                if (GPIO_ReadInputDataBit(KP_COL_PORT, col_pins[col]) == 0)
                {
                    /* 等待按键释放 */
                    while (GPIO_ReadInputDataBit(KP_COL_PORT, col_pins[col]) == 0);

                    /* 恢复当前行 */
                    GPIO_SetBits(KP_ROW_PORT, row_pins[row]);

                    return key_map[row][col];
                }
            }
        }

        /* 恢复当前行 */
        GPIO_SetBits(KP_ROW_PORT, row_pins[row]);
    }

    return '\0';   /* 无按键 */
}
