# Project — STM32F103C8 嵌入式项目合集

基于 STM32F103C8 微控制器的三个嵌入式系统项目，涵盖电机监测、压力注射控制、温湿度采集显示。所有项目均支持 Keil MDK 编译和 Proteus 仿真。

---

## 项目概览

| 项目 | 名称 | 核心功能 | 传感器/外设 |
|------|------|----------|-------------|
| [project1](./project1/) | 电机状态监测系统 | 多参数实时监测 + WiFi上传 | ADXL1002, MAX31855, 电流互感器, ESP8266 |
| [project2](./project2/) | 压力注射装置 | 步进电机精确控制 + 状态机 | 压力传感器, 28BYJ-48, EEPROM |
| [project3](./project3/) | 温湿度显示系统 | 双节点传感器采集 + OLED显示 | SHT21, OLED SSD1306 |

---

## project1 — 电机状态监测系统

### 功能说明

实时监测电机运行状态，采集振动、温度、三相电流参数，支持 OLED 本地显示、阈值报警、按键参数设置、WiFi 数据上传。

### 主要特性

- **5 通道 ADC 采集**：振动（ADXL1002）、温度（MAX31855）、A/B/C 三相电流
- **OLED 显示**：I2C SSD1306，主页显示全部参数 + 运行状态
- **四按键参数设置**：振动阈值、温度阈值、电流阈值、WiFi 上传间隔均可调
- **报警功能**：任一参数超阈值触发蜂鸣器 + LED 报警，OLED 闪烁 "ALARM!"
- **WiFi 上传**：USART3 连接 ESP8266，JSON 格式定时上传数据
- **Proteus 仿真**：支持纯软件仿真，按键/传感器用虚拟元件替代

### 硬件引脚

| 外设 | 引脚 | 说明 |
|------|------|------|
| 振动传感器 | PA0 (ADC) | ADXL1002 模拟输出 |
| 温度传感器 | PA1 (ADC) | MAX31855 模拟 |
| A/B/C 相电流 | PA2/PA3/PA4 (ADC) | 电流互感器 |
| OLED SCL/SDA | PB12/PB13 | I2C |
| ESP8266 TX/RX | PB10/PB11 | USART3, 9600bps |
| 按键 1-4 | PB0, PA5, PA6, PB1 | 确认/+/-/切换 |
| 蜂鸣器/LED | PB3/PB4 | 报警输出 |

### 文件结构

```
project1/
├── main.c                    # 主程序（全部业务逻辑）
├── Main.uvprojx              # Keil 工程文件
├── Src/
│   ├── ADC/                  # ADC 驱动（5通道）
│   ├── KEY/                  # 按键驱动（4键+指示灯）
│   ├── LED/                  # 报警驱动（蜂鸣器+LED）
│   ├── OLED/                 # OLED I2C 驱动
│   └── USART3/               # USART3 驱动（WiFi）
├── SYSTEM/
│   ├── delay/                # 延时函数
│   ├── sys/                  # 系统宏（位带操作）
│   └── usart/                # USART1/2/3 综合驱动
├── RTE/Device/STM32F103C8/   # 芯片启动配置
├── proteus/                  # Proteus 仿真文件
└── 程序功能说明.md             # 详细功能文档
```

---

## project2 — 压力注射装置

### 功能说明

医用压力注射装置的嵌入式控制系统，通过步进电机驱动注射器，实现排气、推注、压迫计时、回抽的完整工作流程。支持 OLED 多级菜单显示、按键操作、EEPROM 参数存储。

### 主要特性

- **7 状态状态机**：IDLE → SETUP → PURGE → INJECT → COMPRESS → RETRACT → ERROR
- **步进电机控制**：28BYJ-48 8拍半步步进，软启动斜坡加减速（300~1200Hz）
- **三级 OLED 菜单**：待机界面 → 实时监测界面 → 结果摘要界面
- **按键操作**：3 键控制（确认/启动/停止、参数+、参数-）
- **EEPROM 存储**：语言、目标压力、版本号、压力曲线（120点）、错误日志
- **安全保护**：过压保护（>450kPa）、低电量保护（<10%）、紧急停止
- **Proteus 仿真**：支持完整流程仿真

### 硬件引脚

| 外设 | 引脚 | 说明 |
|------|------|------|
| OLED SCL/SDA | PB12/PB13 | I2C SSD1306 |
| 步进电机 IN1-4 | PA0~PA3 | 28BYJ-48 |
| 压力传感器 | PA4 (ADC) | 模拟压力信号 |
| 电池电量 | PA5 (ADC) | 电池电压检测 |
| 按键 KEY1-3 | PB0, PA6, PA7 | 确认/+/− |
| 蜂鸣器/LED | PB3/PB4 | 报警输出 |
| EEPROM | I2C1 | AT24C02 |

### 文件结构

```
project2/
└── code/
    ├── main.c                # 主程序 + 状态机逻辑
    ├── Main.uvprojx          # Keil 工程文件
    ├── Src/                  # 外设驱动
    ├── SYSTEM/               # 系统函数
    ├── 功能说明文档.txt        # 显示与按键详细说明
    ├── 完整操作手册.txt        # 操作流程手册
    └── 工作原理详解.txt        # 工作原理说明
```

---

## project3 — 温湿度显示系统

### 功能说明

双 STM32F103C8 节点组成的温湿度采集显示系统。一个节点作为 SHT21 传感器采集端，通过 UART 发送数据；另一个节点作为 OLED 显示端，接收并显示温湿度信息。

### 主要特性

- **双节点架构**：传感器节点 + 显示节点，通过 UART 通信
- **SHT21 传感器**：软件 I2C 驱动，采集温度 + 湿度
- **OLED 显示**：I2C SSD1306，实时显示温度和湿度值
- **UART 通信**：USART3, 9600bps，自定义协议 `T:xx.x H:yy.y\r\n`
- **Proteus 仿真**：支持双 STM32 联合仿真

### 硬件引脚

**传感器节点 (SHT21)：**

| 外设 | 引脚 | 说明 |
|------|------|------|
| SHT21 SCL/SDA | PB0/PB1 | 软件 I2C |
| UART TX | PB10 | USART3, 9600bps |

**显示节点 (OLED)：**

| 外设 | 引脚 | 说明 |
|------|------|------|
| OLED SCL/SDA | PB12/PB13 | I2C SSD1306 |
| UART RX | PB11 | USART3, 9600bps |

### 文件结构

```
project3/
├── code/
│   ├── oled_display/         # 显示节点（接收端）
│   │   ├── main.c
│   │   └── Main.uvprojx
│   └── SHT21/                # 传感器节点（采集端）
│       ├── main.c
│       └── Main.uvprojx
└── proteus/
    └── 温湿度显示.pdsprj      # Proteus 仿真文件
```

---

## 开发环境

- **MCU**：STM32F103C8（64KB Flash, 20KB SRAM）
- **IDE**：Keil MDK V5
- **仿真**：Proteus 8.x
- **编译器**：ARMCC V5 / ARMClang V6
- **调试接口**：SWD / JTAG

## 快速开始

1. 安装 Keil MDK V5 和 STM32F1 系列 Pack
2. 用 Keil 打开对应项目的 `Main.uvprojx`
3. 编译生成 `.hex` 文件
4. 在 Proteus 中打开仿真文件，加载 hex 进行仿真
5. 或烧录至实际 STM32F103C8 最小系统板运行

## 相关文档

- [project1 程序功能说明](./project1/程序功能说明.md)
- [project2 显示与按键功能说明](./project2/code/功能说明文档.txt)
- [project2 完整操作手册](./project2/code/完整操作手册.txt)
- [project2 工作原理详解](./project2/code/工作原理详解.txt)
