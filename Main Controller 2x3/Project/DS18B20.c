#include <string.h>
#include "stm32f10x.h"
#include "DS18B20.h"
#include "stm32f10x_conf.h"


#define	 DQ0 GPIO_ResetBits(GPIO_PORT , GPIO_PIN_DS)	  //DS18B20���ݴ���
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
	 
    DS18B20_GPIO_Out();     // DQΪ��� 
	DQ1;                   //DQ��λ
    delayus(50);            //������ʱ70us    

    DQ0;      //��DQ����  
    delayus(480);           //��ȷ��ʱ780us ���� 480us
    DQ1;       //��������
    
    DS18B20_GPIO_In();       //��DQΪ����
	temp=GPIO_ReadInputDataBit(GPIO_PORT,GPIO_PIN_DS); //��DQֵ
    delayus(50);            //������ʱ
   
    if(temp==0)      //���=0���ʼ���ɹ� =1���ʼ��ʧ��
     presence = 1;
    else  
	 presence = 0;
          
    delayus(430);           //  ��ʱ470us
    DS18B20_GPIO_Out();      //��DQΪ��� 
    DQ1;      //�ͷ����� 
     
    return(presence);    //�����źţ�0=presence,1= no presence
}


//*********************************************************************

void WriteOneChar(u8 dat)
{
  	u8 i = 0;

  	DS18B20_GPIO_Out();      //��PD0Ϊ���
	for (i = 8; i > 0; i--)	 //дһ�ֽ�
  	{
		DQ0;
    	if(dat&0x01)
     		DQ1;      //д"1" 
		else     
	 		DQ0;     //д"0"

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
      	DQ0;      //����Ϊ�͵�ƽ
      	dat >>= 1;
    
      	DQ1;       //����Ϊ�ߵ�ƽ(�ͷ�����)   
      	DS18B20_GPIO_In();       //��DQΪ����
        
      	if(GPIO_ReadInputDataBit(GPIO_PORT,GPIO_PIN_DS))	//��DQ
      		dat |= 0x80;

      	delayus(70);
		DS18B20_GPIO_Out();	   //��DQΪ����
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
     WriteOneChar(0xCC);  // ����������кŵĲ���
     WriteOneChar(0x44);  // �����¶�ת��

     Init_DS18B20();
     WriteOneChar(0xCC);  //����������кŵĲ���
     WriteOneChar(0xBE);  //��ȡ�¶ȼĴ���
     
     tem0 = ReadOneChar();   //�¶ȵ�8λ
     tem1 = ReadOneChar();   //�¶ȸ�8λ

    tem0 =  (((tem0&0xf0)>>4)|((tem1&0x0f)<<4))-1;	
    }
	return tem0 ;
}


	 


