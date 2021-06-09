#include "stm32f10x_conf.h"

#include "rfm69.h"

uint8_t packet_buffer[64], rfm69_condition, packet_size;
extern uint8_t internal_packet_buffer[64], internal_pack_size;

void EXTI2_IRQHandler(void)
{
    switch(rfm69_condition)
    {
        case RFM69_SLEEP :
            rfm69_sleep();
            break;
        case RFM69_STBY :
            rfm69_stby();
            break;
        case RFM69_RX :
            if(rfm69_read(REGIRQFLAGS2) & (1<<PAYLOADREADY))
            {
                rfm69_receive_small_packet();
                rfm69_stby();
                rfm69_clear_fifo();
                rfm69_condition = RFM69_NEW_PACK;
            }
            break;
        case RFM69_TX :
            if(rfm69_read(REGIRQFLAGS2) & (1<<PACKETSENT))
            {
                rfm69_receive_start();
            }
            break;
    }
    EXTI_ClearITPendingBit(CRCOK_PKSent_Line);
}


void EXTI1_IRQHandler(void)
{
    switch(rfm69_condition)
    {
        case RFM69_SLEEP :
            break;
        case RFM69_STBY :
            break;
        case RFM69_RX :
            break;
        case RFM69_TX :
            break;
    }
    EXTI_ClearITPendingBit(FifoLevel_Line);
}


void EXTI0_IRQHandler(void)
{
    switch(rfm69_condition)
    {
        case RFM69_SLEEP :
            rfm69_sleep();
            break;
        case RFM69_STBY :
            rfm69_stby();
            break;
        case RFM69_RX :
            TIM2->CNT = 0;
            TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
            NVIC_EnableIRQ(TIM2_IRQn);
            break;
        case RFM69_TX :
            break;
    }
    EXTI_ClearITPendingBit(SyncAddr_Line);
}


void TIM2_IRQHandler(void)
{

    if( (rfm69_read(REGIRQFLAGS1) & (1<<SYNCADDRMATCH)) == 0 )  // if the receiving data is not the package
    {
        rfm69_write(REGOPMODE, REGOPMODE_DEF | STBY_MODE);      // restart the receiver
        rfm69_write(REGOPMODE, REGOPMODE_DEF | RX_MODE);
    }

    NVIC_DisableIRQ(TIM2_IRQn);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}


void rfm69_write(uint8_t address, uint8_t data)
{
	__disable_irq();
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // wait until spi is busy

	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, (address | 0x80));                               // send address
    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );

    SPI_I2S_SendData(SPI1, data);                                           // send data

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // wait until spi is busy
	GPIO_SetBits(NSS_Port, NSS_Pin);

	__enable_irq();
}


uint8_t rfm69_read(uint8_t address)
{
	uint8_t data = 0x00;

	__disable_irq();
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // wait until spi is busy

	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, (address));                                      // send address
    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );

    SPI_I2S_SendData(SPI1, 0x87);                                           // send empty data

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);                                              // receive nothing

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    data = SPI_I2S_ReceiveData(SPI1);                                       // receive data

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // wait until spi is busy
	GPIO_SetBits(NSS_Port, NSS_Pin);

	__enable_irq();
	return data;
}


void rfm69_write_burst(uint8_t address, uint8_t *data, uint8_t ndata)
{
	uint8_t i;

	__disable_irq();
	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, (address | 0x80));

	for (i=0 ; i<ndata ; ++i)
	{
    	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
		SPI_I2S_SendData(SPI1, data[i]);

    	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    	SPI_I2S_ReceiveData(SPI1);
	}
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData(SPI1);

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	GPIO_SetBits(NSS_Port, NSS_Pin);
	__enable_irq();
}


void rfm69_read_burst(uint8_t address, uint8_t *data, uint8_t ndata)
{
	uint8_t i;

	__disable_irq();
	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, address);  // отправили адрес

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); // ждём, пока данные не отпр
	SPI_I2S_SendData(SPI1, 0xff);  // отправили данные

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); // ждём, пока данные не прин
	SPI_I2S_ReceiveData(SPI1);

	--ndata;
	for (i=0 ; i<ndata ; ++i)
	{
    	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); // ждём, пока данные не отпр
		SPI_I2S_SendData(SPI1, 0xff);  // отправили данные

    	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); // ждём, пока данные не прин
    	data[i] = SPI_I2S_ReceiveData(SPI1);  // получили данные
	}
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); // ждём, пока данные не прин
    data[ndata] = SPI_I2S_ReceiveData(SPI1);  // получили данные

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET); // ждём, пока SPI занят
	GPIO_SetBits(NSS_Port, NSS_Pin);
	__enable_irq();
}



void rfm69_mcu_init(void)
{

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);		// enable clock

//	SPI initialization
	GPIO_InitTypeDef gpio;					// NSS pin configuration
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = NSS_Pin;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(NSS_Port, &gpio);

	GPIO_StructInit(&gpio);					// configuration of MOSI and SCK pins
	gpio.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_5;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	GPIO_StructInit(&gpio);					// MISO pin configuration
	gpio.GPIO_Pin = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

// SPI interface configuration
	SPI_InitTypeDef spi;
	SPI_StructInit(&spi);
	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi.SPI_Mode = SPI_Mode_Master;
	spi.SPI_DataSize = SPI_DataSize_8b;
	spi.SPI_CPOL = SPI_CPOL_Low;
	spi.SPI_CPHA = SPI_CPHA_1Edge;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	spi.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI1, &spi);

	SPI_SSOutputCmd(SPI1, DISABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
	GPIO_SetBits(NSS_Port, NSS_Pin);
	SPI_Cmd(SPI1, ENABLE);

//	External interrupt configuration
	NVIC_EnableIRQ(EXTI0_IRQn);					// enable external interrupts
	NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);

	EXTI_InitTypeDef exti_is;
	EXTI_StructInit(&exti_is);
	exti_is.EXTI_Line = CRCOK_PKSent_Line;
	exti_is.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_is.EXTI_Trigger = EXTI_Trigger_Rising;
	exti_is.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_is);

	exti_is.EXTI_Line = FifoLevel_Line;
	exti_is.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_is.EXTI_Trigger = EXTI_Trigger_Rising;
	exti_is.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_is);

	exti_is.EXTI_Line = SyncAddr_Line;
	exti_is.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_is.EXTI_Trigger = EXTI_Trigger_Rising;
	exti_is.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_is);

	GPIO_EXTILineConfig(EXTI_Port1, EXTI_Pin1);
	GPIO_EXTILineConfig(EXTI_Port23, EXTI_Pin2);
	GPIO_EXTILineConfig(EXTI_Port23, EXTI_Pin3);

//  Timer configuration
    TIM_InternalClockConfig(TIM2);

    TIM_TimeBaseInitTypeDef timer_base_is;
    TIM_TimeBaseStructInit(&timer_base_is);
    timer_base_is.TIM_Prescaler = 0xea60;              // 72M / 7500 = 9600 / 8
    timer_base_is.TIM_CounterMode = TIM_CounterMode_Up;
    timer_base_is.TIM_Period = RFM69_SYNCADDR_PERIOD;
    TIM_TimeBaseInit(TIM2, &timer_base_is);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}


int rfm69_init(void)
{
	int j;

    rfm69_mcu_init();

//	RFM69 initialization
	rfm69_write(REGOPMODE, REGOPMODE_DEF | STBY_MODE);
	rfm69_write(REGDATAMODUL, REGDATAMODUL_DEF);

	rfm69_write(REGFDEVMSB, REGFDEVMSB_DEF);
	rfm69_write(REGFDEVLSB, REGFDEVLSB_DEF);

	rfm69_write(REGBITRATEMSB, REGBITRATEMSB_DEF);
	rfm69_write(REGBITRATELSB, REGBITRATELSB_DEF);

	rfm69_write(REGFRFMSB, REGFRFMSB_DEF);
	rfm69_write(REGFRFMID, REGFRFMID_DEF);
	rfm69_write(REGFRFLSB, REGFRFLSB_DEF);

	rfm69_write(REGAFCCTRL, REGAFCCTRL_DEF);

//	rfm69_write(REGLISTEN1, REGLISTEN1_DEF);
//	rfm69_write(REGLISTEN2, REGLISTEN2_DEF);
//	rfm69_write(REGLISTEN3, REGLISTEN3_DEF);

	rfm69_write(REGPALEVEL, REGPALEVEL_DEF);
	rfm69_write(REGPARAMP, REGPARAMP_DEF);
	rfm69_write(REGOCP, REGOCP_DEF);
	rfm69_write(REGLNA, REGLNA_DEF);

	rfm69_write(REGRXBW, REGRXBW_DEF);
	rfm69_write(REGAFCBW, REGAFCBW_DEF);

//	rfm69_write(REGOOKPEAK, REGOOKPEAK_DEF);
//	rfm69_write(REGOOKAVG, REGOOKAVG_DEF);
//	rfm69_write(REGOOKFIX, REGOOKFIX_DEF);

	rfm69_write(REGAFCFEI, REGAFCFEI_DEF);

	rfm69_write(REGDIOMAPPING1, REGDIOMAPPING1_DEF);
	rfm69_write(REGDIOMAPPING2, REGDIOMAPPING2_DEF);

	rfm69_write(REGRSSITHRESH, REGRSSITHRESH_DEF);

	rfm69_write(REGPREAMBLEMSB, REGPREAMBLEMSB_DEF);
	rfm69_write(REGPREAMBLELSB, REGPREAMBLELSB_DEF);

	rfm69_write(REGSYNCCONFIG, REGSYNCCONFIG_DEF);
	rfm69_write(REGSYNCVALUE1, REGSYNCVALUE1_DEF);
	rfm69_write(REGSYNCVALUE2, REGSYNCVALUE2_DEF);
	rfm69_write(REGSYNCVALUE3, REGSYNCVALUE3_DEF);
	rfm69_write(REGSYNCVALUE4, REGSYNCVALUE4_DEF);
	rfm69_write(REGSYNCVALUE5, REGSYNCVALUE5_DEF);
	rfm69_write(REGSYNCVALUE6, REGSYNCVALUE6_DEF);
	rfm69_write(REGSYNCVALUE7, REGSYNCVALUE7_DEF);
	rfm69_write(REGSYNCVALUE8, REGSYNCVALUE8_DEF);

	rfm69_write(REGPACKETCONFIG1, REGPACKETCONFIG1_DEF);
	rfm69_write(REGPAYLOADLENGHT, REGPAYLOADLENGHT_DEF);
	rfm69_write(REGNODEADRS, REGNODEADRS_DEF);
	rfm69_write(REGBROADCASTADRS, REGBROADCASTADRS_DEF);
	rfm69_write(REGAUTOMODES, REGAUTOMODES_DEF);

	rfm69_write(REGFIFOTHRES, REGFIFOTHRES_DEF);
	rfm69_write(REGPACKETCONFIG2, REGPACKETCONFIG2_DEF);

//	rfm69_write(REGAESKEY1, REGAESKEY1_DEF);
//	rfm69_write(REGAESKEY2, REGAESKEY2_DEF);
//	rfm69_write(REGAESKEY3, REGAESKEY3_DEF);
//	rfm69_write(REGAESKEY4, REGAESKEY4_DEF);
//	rfm69_write(REGAESKEY5, REGAESKEY5_DEF);
//	rfm69_write(REGAESKEY6, REGAESKEY6_DEF);
//	rfm69_write(REGAESKEY7, REGAESKEY7_DEF);
//	rfm69_write(REGAESKEY8, REGAESKEY8_DEF);
//	rfm69_write(REGAESKEY9, REGAESKEY9_DEF);
//	rfm69_write(REGAESKEY10, REGAESKEY10_DEF);
//	rfm69_write(REGAESKEY11, REGAESKEY11_DEF);
//	rfm69_write(REGAESKEY12, REGAESKEY12_DEF);
//	rfm69_write(REGAESKEY13, REGAESKEY13_DEF);
//	rfm69_write(REGAESKEY14, REGAESKEY14_DEF);
//	rfm69_write(REGAESKEY15, REGAESKEY15_DEF);
//	rfm69_write(REGAESKEY16, REGAESKEY16_DEF);

	rfm69_write(REGTESTDAGC, AFC_LOW_BETA_OFF);

	rfm69_write(REGIRQFLAGS2, 1<<FIFOOVERRUN);

	for (j=0 ; j<99999 ; ++j);
	rfm69_receive_start();

	if ( rfm69_read(REGRXBW) != REGRXBW_DEF ) {     // spi check
		rfm69_condition = RFM69_SPI_FAILED;
		return -1;
	}
	else	return 0;
}


int rfm69_transmit_start(uint8_t packet_size_loc, uint8_t address)
{
	int i;

	packet_size = packet_size_loc;
	if(packet_size > RFM69_BUFFER_SIZE)	return -1;

	rfm69_condition = RFM69_TX;
	rfm69_write(REGOPMODE, REGOPMODE_DEF | TX_MODE);                        // turn on the transmitter

	rfm69_write(REGFIFO, packet_size+1);									// packet size to FIFO
	rfm69_write(REGFIFO, address);											// address to FIFO
	for(i=0 ; i<packet_size ; ++i)	rfm69_write(REGFIFO, packet_buffer[i]);	// packet to FIFO

	return 0;
}


void rfm69_receive_start(void)
{
	rfm69_condition = RFM69_RX;
	rfm69_write(REGOPMODE, REGOPMODE_DEF | RX_MODE);
}


int rfm69_receive_small_packet(void)
{
	int i;

	packet_size = rfm69_read(REGFIFO);
	if(packet_size == 0)				return -2;
	if(packet_size > RFM69_BUFFER_SIZE)	return -1;

	rfm69_read(REGFIFO);							// stripping off the address
	--packet_size;

	for(i=0 ; i<packet_size ; ++i) packet_buffer[i] = rfm69_read(REGFIFO);
//	rfm69_read_burst(REGFIFO, packet_buffer, packet_size);

	return packet_size;
}


void rfm69_sleep(void)
{
	rfm69_write(REGOPMODE, REGOPMODE_DEF | SLEEP_MODE);
	rfm69_condition = RFM69_SLEEP;
}


void rfm69_stby(void)
{
	rfm69_write(REGOPMODE, REGOPMODE_DEF | STBY_MODE);
	rfm69_condition = RFM69_STBY;
}


void rfm69_clear_fifo(void)
{
    int i;
    for(i=0 ; i<RFM69_BUFFER_SIZE ; ++i)   rfm69_read(REGFIFO);
    rfm69_write(REGIRQFLAGS2, 1<<FIFOOVERRUN);
}
