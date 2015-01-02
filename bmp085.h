 /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BMP085_H
#define __BMP085_H

/* Includes ------------------------------------------------------------------*/
#include "utils.h"
#include "i2c.h"

#define I2Cx I2C2

void RawTemperarure_bmp085(uint8_t *buf);
void RawPreasure_bmp085(uint8_t *buf);

#endif /* __BMP085_H */
