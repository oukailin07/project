#ifndef __12864_H
#define	__12864_H
 
 
#include "stm32f10x.h"
 
#define LCD12864_GPIO     GPIOC
#define LCD12864_Periph   RCC_APB2Periph_GPIOC
#define LCD12864_E_GPIO   GPIO_Pin_8
#define LCD12864_RW_GPIO  GPIO_Pin_9
#define LCD12864_RS_GPIO  GPIO_Pin_10
#define LCD12864_CS2_GPIO GPIO_Pin_11
#define LCD12864_CS1_GPIO GPIO_Pin_12
#define LCD12864_RES_GPIO GPIO_Pin_13
#define LCD12864_DATA_GPIO     GPIO_Pin_0|GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6|GPIO_Pin_7
 
#define LCD12864_RES_0  GPIO_ResetBits(LCD12864_GPIO,LCD12864_RES_GPIO)
#define LCD12864_RES_1  GPIO_SetBits(LCD12864_GPIO,LCD12864_RES_GPIO)
 
#define LCD12864_E_0    GPIO_ResetBits(LCD12864_GPIO,LCD12864_E_GPIO)
#define LCD12864_E_1    GPIO_SetBits(LCD12864_GPIO,LCD12864_E_GPIO)
 
#define LCD12864_RW_W   GPIO_ResetBits(LCD12864_GPIO,LCD12864_RW_GPIO)
#define LCD12864_RW_R   GPIO_SetBits(LCD12864_GPIO,LCD12864_RW_GPIO)
 
#define LCD12864_RS_CMD  GPIO_ResetBits(LCD12864_GPIO,LCD12864_RS_GPIO)
#define LCD12864_RS_DATA GPIO_SetBits(LCD12864_GPIO,LCD12864_RS_GPIO)
 
#define LCD12864_CS1_ON   GPIO_ResetBits(LCD12864_GPIO,LCD12864_CS1_GPIO)
#define LCD12864_CS1_OFF  GPIO_SetBits(LCD12864_GPIO,LCD12864_CS1_GPIO)
 
#define LCD12864_CS2_ON  GPIO_ResetBits(LCD12864_GPIO,LCD12864_CS2_GPIO)
#define LCD12864_CS2_OFF GPIO_SetBits(LCD12864_GPIO,LCD12864_CS2_GPIO)
 
//开启或关闭
#define ON  1
#define OFF 0
 
//LCD命令
#define CMD_DIS_ON   0X3F
#define CMD_DIS_OFF  0X3E
#define CMD_BUSY     0X80
#define CMD_COLUNM   0XC0
#define CMD_LINE     0X40
#define CMD_PAGE     0XB8
 
//显示屏幕选择
#define DIS_NONE  0
#define DIS_LEFT  1
#define DIS_RIGHT 2
#define DIS_ALL   3
 
//尺寸
#define SMALL  1
#define MIDDLE 2
#define LONG   3
 
//方向选择
#define UP   1
#define DOWN 2
 
 
//内部处理函数
void LCD_CMD_DIS(u8 Switch);   //开关函数
void LCD_CMD_COLUNM(u8 Colunm);//指定初始行
void LCD_CMD_Line(u8 Line);    //写入列
void LCD_CMD_COLUNM(u8 Colunm); //写入页
void LCD_INITIAL_COLUNM(u8 Colunm);//初始行
void LCD_DATA_WRIRE(u8 data);  //写一位数据
void LCD_CMD_PAGE(u8 Page);    //初始页
 
//外部调用函数
void LCD_Init(void);           //初始化
void LCD_DIS_Char(u8 row,u8 col,u8* data,int size);//显示字符 
void LCD_MODE_ROLL(u8 path,u8 step,u8 direction,u16 delay);//滚动模式
void LCD_DIS_Number(u8 row,u8 col,float num,int size);//显示数字
void LCD_CLEAR_COL(u8 row);//清零一行
void LCD_CLEAR_ALL(void);//清屏
void LCD_DIS_WORD(u8 row,u8 col,u8* data,int size);//写字
void LCD_DIS_PICTURE(void);//显示罗小黑图片
#endif  
 