/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "utils.h"
//#include "ext_glcd.h"
//#include "onewire.h"
//#include "18b20.h"
#include "dht11.h"
//#include "ds2417.h"
#include "uart.h"
#include "bmp085.h"
#include "18b20.h"
#include "leds.h"
#include "adc.h"
#include "tim.h"
#include "rtc.h"
#include "button.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void TimingDelay_Decrement(void);
void RCC_Configuration(void);
uint16_t uint16_time_diff(uint16_t now, uint16_t before);
void setADCDMA_TransferComplete(void);
void clearADCDMA_TransferComplete(void);

#endif /* __MAIN_H */
