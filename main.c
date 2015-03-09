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

uint32_t dht_cycle [43];

volatile uint32_t dirty_cycle = 0, period = 0;

uint32_t intcnt = 0;

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
    
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);

		period = TIM_GetCapture1(TIM2);
		dirty_cycle = TIM_GetCapture2(TIM2);
		
		if (intcnt < 43) {
			dht_cycle[intcnt] = period;
			intcnt++;
			//dht_cycle[intcnt] = dirty_cycle;
			//intcnt++;
		}
		
    if (TIM_GetFlagStatus(TIM2, TIM_FLAG_CC1OF) != RESET)
    {
      TIM_ClearFlag(TIM2, TIM_FLAG_CC1OF);
      intcnt++;
    }

	}
}

void DMA1_Channel1_IRQHandler    (void)
{
  DMA_ClearFlag(DMA1_IT_TC1);
  setADCDMA_TransferComplete();  /* set flag_ADCDMA_TransferComplete global flag */
}



int main(void){

	char strDisp[25]; 
	
	uint8_t dhtbuf[5]; 
	//uint16_t owraw[owdevnum];
	uint8_t rd;
	uint8_t cmd_mode;
	uint8_t i2craw[5];
	struct bmp085_type bmp085;
	ADC_Typedef ADC_RES;
	
	RTC_DateTypeDef RTCDateStr;
	RTC_TimeTypeDef RTCTimeStr;
	
	uint8_t  num_ow, i_ow;
	uint8_t idbuf[owdevnum][8];
	uint16_t raw_ow[owdevnum];
	
	RCC_Configuration();
	
	button_init_irq();
	
//	pin_mode(DHT11_PORT, DHT11_PIN, GPIO_MODE_OUT2_PP);
// 	GPIO_HIGH(DHT11_PORT,DHT11_PIN);
	
	pin_mode(TIM2_GPIO, TIM2_CH1, GPIO_MODE_OUT2_PP);
	GPIO_HIGH(TIM2_GPIO,	TIM2_CH1);

	pin_mode(GPIOA, GPIO_Pin_9, GPIO_MODE_OUT2_PP);
	GPIO_HIGH(GPIOA,	GPIO_Pin_9);

	led_init();
	
	rtc_init();
	
  USART_open(USART1, 9600);   //for RS232
  USART_open(USART2, 115200); //for DS18B20
	
	configureDMA();
	
	adc_init();	
	
	I2C_LowLevel_Init (I2C2, 1000, 0xA0);
	
	BMP085_RawCalibData(bmp085.calib.raw);

	dac_init(DAC_Channel_1);
	dac_set(DAC_Channel_1, 600);
	
	Delay(200);
	
	num_ow = OW_Scan((uint8_t *)idbuf, owdevnum);
  OW_Send(OW_SEND_RESET, "\xcc\x44", 2, NULL, NULL, OW_NO_READ);
	
	//tim_init_pwmout(8000, 2000); //2KHz
	//tim_init_pwmout(200, 100); //80KHz
	tim_init_pwmout(800, 200);	
	tim_init_cnt();
	//GPIO_HIGH(TIM2_GPIO,	TIM2_CH1);
	
	Delay(1000); //power up delay 
	
	cmd_mode = 1; //first read all

	while(1)
	{
	
		if (flag_UserButton == 1){
			if (++mode == 3){mode = 0;}
			flag_UserButton = 0;
			first_time_in_mode = 1;
		}
									
 		if (uint16_time_diff(systick_ms, toggle_ms) >= 1000) //1 sec delay
 		{
 			toggle_ms = systick_ms;

			GPIO_TOGGLE(LD_PORT,LD_BLUE);
				
			//mode selector
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
										
				if (cmd_mode==0) cmd_mode = 1;						
				break;
				
			case 2:
				if (first_time_in_mode==1) {				
					first_time_in_mode = 0;
					
				}
										
				GPIO_TOGGLE(LD_PORT,LD_GREEN);
				if (cmd_mode==0) cmd_mode = 2;						
										
				break;
				
			}			
		}
		
		if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET)
		{
			rd = USART_ReceiveData(USART1);
			switch (rd){
				case 117:
					cmd_mode = 1; //u
					break;
			
				case 97:
					cmd_mode = 2; //a
					break;
			
				case 116:
					cmd_mode = 4; //t
					break;
			
				case 112:
					cmd_mode = 5; //p
					break;
			
				case 104:
					cmd_mode = 6; //h
					break;
			
				case 122:
					cmd_mode = 7; //z
					break;
			
				case 110:
					cmd_mode = 8; //n
					break;
			
				default :
					cmd_mode = 3;
				
			}
		}
	
		switch (cmd_mode){
			case 1: //read all
			
				//internal ADC with DMA
				clearADCDMA_TransferComplete();
				acquireTemperatureData();
				while (!flag_ADCDMA_TransferComplete);			
				processTempData(&ADC_RES);
				//ADC_RES.temperature_C = adc_coretemp_simple();

				BMP085_RawTemperarure(i2craw);
				BMP085_RawPreasure(i2craw);		
				BMP085_UTUP(i2craw, &bmp085);
				
				temperatureP = BMP085_Temperarure(&bmp085);
				preasure = BMP085_Preasure(&bmp085);
				
				//owraw = GetSingleTemperature();
				for (i_ow = 0; i_ow<num_ow; i_ow++){
					raw_ow[i_ow] = GetTemperature((uint8_t *) idbuf[i_ow]);
				}
				
				RTC_GetTime(RTC_Format_BIN, &RTCTimeStr);
				RTC_GetDate(RTC_Format_BIN, &RTCDateStr);

//				DHT11_RawRead(dhtbuf);		
				DHT11_FromTimerRead(dhtbuf, dht_cycle, &intcnt);
				humidity = DHT22_Humidity(dhtbuf);
				temperature = DHT22_Temperature(dhtbuf);

				sprintf(strDisp, "REDY!\n\r");		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				
				cmd_mode = 0;
				break;
			
			case 2: //write all
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
		//		sprintf(strDisp, "intcnt=%d;\n\r", intcnt);		
		//		USART_DMA_send(USART1, strDisp, strlen(strDisp));
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

				sprintf(strDisp, "BMP085_CALIB_RAW=%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x;\n\r",
					bmp085.calib.raw[0], bmp085.calib.raw[1], bmp085.calib.raw[2], bmp085.calib.raw[3], bmp085.calib.raw[4], bmp085.calib.raw[5],
					bmp085.calib.raw[6], bmp085.calib.raw[7], bmp085.calib.raw[8], bmp085.calib.raw[9], bmp085.calib.raw[10]);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				sprintf(strDisp, "BMP085_RAW=%02x%02x %02x%02x%02x;\n\r",i2craw[0],i2craw[1],i2craw[2],i2craw[3],i2craw[4]);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));

				sprintf(strDisp, "T_BMP085=%2.1fC;\n\r", temperatureP);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));				
				sprintf(strDisp, "P_BMP085=%dPa=%3.1fmmHg;\n\r", preasure, BMP085_Preasure_mm(preasure));		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));

		//		sprintf(strDisp, "T_DS=%2.1fC;\n\r", CalculateTemperature(owraw));		
		//		USART_DMA_send(USART1, strDisp, strlen(strDisp));				
				sprintf(strDisp, "OW_DEV=%d;\n\r", num_ow);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));				
				for (i_ow = 0; i_ow<num_ow; i_ow++){
						sprintf(strDisp, "OW%d_RAW=%02x%02x%02x%02x%02x%02x%02x%02x %04x;\n\r", 
							i_ow, raw_ow[i_ow], 
							idbuf[i_ow][0],idbuf[i_ow][1],idbuf[i_ow][2],idbuf[i_ow][3],
							idbuf[i_ow][4],idbuf[i_ow][5],idbuf[i_ow][6],idbuf[i_ow][7]);		
						USART_DMA_send(USART1, strDisp, strlen(strDisp));
						if (idbuf[i_ow][0] == 0x28){ //temperature sensor id
								sprintf(strDisp, "T_DS%d=%2.1fC;\n\r", i_ow, CalculateTemperature(raw_ow[i_ow]));		
								USART_DMA_send(USART1, strDisp, strlen(strDisp));
						}
				}

				sprintf(strDisp, "DATE: %02d/%02d/%02d %02d:%02d:%02d.\n\r", RTCDateStr.RTC_Year, RTCDateStr.RTC_Month, RTCDateStr.RTC_Date, RTCTimeStr.RTC_Hours, RTCTimeStr.RTC_Minutes, RTCTimeStr.RTC_Seconds);
				USART_DMA_send(USART1, strDisp, strlen(strDisp));		
				cmd_mode = 0;				
      break;
				
			case 3: //write key
				sprintf(strDisp, "UART=%d;\n\r", rd);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				sprintf(strDisp, "MODE=%d;\n\r", mode);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				cmd_mode = 0;				
      break;
			
			case 4: //write only ds18b20
				for (i_ow = 0; i_ow<num_ow; i_ow++){
					if (idbuf[i_ow][0] == 0x28){ //temperature sensor id
						sprintf(strDisp, "%02x%02x%02x%02x%02x%02x%02x%02x ", 
							idbuf[i_ow][7],idbuf[i_ow][6],idbuf[i_ow][5],idbuf[i_ow][4],
							idbuf[i_ow][3],idbuf[i_ow][2],idbuf[i_ow][1],idbuf[i_ow][0]);		
						USART_DMA_send(USART1, strDisp, strlen(strDisp));
						sprintf(strDisp, "%2.1f\n\r", CalculateTemperature(raw_ow[i_ow]));		
						USART_DMA_send(USART1, strDisp, strlen(strDisp));
					}
				}
				cmd_mode = 0;				
      break;				
			
			case 5: //write only bmp035
				sprintf(strDisp, "%2.1f %3.1f\n\r", temperatureP, BMP085_Preasure_mm(preasure));		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				cmd_mode = 0;				
      break;
			
			case 6: //write only dht22
				sprintf(strDisp, "%2.1f %2.1f\n\r", temperature, humidity);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				cmd_mode = 0;				
      break;
			
			case 7: //old protocol
				sprintf(strDisp, "begin;%2.1f;%2.1f;", temperatureP, ADC_RES.voltage_V);				
				USART_DMA_send(USART1, strDisp, strlen(strDisp));		
				sprintf(strDisp, "%2.1f;%2.1f;", temperature, humidity);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));		
				sprintf(strDisp, "%2.1f;%3.1f;end\n\r", humidity, BMP085_Preasure_mm(preasure));				
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				cmd_mode = 0;				
      break;
			
			case 8: //dht22 on timer
				
				DHT11_FromTimerRead(dhtbuf, dht_cycle, &intcnt);
				humidity = DHT22_Humidity(dhtbuf);
				temperature = DHT22_Temperature(dhtbuf);

				sprintf(strDisp, "n=%d \n\r", intcnt);		
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				sprintf(strDisp, "DHT_RAW=%02x%02x%02x%02x%02x;\n\r", dhtbuf[0], dhtbuf[1], dhtbuf[2], dhtbuf[3], dhtbuf[4]);				
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
				sprintf(strDisp, "T_DHT=%2.1fC;\n\r", temperature);				
				USART_DMA_send(USART1, strDisp, strlen(strDisp));		
				sprintf(strDisp, "H_DHT=%2.1f%%;\n\r", humidity);				
				USART_DMA_send(USART1, strDisp, strlen(strDisp));
			
				cmd_mode = 0;				
      break;
			
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


