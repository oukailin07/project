#include "12864.h"
#include "delay.h"
 
static	u8 INITIAL_COLUNM=0;
 
void LCD_FUNCTION_Config(void)
{
		GPIO_InitTypeDef LCD_Struct;
		RCC_APB2PeriphClockCmd(LCD12864_Periph,ENABLE);
		LCD_Struct.GPIO_Mode = GPIO_Mode_Out_PP;
		LCD_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	
		LCD_Struct.GPIO_Pin = LCD12864_RES_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
	
		LCD_Struct.GPIO_Pin = LCD12864_E_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
	
	  LCD_Struct.GPIO_Pin = LCD12864_RS_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
	
		LCD_Struct.GPIO_Pin = LCD12864_RW_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
	
		LCD_Struct.GPIO_Pin = LCD12864_CS1_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
	
		LCD_Struct.GPIO_Pin = LCD12864_CS2_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);
}  
 
void LCD_GPIO_Config(void)
{
	  GPIO_InitTypeDef LCD_Struct;
		RCC_APB2PeriphClockCmd(LCD12864_Periph,ENABLE);
		LCD_Struct.GPIO_Mode = GPIO_Mode_Out_OD;
		LCD_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	  LCD_Struct.GPIO_Pin = LCD12864_DATA_GPIO;
		GPIO_Init(LCD12864_GPIO,&LCD_Struct);	
}
 
void LCD_Init(void)        //lcd初始化
{
	LCD_GPIO_Config();
	LCD_FUNCTION_Config();
	LCD12864_RES_0;
	delay_ms(10);
	LCD12864_RES_1;
	delay_ms(50);
	LCD_CMD_DIS(ON);
	LCD_INITIAL_COLUNM(INITIAL_COLUNM);
	
}
 
void LCD_GPIO_MASK(u8 data)       //数据掩码包装 
{
	u16 mask;
	mask = GPIO_ReadOutputData(LCD12864_GPIO) & 0XFF00 ; 
	
	mask = mask | data ;
	GPIO_Write(LCD12864_GPIO,mask);
}
 
void LCD_CMD_BUSY(void)    //忙检测
{
	LCD12864_RS_CMD;
	LCD12864_RW_R;
	LCD12864_E_1;
	LCD_GPIO_MASK(0xFF);
	while(1)
	{
		LCD12864_E_0;
		delay_us(2);
		LCD12864_E_1;
		if(!(GPIO_ReadInputData(LCD12864_GPIO)&CMD_BUSY))
			break;
	}
	LCD12864_E_0;
}
 
//parameter: ON/OFF
void LCD_CMD_DIS(u8 Switch)
{
	LCD_CMD_BUSY();
	LCD12864_RS_CMD;
	LCD12864_RW_W;
	LCD12864_E_1;
	if(Switch)
		LCD_GPIO_MASK(CMD_DIS_ON);
	else
		LCD_GPIO_MASK(CMD_DIS_OFF);
	delay_us(2);
	LCD12864_E_0;
}
 
 
void LCD_DIS_SELECT(u8 select)   //0全灭 1左屏 2右屏 3全屏
{
	select%=4;
	switch(select)
	{
		case DIS_RIGHT:
			LCD12864_CS2_ON;
			LCD12864_CS1_OFF;
			break;
		case DIS_LEFT:
			LCD12864_CS2_OFF;
			LCD12864_CS1_ON;
			break;
		case DIS_ALL:
			LCD12864_CS2_ON;
			LCD12864_CS1_ON;
			break;
		case DIS_NONE:
			LCD12864_CS2_OFF;
			LCD12864_CS1_OFF;
			break;
	}
}
 
//指令起始行
//parameter: 0-63
void LCD_INITIAL_COLUNM(u8 Colunm)
{
	Colunm%=64;
	Colunm = 64 - Colunm;
	LCD_CMD_BUSY();
	LCD12864_RS_CMD;
	LCD12864_RW_W;
	LCD12864_E_1;
	Colunm = CMD_COLUNM|Colunm;
	LCD_GPIO_MASK(Colunm);
	delay_us(2);
	LCD12864_E_0;
}
 
//指令写入数据显示列
//parameter: 0-128
void LCD_CMD_Line(u8 Line)
{
	Line %= 128; 
	if(Line<64)
	{
		LCD_DIS_SELECT(DIS_LEFT);
	}
	else
	{
		LCD_DIS_SELECT(DIS_RIGHT);
	}
	LCD_CMD_BUSY();
	LCD12864_RS_CMD;
	LCD12864_RW_W;
	LCD12864_E_1;
	Line = CMD_LINE|Line;
	LCD_GPIO_MASK(Line);
	delay_us(2);
	LCD12864_E_0;
}
 
//指令写入数据显示页
//parameter: 0-7
void LCD_CMD_PAGE(u8 Page)
{
	Page%=8;
	LCD_CMD_BUSY();
	LCD12864_RS_CMD;
	LCD12864_RW_W;
	LCD12864_E_1;
	Page = CMD_PAGE|Page;
	LCD_GPIO_MASK(Page);
	delay_us(2);
	LCD12864_E_0;
}
 
//写入数据0-7行
void LCD_CMD_COLUNM(u8 Colunm)
{
	Colunm%=8;
	Colunm=Colunm-INITIAL_COLUNM/8;
	LCD_CMD_PAGE(Colunm);
}
 
//计算数据长度
u8 LCD_NUMS_SIZE(int num)
{
	u8 size=0;
	if(num==0)
	{
		size=1;
	}else
	{
		while(num)
		{
			size++;
			num=num/10;
		}
	}
	return size;
}
 
int LCD_POW(u8 mi)  //计算10的次方
{
	int di=1;
	while(mi--)
	{
		di=di*10;
	}
	return di;
}
 
void LCD_DATA_WRIRE(u8 data)   //写一位数据
{
	LCD_CMD_BUSY();
	LCD12864_RS_DATA;
	LCD12864_RW_W;
	LCD12864_E_1;
	LCD_GPIO_MASK(data);
	delay_us(2);
	LCD12864_E_0;
}
 
//显示一个字符 支持小、中尺寸  反侧显示
//parameter: 行0-7 列0-127 data数据 size尺寸SMALL MIDDLE
void LCD_DIS_Char(u8 row,u8 col,u8* data,int size)
{
	u8 length=0;
	u8 line;
	u8 i;
	row%=8;
	col%=128;
	LCD_CMD_Line(col);
	line=col-1;
	if(size==SMALL)
	{
		LCD_CMD_COLUNM(row);
		while(data[length])
		{
			for(i=0;i<6;i++)
			{
				line+=1;
				if(line==64)
				{
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row);
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row);
				}
				LCD_DATA_WRIRE(CHAR_SMALL[data[length]-32][i]);
			}	
			length++;
		}
	}else if(size==MIDDLE)
	{
		while(data[length])
		{
			LCD_CMD_COLUNM(row);
			for(i=0;i<16;i++)
			{
				line+=1;
				if(i==8)
				{
					if(line<8)
						line=128-(8-line);
					else
						line-=8;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+1);
				}
				else if(line==64)
				{
					LCD_CMD_Line(line);
					if(i<8)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=8)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					if(i<8)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=8)
					{
						LCD_CMD_COLUNM(row+1);
					}	
				}
				LCD_DATA_WRIRE(CHAR_MIDDLE[data[length]-32][i]);
			}
			length++;
		}
	}
}
 
//写数字 支持小、中尺寸 数字可支持小数  小数位数只包括2位  反侧显示
//parameter: 行0-7 列0-127 data数据 size尺寸SMALL MIDDLE
void LCD_DIS_Number(u8 row,u8 col,float num,int size)
{
	u8 line;
	u8 length;
	u8 i;
	u8 flag=0;
	float number=0;
	row%=8;
	col%=128;
	length=LCD_NUMS_SIZE((int)num);
	LCD_CMD_Line(col);
	line=col-1;
	if(num-(int)num>0)
	{
		flag=1;
		number=num;
	}
	if(size==SMALL)
	{
		LCD_CMD_COLUNM(row);//1行
		while(length--)//6位
		{
			for(i=0;i<6;i++)
			{
				line+=1;
				if(line==64)
				{
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row);
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
				}
				if(flag==2)
				{
					LCD_DATA_WRIRE(CHAR_SMALL[14][i]);
				}else
					LCD_DATA_WRIRE(CHAR_SMALL[(int)num/LCD_POW(length)+16][i]);	
			}
			if(flag==2)
			{
				flag=0;
				continue;
			}
			num=(int)num%LCD_POW(length);	
			if(flag==1&&length==0)
			{
				flag=2;
				length=3;
				num=(int)((number- (int)number)*100);
			}
		}
	}else if(size==MIDDLE)//中尺寸
	{
		while(length--)
		{
			LCD_CMD_COLUNM(row);
			for(i=0;i<16;i++)
			{
				line+=1;
				if(i==8)
				{
					if(line<8)
						line=128-(8-line);
					else
						line-=8;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+1);
				}
				else if(line==64)
				{
					LCD_CMD_Line(line);
					if(i<8)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=8)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					if(i<8)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=8)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}
				if(flag==2)
				{
					LCD_DATA_WRIRE(CHAR_MIDDLE[14][i]);
				}else				
					LCD_DATA_WRIRE(CHAR_MIDDLE[(int)num/LCD_POW(length)+16][i]);
			}
			if(flag==2)
			{
				flag=0;
				continue;
			}
			num=(int)num%LCD_POW(length);
			if(flag==1&&length==0)
			{
				flag=2;
				length=3;
				num=(int)((number-(int)number)*100);
			}
		}
	}else if(size==LONG)//大尺寸
	{
		while(length--)
		{
			LCD_CMD_COLUNM(row);
			for(i=0;i<36;i++)
			{
				line+=1;
				if(i==12)
				{
					if(line<12)
						line=128-(12-line);
					else
						line-=12;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+1);
				}else if(i==24)
				{
					if(line<12)
						line=128-(12-line);
					else
						line-=12;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+2);
				}
				else if(line==64)
				{
					LCD_CMD_Line(line);
					if(i<12)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=12&&i<24)
					{
						LCD_CMD_COLUNM(row+1);
					}else if(i>=24)
					{
						LCD_CMD_COLUNM(row+2);
					}
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					if(i<12)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=12&&i<24)
					{
						LCD_CMD_COLUNM(row+1);
					}else if(i>=24)
					{
						LCD_CMD_COLUNM(row+2);
					}
				}	
				if(flag==2)
				{
					LCD_DATA_WRIRE(NUMBER_LONG[10][i]);
				}else
					LCD_DATA_WRIRE(NUMBER_LONG[(int)num/LCD_POW(length)][i]);
			}
			if(flag==2)
			{
				flag=0;
				continue;
			}
			num=(int)num%LCD_POW(length);
			if(flag==1&&length==0)
			{
				flag=2;
				length=3;
				num=(int)((number-(int)number)*100);
			}
		}
	}
}
 
//写汉字   支持中、大尺寸  data为库
void LCD_DIS_WORD(u8 row,u8 col,u8* data,int size)
{
	u8 length=0;
	u8 i;
	u8 line;
	row%=8;
	col%=128;
	LCD_CMD_Line(col);
	line=col-1;
	if(size==MIDDLE)
	{
		while(data[length]!=0)
		{
			LCD_CMD_COLUNM(row);
			for(i=0;i<20;i++)
			{
				line+=1;
				if(i==10)
				{
					if(line<10)
						line=128-(10-line);
					else
						line-=10;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+1);
				}
				else if(line==64)
				{
					LCD_CMD_Line(line);
					if(i<10)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=10)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					if(i<10)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=10)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}
				LCD_DATA_WRIRE(WORD_MIDDLE[data[length]-1][i]);
			}
			length++;
		}
	}else if(size==LONG)
	{
		while(data[length]!=0)
		{
			LCD_CMD_COLUNM(row);
			for(i=0;i<32;i++)
			{
				line+=1;
				if(i==16)
				{
					if(line<16)
						line=128-(16-line);
					else
						line-=16;
					LCD_CMD_Line(line);
					LCD_CMD_COLUNM(row+1);
				}
				else if(line==64)
				{
					LCD_CMD_Line(line);
					if(i<16)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=16)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}else if(line==128)
				{
					line=0;
					LCD_CMD_Line(line);
					if(i<16)
					{
						LCD_CMD_COLUNM(row);
					}else if(i>=16)
					{
						LCD_CMD_COLUNM(row+1);
					}
				}
				LCD_DATA_WRIRE(WORD_LONG[data[length]-1][i]);
			}
			length++;
		}
	}
}
 
//图片显示 尺寸64*128
void LCD_DIS_PICTURE(void)
{
	u8 row;
	u8 col;
	for(row=0;row<8;row++)
	{
		LCD_CMD_Line(0);
		LCD_CMD_COLUNM(row);
		for(col=0;col<128;col++)
		{
			if(col==64){
				LCD_CMD_Line(64);
				LCD_CMD_COLUNM(row);
			}
			if(col<64)
			{
				LCD_DATA_WRIRE(BMP_TEST[row*2][col]);
			}else if(col>=64)
			{
				LCD_DATA_WRIRE(BMP_TEST[row*2+1][col-64]);
			}
		}
		
	}
}
//清除屏幕单独一行
void LCD_CLEAR_COL(u8 row)
{
	u8 j;
	LCD_DIS_SELECT(DIS_ALL);
	LCD_CMD_COLUNM(row);
	for(j=0;j<128;j++)
	{
		LCD_DATA_WRIRE(0xFF);
	}
}
//清除全屏
void LCD_CLEAR_ALL(void)
{
	u8 i;
	u8 j;
	LCD_DIS_SELECT(DIS_ALL);
	for(i=0;i<8;i++)
	{
		LCD_CMD_COLUNM(i);
		for(j=0;j<128;j++)
		{
			LCD_DATA_WRIRE(0xFF);
		}
	}
}
 
 