#include "wizchip_init.h"
#include "stdio.h"

#ifdef USE_STDPERIPH_DRIVER
DMA_InitTypeDef		DMA_RX_InitStructure, DMA_TX_InitStructure;
#elif defined USE_HAL_DRIVER
#endif
void temp_wizchip_cris_enter()
{
}
void temp_wizchip_cris_exit()
{
}

void W6100Initialze(void)
{
	W6100Reset();

#if _WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SPI_
/* SPI method callback registration */
	#if defined SPI_DMA
	reg_wizchip_spi_cbfunc(W6100SpiReadByte, W6100SpiWriteByte, W6100SpiReadBurst, W6100SpiWriteBurst);
	#else
	reg_wizchip_spi_cbfunc(W6100SpiReadByte, W6100SpiWriteByte, 0, 0);
	#endif
	/* CS function register */
	reg_wizchip_cs_cbfunc(W6100CsEnable, W6100CsDisable);
#else
/* Indirect bus method callback registration */
	#if defined BUS_DMA
	reg_wizchip_bus_cbfunc(W6100BusReadByte, W6100BusWriteByte, W6100BusReadBurst, W6100BusWriteBurst);
	#else
	reg_wizchip_bus_cbfunc(W6100BusReadByte, W6100BusWriteByte, 0, 0);
	#endif
#endif
	uint8_t temp;
	unsigned char W6100_AdrSet[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};
	do
	{
		if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1)
		{
			printf("Unknown PHY link status.\r\n");
		}
	} while (temp == PHY_LINK_OFF);
	printf("PHY OK.\r\n");

	temp = IK_DEST_UNREACH;

	if (ctlwizchip(CW_INIT_WIZCHIP, (void *)W6100_AdrSet) == -1)
	{
		printf("W6100 initialized fail.\r\n");
	}

	if (ctlwizchip(CW_SET_INTRMASK, &temp) == -1)
	{
		printf("W6100 interrupt\r\n");
	}
	//printf("interrupt mask: %02x\r\n",getIMR());
}

uint8_t W6100SpiReadByte(void)
{
#ifdef USE_STDPERIPH_DRIVER

	while (SPI_I2S_GetFlagStatus(W6100_SPI, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(W6100_SPI, 0xff);
	while (SPI_I2S_GetFlagStatus(W6100_SPI, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(W6100_SPI);

#elif defined USE_HAL_DRIVER

	uint8_t rx = 0, tx = 0xFF;
	HAL_SPI_TransmitReceive(&W6100_SPI, &tx, &rx, W6100_SPI_SIZE, W6100_SPI_TIMEOUT);
	return rx;
#elif defined USE_HAL_DRIVER

	// Sppi read
#endif
    return 0;
}

void W6100SpiWriteByte(uint8_t byte)
{
#ifdef USE_STDPERIPH_DRIVER

	while (SPI_I2S_GetFlagStatus(W6100_SPI, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(W6100_SPI, byte);
	while (SPI_I2S_GetFlagStatus(W6100_SPI, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData(W6100_SPI);

#elif defined USE_HAL_DRIVER

	uint8_t rx;
	HAL_SPI_TransmitReceive(&W6100_SPI, &byte, &rx, W6100_SPI_SIZE, W6100_SPI_TIMEOUT);
#endif
    return 0;
}

uint8_t W6100SpiReadBurst(uint8_t* pBuf, uint16_t len)
{
#ifdef USE_STDPERIPH_DRIVER

	unsigned char tempbuf =0xff;
	DMA_TX_InitStructure.DMA_BufferSize = len;
	DMA_TX_InitStructure.DMA_MemoryBaseAddr = &tempbuf;
	//DMA_TX_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_Init(W6100_DMA_CHANNEL_TX, &DMA_TX_InitStructure);

	DMA_RX_InitStructure.DMA_BufferSize = len;
	DMA_RX_InitStructure.DMA_MemoryBaseAddr = pBuf;
	DMA_Init(W6100_DMA_CHANNEL_RX, &DMA_RX_InitStructure);
	/* Enable SPI Rx/Tx DMA Request*/
	DMA_Cmd(W6100_DMA_CHANNEL_RX, ENABLE);
	DMA_Cmd(W6100_DMA_CHANNEL_TX, ENABLE);
	/* Waiting for the end of Data Transfer */
	while(DMA_GetFlagStatus(DMA_TX_FLAG) == RESET);
	while(DMA_GetFlagStatus(DMA_RX_FLAG) == RESET);

	DMA_ClearFlag(DMA_TX_FLAG | DMA_RX_FLAG);

	DMA_Cmd(W6100_DMA_CHANNEL_TX, DISABLE);
	DMA_Cmd(W6100_DMA_CHANNEL_RX, DISABLE);

#elif defined USE_HAL_DRIVER
    uint8_t tx[8] ={0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	HAL_SPI_TransmitReceive(&W6100_SPI, tx, pBuf, len, W6100_SPI_TIMEOUT);
#endif
    return 0;

}

void W6100SpiWriteBurst(uint8_t* pBuf, uint16_t len)
{
#ifdef USE_STDPERIPH_DRIVER

	unsigned char tempbuf;
	DMA_TX_InitStructure.DMA_BufferSize = len;
	DMA_TX_InitStructure.DMA_MemoryBaseAddr = pBuf;
	DMA_Init(W6100_DMA_CHANNEL_TX, &DMA_TX_InitStructure);

	DMA_RX_InitStructure.DMA_BufferSize = 1;
	DMA_RX_InitStructure.DMA_MemoryBaseAddr = &tempbuf;
	DMA_RX_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_Init(W6100_DMA_CHANNEL_RX, &DMA_RX_InitStructure);

	DMA_Cmd(W6100_DMA_CHANNEL_RX, ENABLE);
	DMA_Cmd(W6100_DMA_CHANNEL_TX, ENABLE);

	/* Enable SPI Rx/Tx DMA Request*/

	/* Waiting for the end of Data Transfer */
	while(DMA_GetFlagStatus(DMA_TX_FLAG) == RESET);
	while(DMA_GetFlagStatus(DMA_RX_FLAG) == RESET);

	DMA_ClearFlag(DMA_TX_FLAG | DMA_RX_FLAG);

	DMA_Cmd(W6100_DMA_CHANNEL_TX, DISABLE);
	DMA_Cmd(W6100_DMA_CHANNEL_RX, DISABLE);

#elif defined USE_HAL_DRIVER
    uint8_t rx[8] ={0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    HAL_SPI_TransmitReceive(&W6100_SPI, pBuf, rx, len, W6100_SPI_TIMEOUT);

#endif

}

void W6100BusWriteByte(uint32_t addr, iodata_t data)
{
	(*(volatile uint8_t*)(addr)) = data;
}

iodata_t W6100BusReadByte(uint32_t addr)
{
	return (*((volatile uint8_t*)(addr)));
}

void W6100BusWriteBurst(uint32_t addr, uint8_t* pBuf ,uint32_t len,uint8_t addr_inc)
{
#ifdef USE_STDPERIPH_DRIVER

	if(addr_inc){
	 	DMA_TX_InitStructure.DMA_MemoryInc  = DMA_MemoryInc_Enable;

	}
	else 	DMA_TX_InitStructure.DMA_MemoryInc  = DMA_MemoryInc_Disable;


	DMA_TX_InitStructure.DMA_BufferSize = len;
	DMA_TX_InitStructure.DMA_MemoryBaseAddr = addr;
	DMA_TX_InitStructure.DMA_PeripheralBaseAddr = pBuf;

	DMA_Init(W6100_DMA_CHANNEL_TX, &DMA_TX_InitStructure);

	DMA_Cmd(W6100_DMA_CHANNEL_TX, ENABLE);

	/* Enable SPI Rx/Tx DMA Request*/


	/* Waiting for the end of Data Transfer */
	while(DMA_GetFlagStatus(DMA_TX_FLAG) == RESET);


	DMA_ClearFlag(DMA_TX_FLAG);

	DMA_Cmd(W6100_DMA_CHANNEL_TX, DISABLE);

#elif defined USE_HAL_DRIVER

#endif

}

void W6100BusReadBurst(uint32_t addr,uint8_t* pBuf, uint32_t len,uint8_t addr_inc)
{
#ifdef USE_STDPERIPH_DRIVER

	DMA_RX_InitStructure.DMA_BufferSize = len;
	DMA_RX_InitStructure.DMA_MemoryBaseAddr =pBuf;
	DMA_RX_InitStructure.DMA_PeripheralBaseAddr =addr;

	DMA_Init(W6100_DMA_CHANNEL_RX, &DMA_RX_InitStructure);

	DMA_Cmd(W6100_DMA_CHANNEL_RX, ENABLE);
	/* Waiting for the end of Data Transfer */
	while(DMA_GetFlagStatus(DMA_RX_FLAG) == RESET);


	DMA_ClearFlag(DMA_RX_FLAG);


	DMA_Cmd(W6100_DMA_CHANNEL_RX, DISABLE);

#elif defined USE_HAL_DRIVER

#endif

}

inline void W6100CsEnable(void)
{
#ifdef USE_STDPERIPH_DRIVER

	GPIO_ResetBits(W6100_CS_PORT, W6100_CS_PIN);

#elif defined USE_HAL_DRIVER

	HAL_GPIO_WritePin(W6100_CS_PORT, W6100_CS_PIN, GPIO_PIN_RESET);
#endif

}

inline void W6100CsDisable(void)
{
#ifdef USE_STDPERIPH_DRIVER

	GPIO_SetBits(W6100_CS_PORT, W6100_CS_PIN);

#elif defined USE_HAL_DRIVER

	HAL_GPIO_WritePin(W6100_CS_PORT, W6100_CS_PIN, GPIO_PIN_SET);
#endif

}

inline void W6100ResetAssert(void)
{
#ifdef USE_STDPERIPH_DRIVER

	GPIO_ResetBits(W6100_RESET_PORT, W6100_RESET_PIN);

#elif defined USE_HAL_DRIVER

	HAL_GPIO_WritePin(W6100_RESET_PORT, W6100_RESET_PIN, GPIO_PIN_RESET);
#endif

}

inline void W6100ResetDeassert(void)
{
#ifdef USE_STDPERIPH_DRIVER

	GPIO_SetBits(W6100_RESET_PORT, W6100_RESET_PIN);

#elif defined USE_HAL_DRIVER

	HAL_GPIO_WritePin(W6100_RESET_PORT, W6100_RESET_PIN, GPIO_PIN_SET);
#endif

}

void W6100Reset(void)
{
#ifdef USE_STDPERIPH_DRIVER
	W6100ResetAssert();
	delay(10);
	W6100ResetDeassert();
	delay(10);
#elif defined USE_HAL_DRIVER
	W6100ResetAssert();
	HAL_Delay(10);
	W6100ResetDeassert();
	HAL_Delay(10);
#endif
}

#ifdef WIZCHIP_REGISTER_DUMP

void W6100Register_read(void)
{
	int i;
	printf("                    ----register read----\r\n");
	printf("Address | ");
	for (i = 0; i < 16; i++)
		printf("%02x ", i);
	printf("\r\n---------------------------------------------------------");
	for (i = 0; i < 0x0090; i++)
	{
		if (i % 16 == 0)
			printf("\r\n  %04x  | ", i);
		printf("%02x ", WIZCHIP_READ(_W6100_IO_BASE_ + (i << 8) + (WIZCHIP_CREG_BLOCK << 3)));
	}
	printf("\r\n");
}

void W6100SocketRegister_read(uint8_t sn)
{
	int i;
	printf("                    ----Sn register read----\r\n");
	printf("Address | ");
	for (i = 0; i < 16; i++)
		printf("%02x ", i);
	printf("\r\n---------------------------------------------------------");
	for (i = 0x400 + (sn * (0x100)); i < 0x400 + (sn * (0x100) + 0x35); i++)
	{
		if (i % 16 == 0)
			printf("\r\n0x%04x  | ", i);
		printf("%02x ", WIZCHIP_READ(i));
	}
	printf("\r\n");
}

#endif
