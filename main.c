#include "main.h"

static volatile uint32_t TimingDelay;
RCC_ClocksTypeDef RCC_Clocks;

volatile bool flag_ADCDMA_TransferComplete;

//uint32_t preasureAVG, altTempAVG;
//float preasure_V, altTemp_V; 

volatile uint16_t systick_ms = 0, toggle_ms = 0;

float humidity;

float temperature, temperatureP;

int32_t preasure;

volatile uint16_t dirty_cycle = 0, period = 0; // ;

volatile uint8_t mode = 0, first_time_in_mode = 1, flag_UserButton = 0;

void SysTick_Handler(void) {
    TimingDelay_Decrement();
}

void EXTI0_IRQHandler(void)
{
  /* set UserButton Flag */
	flag_UserButton = 1;
  EXTI_ClearITPendingBit(EXTI_Line0);
}

void TIM2_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
  {
    /* Даём знать, что обработали прерывание */
    TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    period = TIM_GetCapture1(TIM2);
    dirty_cycle = TIM_GetCapture2(TIM2);
	}
}

void DMA1_Channel1_IRQHandler    (void)
{
  DMA_ClearFlag(DMA1_IT_TC1);
  setADCDMA_TransferComplete();  /* set flag_ADCDMA_TransferComplete global flag */
}



int main(void){

	char strDisp[25]; uint8_t dhtbuf[5]; 
	uint8_t i2craw[5];
	uint16_t owraw;
	char rd;
	struct bmp085_type bmp085;
	ADC_Typedef ADC_RES;
	RTC_DateTypeDef RTCDateStr;
	RTC_TimeTypeDef RTCTimeStr;
	
	RCC_Configuration();
	
	button_init_irq();
	
	led_init();
	
	rtc_init();
	
  USART_open(USART1, 9600);   //for RS232
  USART_open(USART2, 115200); //for DS18B20
	
	configureDMA();
	
	adc_init();	
	
	I2C_LowLevel_Init (I2C2, 1000, 0xA0);
	
	BMP085_RawCalibData(bmp085.calib.raw);

	dac_init(DAC_Channel_1, DAC_OUT1);
	dac_set(DAC_Channel_1, 500);
	
	tim_init_pwmout();
	tim_init_cnt();

	sprintf(strDisp, "REDY!\n\r");		
	USART_DMA_send(USART1, strDisp, strlen(strDisp));

	while(1){
	
		if (flag_UserButton == 1){
			if (++mode == 3){mode = 0;}
			flag_UserButton = 0;
			first_time_in_mode = 1;
		}

// 		if (uint16_time_diff(systick_ms, toggle_ms) >= 500)
// 		{
// 			toggle_ms = systick_ms;
// 			if (stop==0) {GPIO_TOGGLE(LD_PORT,LD_BLUE);}	
// 		}		
									
 		if (uint16_time_diff(systick_ms, toggle_ms) >= 1000)
 		{
 			toggle_ms = systick_ms;

			GPIO_TOGGLE(LD_PORT,LD_BLUE);
			
			clearADCDMA_TransferComplete();
			acquireTemperatureData();

			while (!flag_ADCDMA_TransferComplete);
			
			processTempData(&ADC_RES);

			DHT11_RawRead(dhtbuf);		
			humidity = DHT22_Humidity(dhtbuf);
			temperature = DHT22_Temperature(dhtbuf);

			BMP085_RawTemperarure(i2craw);
			BMP085_RawPreasure(i2craw);		
			BMP085_UTUP(i2craw, &bmp085);
			
			temperatureP = BMP085_Temperarure(&bmp085);
			preasure = BMP085_Preasure(&bmp085);
			
			owraw = GetSingleTemperature();
			
			RTC_GetTime(RTC_Format_BIN, &RTCTimeStr);
			RTC_GetDate(RTC_Format_BIN, &RTCDateStr);
			
			switch (mode){
			case 0:				
				if (first_time_in_mode==1) {				
					first_time_in_mode = 0;
					GPIO_LOW(LD_PORT,LD_GREEN);
				}
				
				break;
				
			case 1:
				if (first_time_in_mode==1) {				
					first_time_in_mode = 0;
					GPIO_TOGGLE(LD_PORT,LD_GREEN);
					
				}
										
										
				break;
				
			case 2:
				if (first_time_in_mode==1) {				
					first_time_in_mode = 0;
					
				}
										
				GPIO_TOGGLE(LD_PORT,LD_GREEN);
										
				break;
				
		}			

	}
		
	if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET)
	{
		rd = USART_ReceiveData(USART1);
//			if (rd == 122) 
//			{				
//				sprintf(strDisp, "begin;%2.2f;%2d;%2.1f;", voltage_V, temperature_C, temperature);				
//				USART_DMA_send(USART1, strDisp, strlen(strDisp));		
//				sprintf(strDisp, "%2.1f;end\n\r", humidity);				
//				USART_DMA_send(USART1, strDisp, strlen(strDisp));
//			}
//			else
//			{
//			}				
		
		sprintf(strDisp, "UART=%d;\n\r", rd);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "MODE=%d;\n\r", mode);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "CH13_RAW=%d;\n\r", ADC_RES.Chanel13AVG);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "T=%d;\n\r", period);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "t=%d;\n\r", dirty_cycle);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "V_ref_RAW=%d;\n\r", ADC_RES.refAVG);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "Vref=%2.2fV;\n\r", ADC_RES.voltage_V);				
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "T_core_RAW=%d;\n\r", ADC_RES.tempAVG);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "Tcore=%2dC;\n\r", ADC_RES.temperature_C);				
		USART_DMA_send(USART1, strDisp, strlen(strDisp));

		sprintf(strDisp, "DHT_RAW=%02x%02x%02x%02x%02x;\n\r", dhtbuf[0], dhtbuf[1], dhtbuf[2], dhtbuf[3], dhtbuf[4]);				
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "T_DHT=%2.1fC;\n\r", temperature);				
		USART_DMA_send(USART1, strDisp, strlen(strDisp));		
		sprintf(strDisp, "H_DHT=%2.1f%%;\n\r", humidity);				
		USART_DMA_send(USART1, strDisp, strlen(strDisp));

		sprintf(strDisp, "BMP085_Calib %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x;\n\r",
			bmp085.calib.raw[0], bmp085.calib.raw[1], bmp085.calib.raw[2], bmp085.calib.raw[3], bmp085.calib.raw[4], bmp085.calib.raw[5],
			bmp085.calib.raw[6], bmp085.calib.raw[7], bmp085.calib.raw[8], bmp085.calib.raw[9], bmp085.calib.raw[10]);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));
		sprintf(strDisp, "BMP085_RAW %02x%02x %02x%02x%02x;\n\r",i2craw[0],i2craw[1],i2craw[2],i2craw[3],i2craw[4]);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));

		sprintf(strDisp, "T_BMP085=%2.1fC;\n\r", temperatureP);		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));				
		sprintf(strDisp, "P_BMP085=%dPa=%3.1fmmHg;\n\r", preasure, BMP085_Preasure_mm(preasure));		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));

		sprintf(strDisp, "T_DS=%2.1fC;\n\r", CalculateTemperature(owraw));		
		USART_DMA_send(USART1, strDisp, strlen(strDisp));				

		sprintf(strDisp, "DATE: %02d/%02d/%02d %02d:%02d:%02d.\n\r", RTCDateStr.RTC_Year, RTCDateStr.RTC_Month, RTCDateStr.RTC_Date, RTCTimeStr.RTC_Hours, RTCTimeStr.RTC_Minutes, RTCTimeStr.RTC_Seconds);
		USART_DMA_send(USART1, strDisp, strlen(strDisp));				

		}
	
	}
	
}




void RCC_Configuration(void){

  //SysTick_Config(SystemCoreClock / 4000);
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 2000);

  /* Enable the GPIOs Clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC| RCC_AHBPeriph_GPIOD| RCC_AHBPeriph_GPIOE| RCC_AHBPeriph_GPIOH, ENABLE);     
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
		
}  




uint16_t uint16_time_diff(uint16_t now, uint16_t before)
{
  return (now >= before) ? (now - before) : (UINT16_MAX - before + now);
}

void Delay(uint32_t nTime){
  TimingDelay = nTime;

  while(TimingDelay != 0);
  
}

void TimingDelay_Decrement(void){

  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
	
	++systick_ms;
}

void setADCDMA_TransferComplete(void)
{
  flag_ADCDMA_TransferComplete = TRUE;
}

void clearADCDMA_TransferComplete(void)
{
  flag_ADCDMA_TransferComplete = FALSE;
}


