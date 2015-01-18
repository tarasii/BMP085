#include "adc.h"

//configureDMA
DMA_InitTypeDef DMA_InitStructure;

//acquireTemperatureData
CALIB_TypeDef calibdata;    /* field storing temp sensor calibration data */
uint16_t ADC_ConvertedValueBuff[ADC_CONV_BUFF_SIZE];

void acquireTemperatureData()
{
  
 /* Enable ADC clock */
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  /* Enable DMA1 clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  
  /* Enable ADC1 */
  //ADC_Cmd(ADC1, ENABLE);

  /* Wait until the ADC1 is ready */
  //while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADONS) == RESET); 

  /* re-initialize DMA -- is it needed ?*/
  DMA_DeInit(DMA1_Channel1);
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  DMA_Cmd(DMA1_Channel1, ENABLE);
  
  /* Enable DMA channel 1 Transmit complete interrupt*/
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

  /* Disable DMA mode for ADC1 */ 
  ADC_DMACmd(ADC1, DISABLE);

   /* Enable DMA mode for ADC1 */  
  ADC_DMACmd(ADC1, ENABLE);
  
//  /* Clear global flag for DMA transfert complete */
//  clearADCDMA_TransferComplete(); 
  
  /* Start ADC conversion */
  ADC_SoftwareStartConv(ADC1);
}

void insertionSort(uint16_t *numbers, uint32_t array_size) 
{
  
	uint32_t i, j;
	uint32_t index;

  for (i=1; i < array_size; i++) {
    index = numbers[i];
    j = i;
    while ((j > 0) && (numbers[j-1] > index)) {
      numbers[j] = numbers[j-1];
      j = j - 1;
    }
    numbers[j] = index;
  }
}

uint32_t interquartileMean(uint16_t *array, uint32_t numOfSamples)
{
	uint32_t sum=0;
	uint32_t  index, maxindex;
	/* discard  the lowest and the highest data samples */ 
	maxindex = 3 * numOfSamples / 4;
	for (index = (numOfSamples / 4); index < maxindex; index++){
					sum += array[index];
	}
	/* return the mean value of the remaining samples value*/
	return ( sum / (numOfSamples / 2) );
}

void processTempData(ADC_Typedef* ADC_RES)
{
  uint32_t index, dataSum;

  /* sort received data in */
  insertionSort(ADC_ConvertedValueBuff, MAX_ADC_CHNL);
  
	//One value
	//ADC_RES->Chanel13AVG = ADC_ConvertedValueBuff[0];

	//Math avarage
  //dataSum = 0;
  //for (index=0; index < MAX_ADC_CHNL; index++){
  //  dataSum += ADC_ConvertedValueBuff[index];
  //}
  //ADC_RES->Chanel13AVG = dataSum / MAX_ADC_CHNL ;
 
  /* Calculate the Interquartile mean */
  ADC_RES->Chanel13AVG = interquartileMean(ADC_ConvertedValueBuff, MAX_ADC_CHNL);
	

	//Core temperature math average
  dataSum = 0;
  /* Sum up all mesured data for reference temperature average calculation */ 
  for (index = MAX_ADC_CHNL; index < MAX_ADC_CHNL + MAX_TEMP_CHNL; index++){
    dataSum += ADC_ConvertedValueBuff[index];
  }
  /* Devide sum up result by 4 for the temperature average calculation*/
  ADC_RES->tempAVG = dataSum / 4 ;
  //ADC_RES->tempAVG = ADC_ConvertedValueBuff[16]  ;
	
  dataSum = 0;
  /* Sum up all mesured data for reference temperature average calculation */ 
  for (index = MAX_ADC_CHNL + MAX_TEMP_CHNL; index < ADC_CONV_BUFF_SIZE; index++){
    dataSum += ADC_ConvertedValueBuff[index];
  }
  /* Devide sum up result by 4 for the temperature average calculation*/
  ADC_RES->refAVG = dataSum / 4 ;


  /* Calculate temperature in �C from Interquartile mean */
  ADC_RES->temperature_C = ADC_RES->tempAVG - (int32_t) calibdata.TS_CAL_COLD;	
  ADC_RES->temperature_C = ADC_RES->temperature_C * (int32_t)(HOT_CAL_TEMP - COLD_CAL_TEMP);                      
  ADC_RES->temperature_C = ADC_RES->temperature_C / 
                  (int32_t)(calibdata.TS_CAL_HOT - calibdata.TS_CAL_COLD); 
  ADC_RES->temperature_C = ADC_RES->temperature_C + COLD_CAL_TEMP; 
	
  /* Calculate voltage in V from average */
  ADC_RES->voltage_V = (VREF/ADC_RES->refAVG) * ADC_CONV;

  /* Calculate preasure in mmHg */
  //preasure_V = (preasure_ref * preasureAVG) / preasure_conv;
	
  //altTemp_V = (altTempAVG * voltage_V * 100)/ ADC_CONV;
	
}


FunctionalState  testUserCalibData(void)
{
  int32_t testdiff;
  FunctionalState retval = DISABLE;
  
  testdiff = USER_CALIB_DATA->TS_CAL_HOT - USER_CALIB_DATA->TS_CAL_COLD;
  
  if ( testdiff > TEST_CALIB_DIFF )    retval = ENABLE;
  
  return retval;
}

FunctionalState  testFactoryCalibData(void)
{
  int32_t testdiff;
  FunctionalState retval = DISABLE;
  
  testdiff = FACTORY_CALIB_DATA->TS_CAL_HOT - FACTORY_CALIB_DATA->TS_CAL_COLD;
  
  if ( testdiff > TEST_CALIB_DIFF )    retval = ENABLE;
  
  return retval;
}

void  writeCalibData(CALIB_TypeDef* calibStruct)
{
	__IO FLASH_Status FLASHStatus = FLASH_COMPLETE; 

  uint32_t  Address = 0;
  uint32_t  dataToWrite;
  
  /* Unlock the FLASH PECR register and Data EEPROM memory */
  DATA_EEPROM_Unlock();
  
  /* Clear all pending flags */      
  FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR
                  | FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR);	
  
  /*  Data EEPROM Fast Word program of FAST_DATA_32 at addresses defined by 
      DATA_EEPROM_START_ADDR and DATA_EEPROM_END_ADDR */  
 
  Address = (uint32_t) USER_CALIB_DATA;


  dataToWrite = 0x00;
  dataToWrite = (uint32_t)(calibStruct->TS_CAL_COLD) << 16;
  
  FLASHStatus = DATA_EEPROM_ProgramWord(Address, dataToWrite);

  if(FLASHStatus != FLASH_COMPLETE)
  {
    while(1); /* stay in loop in case of crittical programming error */
  }

  Address += 4;

  dataToWrite = 0x00;
  dataToWrite = (uint32_t)(calibStruct->TS_CAL_HOT) << 16;
  
  FLASHStatus = DATA_EEPROM_ProgramWord(Address, dataToWrite);
  
}

void adc_init(){
	
  GPIO_InitTypeDef GPIO_InitStructure; 
	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
  uint32_t ch_index;
	__IO uint16_t 	T_StartupTimeDelay;
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);     
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);     
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);     

/* ADC input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3  ;                               
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
	
	
  /* Test user or factory temperature sensor calibration value */
  if ( testUserCalibData() == ENABLE ) calibdata = *USER_CALIB_DATA;
  else if ( testFactoryCalibData() == ENABLE ) calibdata = *FACTORY_CALIB_DATA;
  else {
    calibdata.TS_CAL_COLD = DEFAULT_COLD_VAL;
    calibdata.TS_CAL_HOT = DEFAULT_HOT_VAL;
    writeCalibData(&calibdata);
    calibdata = *USER_CALIB_DATA;
  }
  
  /* Enable the internal connection of Temperature sensor and with the ADC channels*/
  ADC_TempSensorVrefintCmd(ENABLE); 
  
  /* Wait until ADC + Temp sensor start */
  T_StartupTimeDelay = 1024;
  while (T_StartupTimeDelay--);

  /* Setup ADC common init struct */
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  
  /* Initialise the ADC1 by using its init structure */
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;	          			// Set conversion resolution to 12bit
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;	                          // Enable Scan mode (single conversion for each channel of the group)
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;			  							// Disable Continuous conversion
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConvEdge_None; // Disable external conversion trigger
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;                  // Set conversion data alignement to right
  ADC_InitStructure.ADC_NbrOfConversion = ADC_CONV_BUFF_SIZE;             // Set conversion data alignement to ADC_CONV_BUFF_SIZE
  //ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);


	for (ch_index = 1; ch_index <= MAX_ADC_CHNL; ch_index++){
		ADC_RegularChannelConfig(ADC1, ADC_Channel_13, ch_index, 
														 ADC_SampleTime_384Cycles);
	}

//	for (ch_index = MAX_ADC_CHNL + 1; ch_index <= MAX_TEMP_CHNL + MAX_ADC_CHNL; ch_index++){
//		ADC_RegularChannelConfig(ADC1, ADC_Channel_16, ch_index, 
//														 ADC_SampleTime_384Cycles);
//	}

  /* ADC1 regular Temperature sensor channel16 and internal reference channel17 configuration */ 
	// core temperature
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 13, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 14, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 15, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 16, ADC_SampleTime_384Cycles);

	// vref
	ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 17, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 18, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 19, ADC_SampleTime_384Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 20, ADC_SampleTime_384Cycles);

  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Wait until the ADC1 is ready */
  while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADONS) == RESET); 
		
}

void configureDMA()
{
  /* Declare NVIC init Structure */
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable DMA1 clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* De-initialise  DMA */
  DMA_DeInit(DMA1_Channel1);
  
  //DMA_InitTypeDef DMA_InitStructure;
	
	/* DMA1 channel1 configuration */
  DMA_StructInit(&DMA_InitStructure);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);	     		 // Set DMA channel Peripheral base address to ADC Data register
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_ConvertedValueBuff;  // Set DMA channel Memeory base addr to ADC_ConvertedValueBuff address
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                         // Set DMA channel direction to peripheral to memory
  DMA_InitStructure.DMA_BufferSize = ADC_CONV_BUFF_SIZE;                     // Set DMA channel buffersize to peripheral to ADC_CONV_BUFF_SIZE
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	     		 // Disable DMA channel Peripheral address auto increment
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                    // Enable Memeory increment (To be verified ....)
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;// set Peripheral data size to 8bit 
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;	     	 // set Memeory data size to 8bit 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                              // Set DMA in normal mode
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;	                     	 // Set DMA channel priority to High
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                               // Disable memory to memory option 
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);		// Use Init structure to initialise channel1 (channel linked to ADC)

  /* Enable Transmit Complete Interrup for DMA channel 1 */ 
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
  
  /* Setup NVIC for DMA channel 1 interrupt request */
  NVIC_InitStructure.NVIC_IRQChannel =   DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
}

uint16_t adc_coretemp_simple(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	
	uint16_t AD_value;
	uint16_t TemperatureC;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);		//enable ADC1 clock
	
	//ADC1 configuration
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;						//convert single channel only
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 		//convert one time
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConvEdge_None; //select no external triggering
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 	//right 12-bit data alignment in ADC data register
	ADC_InitStructure.ADC_NbrOfConversion = 1; 							//single channel conversion	
	ADC_Init(ADC1, &ADC_InitStructure); 							//load structure values to control and status registers
	
	ADC_TempSensorVrefintCmd(ENABLE); 								//wake up temperature sensor
	
	//ADC1 channel16 configuration
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_384Cycles);
	
	ADC_Cmd(ADC1, ENABLE); 														//Enable ADC1
	
	calibdata = *FACTORY_CALIB_DATA;
	
  ADC_SoftwareStartConv(ADC1);
	
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)){} 	//wait for conversion complete
	
	AD_value = ADC_GetConversionValue(ADC1);					//read ADC value
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);								//clear EOC flag

	TemperatureC = AD_value - (int32_t) calibdata.TS_CAL_COLD;	
  TemperatureC = TemperatureC * (int32_t)(HOT_CAL_TEMP - COLD_CAL_TEMP);                      
  TemperatureC = TemperatureC / 
                  (int32_t)(calibdata.TS_CAL_HOT - calibdata.TS_CAL_COLD); 
  TemperatureC = TemperatureC + COLD_CAL_TEMP; 
	return TemperatureC;
}


