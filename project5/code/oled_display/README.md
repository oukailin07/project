# STM32F103C8 超声波测距 + 电机控制终端

## 1. 接线总览

### STM32F103C8 最小系统引脚分配

```
                    ┌──────────────────────┐
                    │    STM32F103C8T6      │
                    │                      │
      HC-SR04 TRIG ─│ PA0              PB0 │─ L298N IN1
      HC-SR04 ECHO ─│ PA1              PB1 │─ L298N IN2
    L298N ENA(PWM) ─│ PA2              PB2 │─ (BOOT1, 勿用)
    L298N ENB(PWM) ─│ PA3              PB3 │─ JTAG(可用)
                    │ PA4              PB4 │─ 矩阵键盘 ROW1
                    │ PA5              PB5 │─ 矩阵键盘 ROW2
                    │ PA6              PB6 │─ 矩阵键盘 ROW3
                    │ PA7              PB7 │─ 矩阵键盘 ROW4
                    │ PA8              PB8 │─ 矩阵键盘 COL1
                    │ PA9              PB9 │─ 矩阵键盘 COL2
                    │ PA10             PB10│─ L298N IN3
                    │ PA11             PB11│─ L298N IN4
                    │ PA12             PB12│─ OLED SCL
                    │ PA13(SWD)        PB13│─ OLED SDA
                    │ PA14(SWD)        PB14│─ 矩阵键盘 COL3
                    │ PA15             PB15│─ 矩阵键盘 COL4
                    │                      │
                    └──────────────────────┘
```

### 各模块接线表

#### HC-SR04 超声波测距

| HC-SR04 | STM32 | 说明 |
|---------|-------|------|
| VCC | 5V | 供电 5V |
| GND | GND | 共地 |
| TRIG | PA0 | 触发信号，GPIO 推挽输出 |
| ECHO | PA1 | 回波信号，GPIO 浮空输入 |

#### SSD1306 OLED 显示屏（I2C）

| OLED | STM32 | 说明 |
|------|-------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 共地 |
| SCL | PB12 | I2C 时钟线 |
| SDA | PB13 | I2C 数据线 |

#### 4×4 矩阵键盘

| 键盘引脚 | STM32 | 说明 |
|----------|-------|------|
| ROW1 | PB4 | 行扫描，推挽输出 |
| ROW2 | PB5 | 行扫描，推挽输出 |
| ROW3 | PB6 | 行扫描，推挽输出 |
| ROW4 | PB7 | 行扫描，推挽输出 |
| COL1 | PB8 | 列检测，上拉输入 |
| COL2 | PB9 | 列检测，上拉输入 |
| COL3 | PB14 | 列检测，上拉输入 |
| COL4 | PB15 | 列检测，上拉输入 |

> 键盘按键布局：
> ```
> [1] [2] [3] [A]
> [4] [5] [6] [B]
> [7] [8] [9] [C]
> [*] [0] [#] [D]
> ```

#### L298N 电机驱动

| L298N | STM32 | 说明 |
|-------|-------|------|
| ENA | PA2 | 电机A 使能/PWM 调速 (TIM2_CH3) |
| IN1 | PB0 | 电机A 方向控制 |
| IN2 | PB1 | 电机A 方向控制 |
| ENB | PA3 | 电机B 使能/PWM 调速 (TIM2_CH4) |
| IN3 | PB10 | 电机B 方向控制 |
| IN4 | PB11 | 电机B 方向控制 |
| VCC | 12V/5V | 电机电源（根据电机规格） |
| GND | GND | 共地 |

---

## 2. 软件架构

```
main.c (应用层)
  ├── oled_disp.h    ← 显示接口（不暴露 I2C 细节）
  │     └── oled_drv.h   ← I2C 底层驱动
  ├── hc_sr04.h      ← 超声波测距接口
  ├── keypad.h       ← 矩阵按键接口
  └── l298n.h        ← 电机驱动接口
```

### 分层设计

| 层级 | 文件 | 职责 |
|------|------|------|
| 应用层 | `main.c` | 业务逻辑：按键 → 电机控制，定时刷新测距 |
| 显示接口 | `oled_disp.h/c` | 字符/字符串/数字/中文/位图/浮点数显示 |
| 显示驱动 | `oled_drv.h/c` | I2C 软件模拟、SSD1306 初始化序列 |
| 字库数据 | `oledfont.h` | ASCII 6×8 / 8×16 点阵 + 中文 16×32 点阵 |
| 位图数据 | `bmp.h` | 64×32 图片数据 |
| 测距驱动 | `hc_sr04.h/c` | HC-SR04 触发/回波测量 |
| 按键驱动 | `keypad.h/c` | 4×4 矩阵逐行扫描 + 消抖 |
| 电机驱动 | `l298n.h/c` | L298N TIM2 PWM + 方向控制 |

---

## 3. API 接口说明

### 3.1 OLED 显示 — `oled_disp.h`

#### 基础控制

```c
void OLED_Init(void);       // 初始化（调用底层驱动的 I2C + SSD1306 初始化序列）
void OLED_Clear(void);      // 清屏（全黑）
void OLED_Fill(unsigned char fill_Data);  // 全屏填充指定字节（0x00=黑, 0xFF=白）
void OLED_Display_On(void); // 开启显示
void OLED_Display_Off(void);// 关闭显示
void OLED_Set_Pos(unsigned char x, unsigned char y); // 设置光标：x=列(0~127), y=页(0~7)
```

#### 文本显示

```c
// 显示字符 (x=列, y=页, chr=ASCII字符, Char_Size=12或16)
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size);

// 显示字符串 (p='\0'结尾的字符串)
void OLED_ShowString(u8 x, u8 y, u8 *p, u8 Char_Size);

// 显示整数 (右对齐、前导空格, len=显示位数)
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size);

// 显示浮点数 (intLen=整数位数, decLen=小数位数)
void OLED_ShowFloat(u8 x, u8 y, float val, u8 intLen, u8 decLen, u8 size);

// 显示中文 (no=Hzk字库索引号)
void OLED_ShowCHinese(u8 x, u8 y, u8 no);
```

#### 使用示例

```c
#include "oled_disp.h"

OLED_Init();
OLED_Clear();

// 第0行显示标题（每页8像素高，共8页=64像素）
OLED_ShowString(0, 0, (u8 *)"Hello World", 16);

// 第2页显示温度
OLED_ShowFloat(0, 2, 23.5f, 2, 1, 16);  // 显示为 "23.5"

// 第4页显示带单位的数值
char buf[32];
sprintf(buf, "Dist:%05.1fcm", 123.4);
OLED_ShowString(0, 4, (u8 *)buf, 16);
```

> **坐标系说明：**
> - X 轴：0~127 列（像素）
> - Y 轴：0~7 页（每页 8 像素，共 64 像素）
> - 16 号字体占 2 页高、8 像素宽
> - 12 号字体占 1 页高、6 像素宽

---

### 3.2 HC-SR04 超声波测距 — `hc_sr04.h`

```c
void  HC_SR04_Init(void);        // 初始化 GPIO
float HC_SR04_GetDistance(void); // 返回距离(cm)，超时/无回波返回 0.0
```

#### 使用示例

```c
#include "hc_sr04.h"

HC_SR04_Init();

float dist = HC_SR04_GetDistance();
if (dist > 0.0f && dist < 400.0f)
{
    printf("距离: %.1f cm\n", dist);
}
else
{
    printf("超出量程\n");
}
```

> **注意事项：**
> - 量程 2cm ~ 400cm
> - 单次测量耗时约 1~38ms（取决于距离）
> - 返回 0.0 表示超时（无回波或超出量程）
> - 测量期间会阻塞 CPU（软件延时）

---

### 3.3 4×4 矩阵按键 — `keypad.h`

```c
void KEYPAD_Init(void);  // 初始化 GPIO
char KEYPAD_Scan(void);  // 扫描按键，返回字符；无按键返回 '\0'
```

#### 使用示例

```c
#include "keypad.h"

KEYPAD_Init();

char key = KEYPAD_Scan();
switch (key)
{
case '1': /* 按键1 */ break;
case '2': /* 按键2 */ break;
case 'A': /* 按键A */ break;
case '#': /* 按键# */ break;
case '\0': /* 无按键 */ break;
}
```

> **注意事项：**
> - 阻塞式扫描：检测到按键后会等待释放才返回
> - 带 20ms 消抖处理
> - 在同一时刻只返回一个按键（无组合键）
> - 扫描耗时约 12~30ms

---

### 3.4 L298N 电机驱动 — `l298n.h`

```c
void L298N_Init(void);               // 初始化 GPIO + TIM2 PWM

void L298N_MotorA_Set(s8 speed);     // 电机A: speed = -100~100
void L298N_MotorB_Set(s8 speed);     // 电机B: speed = -100~100

void L298N_MotorA_Stop(void);        // 停止电机A
void L298N_MotorB_Stop(void);        // 停止电机B
void L298N_Stop(void);               // 同时停止两个电机
```

#### 速度与方向对应关系

| speed 值 | 方向 | IN1 | IN2 | PWM 占空比 |
|----------|------|-----|-----|-----------|
| 100 | 正转（最快） | H | L | 100% |
| 50 | 正转（半速） | H | L | 50% |
| 0 | 停止 | L | L | 0% |
| -50 | 反转（半速） | L | H | 50% |
| -100 | 反转（最快） | L | H | 100% |

#### 使用示例

```c
#include "l298n.h"

L298N_Init();

// 电机A 正转半速
L298N_MotorA_Set(50);
delay_ms(2000);

// 电机A 停止
L298N_MotorA_Stop();

// 电机B 反转全速
L298N_MotorB_Set(-100);
delay_ms(1000);

// 全部停止
L298N_Stop();
```

> **注意事项：**
> - PWM 频率 1kHz，分辨率 0.1%
> - L298N 模块需单独供电（12V/5V，根据电机规格）
> - STM32 与 L298N 必须共地
> - 控制逻辑: IN1/IN2 同高 = 刹车，同低 = 滑行停止

---

## 4. 业务逻辑开发指南

### 典型使用模式

```c
#include "oled_disp.h"   // 显示
#include "hc_sr04.h"    // 测距
#include "keypad.h"     // 按键
#include "l298n.h"      // 电机

int main(void)
{
    // 1. 初始化
    delay_init();
    OLED_Init();
    OLED_Clear();
    HC_SR04_Init();
    KEYPAD_Init();
    L298N_Init();

    // 2. 业务循环
    while (1)
    {
        // ── 输入处理 ──
        char key = KEYPAD_Scan();
        float dist = HC_SR04_GetDistance();

        // ── 业务逻辑 ──
        if (dist < 20.0f)  // 障碍物小于20cm → 急停
        {
            L298N_Stop();
            OLED_ShowString(0, 0, (u8 *)"OBSTACLE! STOP", 16);
        }
        else if (key == 'A')  // 按键A → 前进
        {
            L298N_MotorA_Set(80);
            L298N_MotorB_Set(80);
        }
        else if (key == '#')  // 按键# → 停止
        {
            L298N_Stop();
        }

        // ── 显示更新 ──
        char buf[32];
        sprintf(buf, "Dist: %05.1fcm", dist);
        OLED_ShowString(0, 2, (u8 *)buf, 16);

        delay_ms(50);
    }
}
```

### 添加新模块

1. 在 `Src/` 下新建目录和 `.h/.c` 文件
2. 应用层 `#include` 新头文件
3. 在 Keil 工程中添加 `.c` 文件和包含路径
4. 如需定时器/外设，在 `stm32f10x_conf.h` 中确认对应模块已开启

---

## 5. Keil MDK 工程配置

### 需添加的源文件

| 目录 | 文件 | 备注 |
|------|------|------|
| `Src/OLED/` | `oled_drv.c`, `oled_disp.c` | OLED 驱动+显示 |
| `Src/HC_SR04/` | `hc_sr04.c` | 超声波测距 |
| `Src/KEYPAD/` | `keypad.c` | 矩阵按键 |
| `Src/L298N/` | `l298n.c` | 电机驱动 |
| `SYSTEM/delay/` | `delay.c` | 延时函数 |
| `SYSTEM/sys/` | `sys.c` | 系统函数 |
| `RTE/...` | `system_stm32f10x.c` | 系统初始化 |

### 需添加的包含路径

```
Src/OLED
Src/HC_SR04
Src/KEYPAD
Src/L298N
SYSTEM/delay
SYSTEM/sys
RTE/Device/STM32F103C8
RTE/_Target_1
```

### 需使能的 STM32 标准外设库模块

- `stm32f10x_gpio.c` — GPIO
- `stm32f10x_rcc.c` — 时钟
- `stm32f10x_tim.c` — 定时器（L298N PWM）
- `stm32f10x_exti.c` — 外部中断（如需要）
- `misc.c` — NVIC

---

## 6. 当前引脚占用总览

| 引脚 | 功能 | 模块 |
|------|------|------|
| PA0 | TRIG 触发 | HC-SR04 |
| PA1 | ECHO 回波 | HC-SR04 |
| PA2 | ENA PWM | L298N (TIM2_CH3) |
| PA3 | ENB PWM | L298N (TIM2_CH4) |
| PB0 | IN1 方向 | L298N |
| PB1 | IN2 方向 | L298N |
| PB4~PB7 | ROW1~4 | 矩阵键盘 |
| PB8, PB9 | COL1~2 | 矩阵键盘 |
| PB10 | IN3 方向 | L298N |
| PB11 | IN4 方向 | L298N |
| PB12 | SCL | OLED (I2C) |
| PB13 | SDA | OLED (I2C) |
| PB14, PB15 | COL3~4 | 矩阵键盘 |

**剩余可用引脚：** PA4~PA8, PA11~PA12, PA15, PB2~PB3（JTAG，SWD 模式下可用）

---

## 7. 编译环境

- **MCU**: STM32F103C8T6 (64KB Flash, 20KB RAM)
- **IDE**: Keil MDK v5
- **标准库**: STM32F10x Standard Peripheral Library v3.5
- **调试器**: ST-Link / J-Link (SWD: PA13-SWDIO, PA14-SWCLK)
