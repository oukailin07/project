#include "sim900a.h"
#include "usart.h"		
#include "delay.h"	
#include "malloc.h"
#include "string.h"  

//////////////////////////////////////////////////////////////////////////////////	   
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F103开发板 
//ATK-SIM900A GSM/GPRS模块驱动	  
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/4/12
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//******************************************************************************** 
//无
/////////////////////////////////////////////////////////////////////////////////// 	
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//usmart支持部分
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART1_RX_STA;
//     1,清零USART1_RX_STA;
void sim_at_response(u8 mode)
{
	if(USART1_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART1_RX_BUF[USART1_RX_STA&0X7FFF]=0;//添加结束符
		printf("%s",USART1_RX_BUF);	//发送到串口
		if(mode)USART1_RX_STA=0;
	} 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//ATK-SIM900A 各项测试(拨号测试、短信测试、GPRS测试)共用代码

//sim900a发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* sim900a_check_cmd(u8 *str)
{
	char *strx=0;
	if(USART1_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART1_RX_BUF[USART1_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART1_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}
//向sim900a发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART1_RX_STA=0;
	if((u32)cmd<=0XFF)
	{
		while((USART1->SR&0X40)==0);//等待上一次数据发送完成  
		USART1->DR=(u32)cmd;
	}else printf("%s\r\n",cmd);//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(USART1_RX_STA&0X8000)//接收到期待的应答结果
			{
				if(sim900a_check_cmd(ack))break;//得到有效数据 
				USART1_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 

//int sim900_Warning(){
//	if(sim900a_send_cmd("AT","OK",100)==1) return 0;															//检测模块是否能够正常工作
//	 sim900a_send_cmd("AT+CSQ",0,100);																						//查询信号强度
//	if(sim900a_send_cmd("AT+CPIN?","READY",100)==1) return 0;											//查询是否检测到手机卡
//	if(sim900a_send_cmd("AT+COPS?","CHINAMOBILE",100)==1) return 0;								//查询模块是否注册到网络，只能使用中国移动电话卡
//	if(sim900a_send_cmd("AT+CSCS=\"GSM\"","OK",100)==1) return 0;									//
//	if(sim900a_send_cmd("AT+CMGF=1","OK",100)==1) return 0;												
//	if(sim900a_send_cmd("AT+CMGS=\"189xxxxxxxx\"",">",1000)==1) return 0;					//发送要发送电话号码 189xxxxxxxx 虚假号码
//	printf("pilferage warring ");																									//发送短信内容
// 	if(sim900a_send_cmd((u8*)0X1A,"+CMGS:",1000)==0) return 0;//发送结束符,等待发送完成(最长等待10秒钟,因为短信长了的话,等待时间会长一些)
//	return 1;
//}


int sim900_Warning(char *str){
	if(sim900a_send_cmd("AT",0,100)==1) return 0;							//
	 sim900a_send_cmd("AT+CSQ",0,100);
	if(sim900a_send_cmd("AT+CPIN?",0,100)==1) return 0;
	if(sim900a_send_cmd("AT+COPS?",0,100)==1) return 0;
	if(sim900a_send_cmd("AT+COPS?",0,100)==1) return 0;
	if(sim900a_send_cmd("AT+CSCS=\"GSM\"",0,100)==1) return 0;
	if(sim900a_send_cmd("AT+CMGF=1",0,100)==1) return 0;
	if(sim900a_send_cmd("AT+CMGS=\"189xxxxxxxx\"",0,1000)==1) return 0;
	sim900a_send_cmd(str,0,100);
 	if(sim900a_send_cmd((u8*)0X1A,0,1000)==0) return 0;//发送结束符,等待发送完成(最长等待10秒钟,因为短信长了的话,等待时间会长一些)
	return 1;
}














