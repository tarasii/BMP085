#ifndef I2C_H
#define I2C_H

#include "utils.h"

//+------+------+------+
//| I2C1               |
//+------+------+------+
//| SDA  | SLC  | SMBA |
//+------+------+------+
//| PB6  | PB7  | PB5  |
//| PB8  | PB9  |      |
//+------+------+------+
//| I2C2               |
//+------+------+------+
//| PB10 | PB11 | PB12 |
//+------+------+------+

typedef enum {Error = 0, Success = !Error } Status;

Status I2C_Read(I2C_TypeDef* I2Cx, uint8_t* buf, uint32_t nbuf, uint8_t SlaveAddress);
Status I2C_Write(I2C_TypeDef* I2Cx, const uint8_t* buf, uint32_t nbuf,  uint8_t SlaveAddress);
void I2C_LowLevel_Init(I2C_TypeDef* I2Cx, int ClockSpeed, int OwnAddress);

#endif
