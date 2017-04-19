#include <string.h>
#include "stm32f10x.h"
#include "DS18B20.h"
#include "stm32f10x_conf.h"


#define	 DQ0 GPIO_ResetBits(GPIO_PORT , GPIO_PIN_DS)	  //DS18B20数据串口
#define	 DQ1 GPIO_SetBits(GPIO_PORT , GPIO_PIN_DS)

u8  presence;


//*********************************************************************
void delayus(u16 nCount)
{
  u16 TIMCounter = nCount;
  TIM_Cmd(TIM2, ENABLE);
  TIM_SetCounter(TIM2, TIMCounter);
  while (TIMCounter)
  {
    TIMCounter = TIM_GetCounter(TIM2);
  }
  TIM_Cmd(TIM2, DISABLE);
}
//*********************************************************************
void DS18B20_GPIO_In(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	  
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_PORT , ENABLE); 

	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_DS ;  
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
  	GPIO_Init(GPIO_PORT, &GPIO_InitStructure);				 
}
//*********************************************************************
void DS18B20_GPIO_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	  
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIO_PORT , ENABLE); 

	GPIO_InitStructure.GPIO_Pin = GPIO_PIN_DS; 
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  	GPIO_Init(GPIO_PORT, &GPIO_InitStructure);				 
}

//*********************************************************************

int Init_DS18B20(void)
{   
	u8 temp;
	 
    DS18B20_GPIO_Out();     // DQ为输出 
	DQ1;                   //DQ复位
    delayus(50);            //稍做延时70us    

    DQ0;      //将DQ拉低  
    delayus(480);           //精确延时780us 大于 480us
    DQ1;       //拉高总线
    
    DS18B20_GPIO_In();       //置DQ为输入
	temp=GPIO_ReadInputDataBit(GPIO_PORT,GPIO_PIN_DS); //读DQ值
    delayus(50);            //稍做延时
   
    if(temp==0)      //如果=0则初始化成功 =1则初始化失败
     presence = 1;
    else  
	 presence = 0;
          
    delayus(430);           //  延时470us
    DS18B20_GPIO_Out();      //置DQ为输出 
    DQ1;      //释放总线 
     
    return(presence);    //返回信号，0=presence,1= no presence
}


//*********************************************************************

void WriteOneChar(u8 dat)
{
  	u8 i = 0;

  	DS18B20_GPIO_Out();      //置PD0为输出
	for (i = 8; i > 0; i--)	 //写一字节
  	{
		DQ0;
    	if(dat&0x01)
     		DQ1;      //写"1" 
		else     
	 		DQ0;     //写"0"

    	delayus(60);
    	DQ1;        
    	dat>>=1;
  }
}

//*********************************************************************

int ReadOneChar(void)
{
	u8 i = 0;
    u8 dat = 0;

    for (i = 8; i > 0; i--)
    {
      	DQ0;      //总线为低电平
      	dat >>= 1;
    
      	DQ1;       //总线为高电平(释放总线)   
      	DS18B20_GPIO_In();       //置DQ为输入
        
      	if(GPIO_ReadInputDataBit(GPIO_PORT,GPIO_PIN_DS))	//读DQ
      		dat |= 0x80;

      	delayus(70);
		DS18B20_GPIO_Out();	   //置DQ为输入
      	DQ1;  
    }
    return (dat);
}
 
 
//*********************************************************************

unsigned char Read_Temperature()
{
unsigned char tem0 = 0, tem1 ;
   Init_DS18B20();
   if(presence==0)  
   {
     WriteOneChar(0xCC);  // 跳过读序号列号的操作
     WriteOneChar(0x44);  // 启动温度转换

     Init_DS18B20();
     WriteOneChar(0xCC);  //跳过读序号列号的操作
     WriteOneChar(0xBE);  //读取温度寄存器
     
     tem0 = ReadOneChar();   //温度低8位
     tem1 = ReadOneChar();   //温度高8位

    tem0 =  (((tem0&0xf0)>>4)|((tem1&0x0f)<<4))-1;	
    }
	return tem0 ;
}


	 


