#include<reg51.h>
#include<intrins.h>
#define uchar unsigned char
#define uint unsigned int
sbit wr=P2^0;
sbit rd=P2^1;
sbit ce=P2^2;
sbit cd=P2^3;
sbit rst=P2^4;

void delayus(uchar x)
{
 while(--x)
 {
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
  _nop_();
 }
}


uchar read_status()
{
 uchar status;
 rd=0;
 wr=1;
 ce=0;
 cd=1;
 status=P1;
 return status;
}

void status_check()
{
 uchar s;
 while((s&0x03)!=0x03)
    s=read_status();
}


void data_write(uint data1)
{
 rd=1;
 cd=0;
 ce=0;
 wr=0;
 P0=data1;
 delayus(10);
 wr=1;
 ce=1;
 cd=1;
}

void command_write(uchar command)
{
 rd=1;
 cd=1;
 wr=0;
 ce=0;
 P0=command;
 delayus(10);
 wr=1;
 ce=1;
 cd=0; 
}

void command_sending_with_one_data(uchar data1,uchar command)
{
 status_check();
 data_write(data1);
 status_check();
 command_write(command); 
}

void command_sending_with_two_data(uint data1,uint data2,uchar command)
{
 status_check();
 data_write(data1);
 status_check();
 data_write(data2);
 status_check();
 command_write(command);   
}

void display_HZ(uchar x,uint y,uchar *hz)////x 0-3      y 0-7
{
 uchar i;
 uint j=0;
 uint k=x;
 for(i=0;i<16;i++)
 {
  if(j>=16)x=k+(j/16);
  command_sending_with_two_data((((j)<<4)|(y*2)),x,0x24);//地址指针设置  低地址，高地址，命令
  command_sending_with_one_data(hz[j++],0xc0);
  command_sending_with_one_data(hz[j++],0xc0);

 }
}
void display_num(uchar x,uint y,uchar *hz)////x 0-3      y 0-7
{
 uchar i;
 uint j=0;
 uint k=x;
 for(i=0;i<16;i++)
 {
  if(j>=8)x=k+(j/8);
  command_sending_with_two_data((((j*2)<<4)|(y)),x,0x24);//地址指针设置  低地址，高地址，命令
  command_sending_with_one_data(hz[j++],0xc0);

 }
}

void init_12864()
{
 wr=1;
 rd=1;
 ce=1;
 cd=1;
 rst=1;
 status_check();
 command_sending_with_two_data(0x00,0x00,0x21); //光标指针设置，本程序不设置偏移寄存器

 status_check();                                //地址指针在开始写字的时候才进行设置
 command_sending_with_two_data(0x00,0x00,0x42); //设置图形显示首地址
 status_check();
 command_sending_with_two_data(32,0x20,0x43);   //设置图形区域大小
 status_check();
 command_write(0x80);      //模式设置
 status_check();
 command_write(0x98);      //显示设置
 status_check();
 command_write(0xa0);      //光标设置
}

 