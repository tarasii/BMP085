 /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BMP085_H
#define __BMP085_H

/* Includes ------------------------------------------------------------------*/
#include "utils.h"
#include "i2c.h"

#define I2Cx I2C2

//typedef enum {Error = 0, Success = !Error } Status;

//osrs  0    1    2    3
//addr 0x34 0x74 0xB4 0xF4 
#define BMP085_OSRS 1 

#pragma anon_unions
union bmp085_calibration_coefficients_type
{
//	uint8_t raw[22];
	uint16_t raw[11];
	struct 
	{
		int16_t AC1;
		int16_t AC2;
		int16_t AC3;
		uint16_t AC4;
		uint16_t AC5;
		uint16_t AC6;
		int16_t B1;
		int16_t B2;
		int16_t MB;
		int16_t MC;
		int16_t MD;
	};
};

struct bmp085_type
{
	union bmp085_calibration_coefficients_type calib;
	uint16_t UT;
	uint32_t UP;
	int32_t B5;
	int32_t B3;
	uint32_t B4;
	uint32_t B7;
	int32_t X1;
	int32_t X2;
};

Status BMP085_RawCalibData(uint16_t *buf);
Status BMP085_RawTemperarure(uint8_t *buf);
Status BMP085_RawPreasure(uint8_t *buf);

float BMP085_Temperarure(struct bmp085_type *bmp085);
int32_t BMP085_Preasure(struct bmp085_type *bmp085);

float BMP085_Preasure_mm(int32_t P);

void BMP085_UTUP(uint8_t *buf, struct bmp085_type *bmp085);

#endif /* __BMP085_H */
