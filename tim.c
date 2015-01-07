#include "tim.h"

void tim_init(void){

  GPIO_InitTypeDef GPIO_InitStructure; 
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);	
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE);

/* TIM inputs */
	GPIO_InitStructure.GPIO_Pin = TIM2_CH1  ;                               
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_Init(TIM2_GPIO, &GPIO_InitStructure);

  GPIO_PinAFConfig(TIM2_GPIO, GPIO_PinSource1, GPIO_AF_TIM2);
	
// 	GPIO_InitStructure.GPIO_Pin = TIM2_PIN5  ;                               
//   //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
//   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//   GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
//   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
//   GPIO_Init(GPIOA, &GPIO_InitStructure);
//   GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2);
	
	
}

void Init_tim_cnt(void){
	TIM_TimeBaseInitTypeDef TIM_InitBaseStructure;
 	TIM_ICInitTypeDef TIM_ICInitStruct;
  //NVIC_InitTypeDef NVIC_InitStructure; //create NVIC structure
	
	TIM_TimeBaseStructInit(&TIM_InitBaseStructure);
	TIM_InitBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInit(TIM2, &TIM_InitBaseStructure);
	
  TIM_ICStructInit(&TIM_ICInitStruct);
  //TIM_ICInitStruct.TIM_Channel = TIM_Channel_1;
  TIM_ICInitStruct.TIM_Channel = TIM_Channel_2;
  //TIM_ICInit(TIM2, &TIM_ICInitStruct);
	TIM_PWMIConfig(TIM2, &TIM_ICInitStruct); 

	TIM_SelectInputTrigger(TIM2, TIM_TS_TI2FP2);
	TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIM2, TIM_MasterSlaveMode_Enable);

  TIM_ITConfig(TIM2, TIM_IT_CC2, ENABLE);
	
//   NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
//   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//   NVIC_Init(&NVIC_InitStructure);	
	
	TIM_Cmd(TIM2, ENABLE);
	
	NVIC_EnableIRQ(TIM2_IRQn);	
}

