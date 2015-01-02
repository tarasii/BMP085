#include "bmp085.h"

void RawTemperarure_bmp085(uint8_t *buf)
{
	uint8_t i2cbuf[2];
	
	i2cbuf[0] = 0xF4;
	i2cbuf[1] = 0x2E;
	I2C_Write(I2Cx,i2cbuf,2,0xEE);
	
	Delay(5);
	
	i2cbuf[0] = 0xF6;
	I2C_Write(I2Cx,i2cbuf,1,0xEE);
	
	I2C_Read(I2Cx,i2cbuf,1,0xEF);
	buf[0] = i2cbuf[0];
	
	I2C_Read(I2Cx,i2cbuf,1,0xEF);
	buf[1] = i2cbuf[0];
}

void RawPreasure_bmp085(uint8_t *buf)
{
	uint8_t i2cbuf[2];
	
	i2cbuf[0] = 0xF4;
	i2cbuf[1] = 0x34;
	I2C_Write(I2Cx,i2cbuf,2,0xEE);
	
	Delay(50);
	
	i2cbuf[0] = 0xF6;
	I2C_Write(I2Cx,i2cbuf,1,0xEE);
	
	I2C_Read(I2Cx,i2cbuf,1,0xEF);
	buf[2] = i2cbuf[0];
	
	I2C_Read(I2Cx,i2cbuf,1,0xEF);
	buf[3] = i2cbuf[0];
	
	I2C_Read(I2Cx,i2cbuf,1,0xEF);
	buf[4] = i2cbuf[0];
}
