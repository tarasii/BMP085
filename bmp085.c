#include "bmp085.h"

Status BMP085_RawTemperarure(uint8_t *buf)
{
	uint8_t i2cbuf[2];
	Status res;
	
	i2cbuf[0] = 0xF4;
	i2cbuf[1] = 0x2E;
	res = I2C_Write(I2Cx, i2cbuf, 2, 0xEE);
	if (!res) return Error; 
	
	Delay(5);
	
	i2cbuf[0] = 0xF6;
	res = I2C_Write(I2Cx, i2cbuf, 1, 0xEE);
	if (!res) return Error; 
	
	res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
	if (!res) return Error; 
	buf[0] = i2cbuf[0];
	
	res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
	if (!res) return Error; 
	buf[1] = i2cbuf[0];
	
	return Success;
}

Status BMP085_RawPreasure(uint8_t *buf)
{
	uint8_t i2cbuf[2];
	Status res;
	
	i2cbuf[0] = 0xF4;
	i2cbuf[1] = 0x34 + (BMP085_OSRS << 6);
	res = I2C_Write(I2Cx, i2cbuf, 2, 0xEE);
	if (!res) return Error; 
	
	Delay(25);
	
	i2cbuf[0] = 0xF6;
	res = I2C_Write(I2Cx, i2cbuf, 1, 0xEE);
	if (!res) return Error; 
	
	res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
	if (!res) return Error; 
	buf[2] = i2cbuf[0];
	
	res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
	if (!res) return Error; 
	buf[3] = i2cbuf[0];
	
	res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
	if (!res) return Error; 
	buf[4] = i2cbuf[0];

	return Success;
}

Status BMP085_RawCalibData(uint16_t *buf)
{
	uint8_t i2cbuf[2];
	Status res;
	uint8_t i;
		
	i2cbuf[0] = 0xAA;
	res = I2C_Write(I2Cx, i2cbuf, 1, 0xEE);
	if (!res) return Error; 
	
	for (i = 0; i<11; i++){
		res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
		if (!res) return Error; 
		buf[i] = i2cbuf[0]<<8;
		
		res = I2C_Read(I2Cx, i2cbuf, 1, 0xEF);
		if (!res) return Error; 
		buf[i] |= i2cbuf[0];
	}
	
	return Success;
}

float BMP085_Temperarure(struct bmp085_type *bmp085){
	
	int32_t X1, X2, T;
	
	//X1 = (UT-AC6)*AC5/0x8000
	X1 = (bmp085->UT - bmp085->calib.AC6);
	X1 *= bmp085->calib.AC5;
	X1 >>= 15;
  
	//X2 = MC * 0x800/(X1 + MD)  
	X2 = bmp085->calib.MC;
  X2 <<= 11;
  X2 /= (X1 + bmp085->calib.MD);

	//B5 = X1 + X2
  bmp085->B5 = X1 + X2;
	
	//T = (B5 + 8) / 0x10
	T = (bmp085->B5 + 8) >> 4;
	//T = (bmp085->B5 + 8);
 
  return (float) T/10;
	
}

int32_t BMP085_Preasure(struct bmp085_type *bmp085){
	
	int32_t X1, X2, X3, B6, B3, P;
	uint32_t B4, B7;
	
	//B6 = B5 - 4000 
	B6 = bmp085->B5 - 4000;

	//X1 = (B2 * (B6 * B6 / 0x10000)) / 0x8000
	X1 = B6 * B6;
	X1 >>= 12;
	X1 *= bmp085->calib.B2;
	X1 >>= 11;
	
	X2 = B6 * bmp085->calib.AC2;
	X2 >>= 11;

	//X3 = X1 + X2
	X3 = X1 + X2;

	B3 = (bmp085->calib.AC1 * 4) + X3;
	B3 <<= BMP085_OSRS;
	B3 += 2;
	B3 /= 4;
	bmp085->B3 = B3;

	X1 = bmp085->calib.AC3 * B6;
	X1 >>= 13;
	bmp085->X1 = X1;

	X2 = B6 * B6;
	X2 >>= 12;
	X2 *= bmp085->calib.B1;
	X2 >>= 16;
	bmp085->X2 = X2;
  
	//X3 = (X1 + X2 + 2)/4
	X3 = X1 + X2;
	X3 += 2;
	//X3 >>= 2;
	X3 /= 2;

	B4 = (uint32_t) (X3 + 32768) * bmp085->calib.AC4;
	B4 >>= 15;
	bmp085->B4 = B4;

	B7 = (uint32_t) (bmp085->UP - B3) * (50000 >> BMP085_OSRS);
	bmp085->B7 = B7;

	if (B7 < 0x80000000) P = B7 * 2 / B4;
	else P = B7 * B4 / 2;

	X1 = (P >> 8);
	X1 *= X1 * 3038;
	X1 >>= 16;

	X2 = -7357 * P;
	X2 >>= 16;
	
	P = ((P + (X1 + X2 + 3791) / 16) * 3) >> 4;

	return P;

}

float BMP085_Preasure_mm(int32_t P){
	
	return P * 0.0075;

}

void BMP085_UTUP(uint8_t *buf, struct bmp085_type *bmp085){
		bmp085->UT = (buf[0] << 8) | buf[1];
		bmp085->UP = ((buf[2] << 16) | (buf[3] << 8) | buf[4]) >> (8 - BMP085_OSRS);
}

