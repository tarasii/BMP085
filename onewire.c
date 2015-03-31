#include "onewire.h"

// ����� ��� ������/�������� �� 1-wire
uint8_t ow_buf[8];

#define OW_0	0x00
#define OW_1	0xff
#define OW_R_1	0xff

//-----------------------------------------------------------------------------
// ������� ����������� ���� ���� � ������, ��� �������� ����� USART
// ow_byte - ����, ������� ���� �������������
// ow_bits - ������ �� �����, �������� �� ����� 8 ����
//-----------------------------------------------------------------------------
void OW_toBits(uint8_t ow_byte, uint8_t *ow_bits) {
	uint8_t i;
	for (i = 0; i < 8; i++) {
		if (ow_byte & 0x01) {
			*ow_bits = OW_1;
		} else {
			*ow_bits = OW_0;
		}
		ow_bits++;
		ow_byte = ow_byte >> 1;
	}
}

//-----------------------------------------------------------------------------
// �������� �������������� - �� ����, ��� �������� ����� USART ����� ���������� ����
// ow_bits - ������ �� �����, �������� �� ����� 8 ����
//-----------------------------------------------------------------------------
uint8_t OW_toByte(uint8_t *ow_bits) {
	uint8_t ow_byte, i;
	ow_byte = 0;
	for (i = 0; i < 8; i++) {
		ow_byte = ow_byte >> 1;
		if (*ow_bits == OW_R_1) {
			ow_byte |= 0x80;
		}
		ow_bits++;
	}

	return ow_byte;
}


uint8_t OW_Reset() {
	uint8_t ow_presence;
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(OW_USART, &USART_InitStructure);

	// ���������� 0xf0 �� �������� 9600
	USART_ClearFlag(OW_USART, USART_FLAG_TC);
	USART_SendData(OW_USART, 0x00);
	while (USART_GetFlagStatus(OW_USART, USART_FLAG_TC) == RESET) {
		}

	ow_presence = USART_ReceiveData(OW_USART);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(OW_USART, &USART_InitStructure);

	if (ow_presence != 0xf0) {
		return OW_OK;
	}

	return OW_NO_DEVICE;
}

// ���������� ���������. ���������� ��������� ����� ���
void OW_SendBits(uint8_t num_bits) {
	DMA_InitTypeDef DMA_InitStructure;

	// DMA �� ������
	DMA_DeInit(OW_DMA_CH_RX);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART2->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = num_bits;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(OW_DMA_CH_RX, &DMA_InitStructure);

	// DMA �� ������
	DMA_DeInit(OW_DMA_CH_TX);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(USART2->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = num_bits;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(OW_DMA_CH_TX, &DMA_InitStructure);

	// ����� ����� ��������
	USART_ClearFlag(OW_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
	USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
	DMA_Cmd(OW_DMA_CH_RX, ENABLE);
	DMA_Cmd(OW_DMA_CH_TX, ENABLE);

	// ����, ���� �� ������ 8 ����
	while (DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) {
	}

	// ��������� DMA
	DMA_Cmd(OW_DMA_CH_TX, DISABLE);
	DMA_Cmd(OW_DMA_CH_RX, DISABLE);
	USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);

}

//-----------------------------------------------------------------------------
// ��������� ������� � ����� 1-wire
// sendReset - �������� RESET � ������ �������.
// 		OW_SEND_RESET ��� OW_NO_RESET
// command - ������ ����, ���������� � ����. ���� ����� ������ - ���������� OW_READ_SLOTH
// cLen - ����� ������ ������, ������� ���� ��������� � ����
// data - ���� ��������� ������, �� ������ �� ����� ��� ������
// dLen - ����� ������ ��� ������. ����������� �� ����� ���� �����
// readStart - � ������ ������� �������� �������� ������ (���������� � 0)
//		����� ������� OW_NO_READ, ����� ����� �� �������� data � dLen
//-----------------------------------------------------------------------------
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen,
		uint8_t *data, uint8_t dLen, uint8_t readStart) {


	// ���� ��������� ����� - ���������� � ��������� �� ������� ���������
	if (sendReset == OW_SEND_RESET) {
		if (OW_Reset() == OW_NO_DEVICE) {
			return OW_NO_DEVICE;
		}
	}

	while (cLen > 0) {

		OW_toBits(*command, ow_buf);
		command++;
		cLen--;


		OW_SendBits(8);

		// ���� ����������� ������ ����-�� ����� - ������� �� � �����
		if (readStart == 0 && dLen > 0) {
			*data = OW_toByte(ow_buf);
			data++;
			dLen--;
		} else {
			if (readStart != OW_NO_READ) {
				readStart--;
			}
		}
	}

	return OW_OK;
}

//-----------------------------------------------------------------------------
// ������ ������� ������������ ������������ ���� 1-wire � ���������� ���������
//   ID ��������� � ������ buf, �� 8 ���� �� ������ ����������.
// ���������� num ������������ ���������� ��������� ���������, ����� �� �����������
// �����.
//-----------------------------------------------------------------------------
uint8_t OW_Scan(uint8_t *buf, uint8_t num) {

        uint8_t found = 0;
        uint8_t *lastDevice;
        uint8_t *curDevice = buf;
        uint8_t numBit, lastCollision, currentCollision, currentSelection;

        lastCollision = 0;
        while (found < num) {
                numBit = 1;
                currentCollision = 0;

                // �������� ������� �� ����� ���������
                OW_Send(OW_SEND_RESET, (uint8_t*)"\xf0", 1, 0, 0, OW_NO_READ);

                for (numBit = 1; numBit <= 64; numBit++) {
                        // ������ ��� ����. �������� � ���������������
                        OW_toBits(OW_READ_SLOT, ow_buf);
                        OW_SendBits(2);

                        if (ow_buf[0] == OW_R_1) {
                                if (ow_buf[1] == OW_R_1) {
                                        // ��� �������, ���-�� ���������� � ����������� �����
                                        return found;
                                } else {
                                        // 10 - �� ������ ����� ������ 1
                                        currentSelection = 1;
                                }
                        } else {
                                if (ow_buf[1] == OW_R_1) {
                                        // 01 - �� ������ ����� ������ 0
                                        currentSelection = 0;
                                } else {
                                        // 00 - ��������
                                        if (numBit < lastCollision) {
                                                // ���� �� ������, �� ����� �� ��������
                                                if (lastDevice[(numBit - 1) >> 3]
                                                                & 1 << ((numBit - 1) & 0x07)) {
                                                        // (numBit-1)>>3 - ����� �����
                                                        // (numBit-1)&0x07 - ����� ���� � �����
                                                        currentSelection = 1;

                                                        // ���� ����� �� ������ �����, ���������� ����� ����
                                                        if (currentCollision < numBit) {
                                                                currentCollision = numBit;
                                                        }
                                                } else {
                                                        currentSelection = 0;
                                                }
                                        } else {
                                                if (numBit == lastCollision) {
                                                        currentSelection = 0;
                                                } else {
                                                        // ���� �� ������ �����
                                                        currentSelection = 1;

                                                        // ���� ����� �� ������ �����, ���������� ����� ����
                                                        if (currentCollision < numBit) {
                                                                currentCollision = numBit;
                                                        }
                                                }
                                        }
                                }
                        }

                        if (currentSelection == 1) {
                                curDevice[(numBit - 1) >> 3] |= 1 << ((numBit - 1) & 0x07);
                                OW_toBits(0x01, ow_buf);
                        } else {
                                curDevice[(numBit - 1) >> 3] &= ~(1 << ((numBit - 1) & 0x07));
                                OW_toBits(0x00, ow_buf);
                        }
                        OW_SendBits(1);
                }
                found++;
                lastDevice = curDevice;
                curDevice += 8;
                if (currentCollision == 0)
                        return found;

                lastCollision = currentCollision;
        }

        return found;
}

