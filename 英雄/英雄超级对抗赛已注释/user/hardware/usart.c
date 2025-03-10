#include "main.h"

/*
  串口一 通过DMA2进行遥控器数据接收并解析
	
	串口六 通过DMA2进行裁判系统数据接收，连接方式是裁判系统电源管理模块有串口通过接线就可以接收到数据，
	在通过crc校验解析数据并在 judge.c的结构体里存储，便于读取裁判系统的数据

 
*/


//通过DMA2 将传输遥感数据值保存
volatile unsigned char sbus_rx_buffer[25]; 
//结构体嵌套结构体 用来存放处理后的遥感数据

//遥控器串口端口PB7,波特率10000，
volatile  RC_Ctl_t RC_Ctl;
//接收到裁判系统的数据
uint8_t  referee_Buffer[ 200 ] ;
int Usart6_Clean_IDLE_Flag = 0;


void USART1_Configuration(void)
{
    USART_InitTypeDef usart1;
	  GPIO_InitTypeDef  gpio;
    NVIC_InitTypeDef  nvic;
		DMA_InitTypeDef dma;
	
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_DMA2,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
    
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_USART1); 
	
    gpio.GPIO_Pin = GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB,&gpio);
		
		USART_DeInit(USART1);
    usart1.USART_BaudRate = 100000;
    usart1.USART_WordLength = USART_WordLength_8b;
    usart1.USART_StopBits = USART_StopBits_1;
		usart1.USART_Parity = USART_Parity_Even;
		usart1.USART_Mode =USART_Mode_Rx;
    usart1.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1,&usart1);

    USART_ITConfig(USART1,USART_IT_RXNE,DISABLE);
    USART_Cmd(USART1,ENABLE);
		
		USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);
   
	  nvic.NVIC_IRQChannel = DMA2_Stream5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
		
		DMA_Cmd(DMA2_Stream5, DISABLE);
    while (DMA2_Stream5->CR & DMA_SxCR_EN);
		DMA_DeInit(DMA2_Stream5);
    dma.DMA_Channel= DMA_Channel_4;//通道选择
    dma.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);//DMA外设地址
    dma.DMA_Memory0BaseAddr = (uint32_t)sbus_rx_buffer;//DMA存储器地址就是遥感数据存放的地方
    dma.DMA_DIR = DMA_DIR_PeripheralToMemory;//存储器到外设模式
    dma.DMA_BufferSize = 18;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设+非增量模式
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器增量模式
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//存储器数据长度
    dma.DMA_Mode = DMA_Mode_Circular;//使用模式
    dma.DMA_Priority = DMA_Priority_VeryHigh;//最高优先级
    dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    dma.DMA_MemoryBurst = DMA_Mode_Normal;
    dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//单词传输
    DMA_Init(DMA2_Stream5,&dma);

		DMA_ITConfig(DMA2_Stream5,DMA_IT_TC,ENABLE);
    DMA_Cmd(DMA2_Stream5,ENABLE);
		
		
}

void DMA2_Stream5_IRQHandler(void)
{             
    if(DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5))
    {
			
        DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);
        DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_TCIF5);
			
				/*************/
				//串口打印遥控器数据
				if(USE_debug==3)
				{
					u8 i=0;
					for(i=0;i<18;i++)
					{
						USART6_TX_Byte(sbus_rx_buffer[i]);
					}
					USART6_TX_Byte(0XDD);
          USART6_TX_Byte(0XDD);
				}
				/*************/

			

		 RC_Ctl.rc.ch[0] = (sbus_rx_buffer[0] | (sbus_rx_buffer[1] << 8)) & 0x07ff;        //!< Channel 0
     RC_Ctl.rc.ch[1] = ((sbus_rx_buffer[1] >> 3) | (sbus_rx_buffer[2] << 5)) & 0x07ff; //!< Channel 1
     RC_Ctl.rc.ch[2] = ((sbus_rx_buffer[2] >> 6) | (sbus_rx_buffer[3] << 2) |          //!< Channel 2
                         (sbus_rx_buffer[4] << 10)) &
                        0x07ff;
     RC_Ctl.rc.ch[3] = ((sbus_rx_buffer[4] >> 1) | (sbus_rx_buffer[5] << 7)) & 0x07ff; //!< Channel 3
				
    RC_Ctl.rc.s1 = ((sbus_rx_buffer[5] >> 4) & 0x0003);                  //!< Switch left
     RC_Ctl.rc.s2 = ((sbus_rx_buffer[5] >> 4) & 0x000C) >> 2;                       //!< Switch right
    RC_Ctl.mouse.x = sbus_rx_buffer[6] | (sbus_rx_buffer[7] << 8);                    //!< Mouse X axis
    RC_Ctl.mouse.y = sbus_rx_buffer[8] | (sbus_rx_buffer[9] << 8);                    //!< Mouse Y axis
    RC_Ctl.mouse.z = sbus_rx_buffer[10] | (sbus_rx_buffer[11] << 8);                  //!< Mouse Z axis
    RC_Ctl.mouse.press_l = sbus_rx_buffer[12];                                  //!< Mouse Left Is Press ?
    RC_Ctl.mouse.press_r = sbus_rx_buffer[13];                                  //!< Mouse Right Is Press ?
    RC_Ctl.key.v = sbus_rx_buffer[14] | (sbus_rx_buffer[15] << 8);                    //!< KeyBoard value
    RC_Ctl.rc.ch[4] = sbus_rx_buffer[16] | (sbus_rx_buffer[17] << 8);                 //NULL

    RC_Ctl.rc.ch[0] -= RC_CH_VALUE_OFFSET;
    RC_Ctl.rc.ch[1] -= RC_CH_VALUE_OFFSET;
    RC_Ctl.rc.ch[2] -= RC_CH_VALUE_OFFSET;
      RC_Ctl.rc.ch[3] -= RC_CH_VALUE_OFFSET;
      RC_Ctl.rc.ch[4] -= RC_CH_VALUE_OFFSET;
    }
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
        
    }
}

//裁判系统的数据
void USART6_Configuration(u32 bound){
		//GPIO端口设置
		GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		DMA_InitTypeDef dma;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE); //使能GPIOG时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6,ENABLE);//使能USART6时钟

		//串口6对应引脚复用映射
		GPIO_PinAFConfig(GPIOG,GPIO_PinSource9,GPIO_AF_USART6); //GPIOA9复用为USART6
		GPIO_PinAFConfig(GPIOG,GPIO_PinSource14,GPIO_AF_USART6); //GPIOA10复用为USART6

		//USART6端口配置
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_14; //GPIOG9与GPIOG14
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;        //速度50MHz
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
		GPIO_Init(GPIOG,&GPIO_InitStructure); //初始化GPIOG9与GPIOG14

		//USART6 初始化设置
		USART_InitStructure.USART_BaudRate = 115200;//波特率设置
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx ;        //收发模式
		USART_Init(USART6, &USART_InitStructure); //初始化串口6

		USART_Cmd(USART6, ENABLE);  //使能串口6 
	  USART_DMACmd(USART6,USART_DMAReq_Rx,ENABLE);
		USART_ClearFlag(USART6, USART_FLAG_TC);

		USART_ITConfig(USART6, USART_IT_RXNE, DISABLE);//开启相关中断

		//USART6 NVIC 配置
		NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream2_IRQn;//串口6中断通道
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
		NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;                //子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                        //IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);        //根据指定的参数初始化VIC寄存器、
	  
		DMA_Cmd(DMA2_Stream2, DISABLE);
    while (DMA2_Stream2->CR & DMA_SxCR_EN);
		DMA_DeInit(DMA2_Stream2);
    dma.DMA_Channel= DMA_Channel_5;//通道选择
    dma.DMA_PeripheralBaseAddr = (uint32_t)&(USART6->DR);//DMA外设地址
    dma.DMA_Memory0BaseAddr = (uint32_t)referee_Buffer;//DMA存储器地址就是遥感数据存放的地方
    dma.DMA_DIR = DMA_DIR_PeripheralToMemory;//存储器到外设模式
    dma.DMA_BufferSize = 100;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设+非增量模式
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;//存储器增量模式
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//存储器数据长度
    dma.DMA_Mode = DMA_Mode_Circular;//使用模式
    dma.DMA_Priority = DMA_Priority_VeryHigh;//最高优先级
    dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    dma.DMA_MemoryBurst = DMA_Mode_Normal;
    dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//单词传输
    DMA_Init(DMA2_Stream2,&dma);

		DMA_ITConfig(DMA2_Stream2,DMA_IT_TC,ENABLE);
    DMA_Cmd(DMA2_Stream2,ENABLE);
        
}
void DMA2_Stream2_IRQHandler(void)
{             
    if(DMA_GetITStatus(DMA2_Stream2, DMA_IT_TCIF2))
    {
			
        DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);
        DMA_ClearITPendingBit(DMA2_Stream2, DMA_IT_TCIF2);
			  
			Judge_Read_Data(referee_Buffer);
			
			memset(referee_Buffer, 0, 200);
				//串口打印遥控器数据
				if(USE_debug==2)
				{
					u8 i=0;
					for(i=0;i<100;i++)
					{
						UART7_SendChar(referee_Buffer[i]);
					}

				}
			

    }
}

//取消ARM的半主机工作模式
#pragma import(__use_no_semihosting)                             
struct __FILE { 
    int handle; 
}; 

FILE __stdout;          
void _sys_exit(int x) 
{ 
    x = x; 
}


int fputc(int ch, FILE *f){      
    while((USART6->SR&0X40)==0);
    USART6->DR = (u8) ch;      
    return ch;
}

void USART6_TX_Byte(unsigned char data)   //发送
{
	USART_SendData(USART6, data);
	while(USART_GetFlagStatus(USART6,USART_FLAG_TXE) == RESET);
}


void USART6_IRQHandler(void)               //接收
{ 
//    u8 code;
    
    if(USART_GetITStatus(USART6, USART_IT_RXNE) == SET)
    {

    }
    USART_ClearITPendingBit(USART6,USART_IT_RXNE);
}




