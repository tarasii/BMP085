#ifndef _UART_H
#define _UART_H

#include "utils.h"

#define DMAy_Channelx DMA1_Channel4

void USART_open(USART_TypeDef* USARTx, uint32_t baud);
void USART_DMA_send(USART_TypeDef* USARTx, char *buf, uint8_t len);
void USART_close(USART_TypeDef* USARTx);
void USART_print(USART_TypeDef* USARTx, char *buf, uint8_t len);
int uart_putc(USART_TypeDef* USARTx, uint32_t c);
int uart_getc(USART_TypeDef* USARTx);

#endif
