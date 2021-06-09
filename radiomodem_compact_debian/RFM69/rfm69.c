#include "stm32f10x_conf.h"

#include "rfm69.h"

/** @mainpage
	Библиотека осуществляет пакетную передачу данных с помощью радиомодулей RFM69CW.

	Для её использования необходимо подключить файл rfm69.h и вызвать функцию rfm69_init(). После чего можно отправлять 
	данные с помощью функции rfm69_transmit_start(), а также принимать данные. Обработка принятых пакетов осуществляется 
	автоматически, вам достаточно отслеживать состояние переменной @ref rfm69_condition. Если ее значение равно @ref RFM69_NEW_PACK,
	значит пакет был успешно принят, далее можно запустить функцию rfm69_receive_start() для приема следующего пакета по 
	радиоканалу. При этом после осуществления передачи пакета приемник включается автоматически. Пакет находится в 
	глобальном массиве @ref packet_buffer[], а его размер в глобальной переменной @ref packet_size. Настройки радиомодуля задаются 
	макросами и находятся в файле rfm69.h.

 @file rfm69.c @brief
	В этом файле находятся функции библиотеки RFM69CW.
*/

/* Глобальные переменные необходимые для работы радиомодуля */
uint8_t packet_buffer[RFM69_BUFFER_SIZE - 2];	///< буфер, в котором хранится пакет
uint8_t rfm69_condition;						///< в ней хранится состояние радиомодуля на данный момент
uint8_t packet_size;							///< размер пакета

extern uint8_t internal_packet_buffer[64], internal_pack_size;

/** 
	@function
	Обработчик внешнего прерывания. Запускается либо после успешной передачи, либо после успешного приема пакета.
*/
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

/** 
	@function
	Обработчик внешнего прерывания. Запускается по событию переполнения FIFO, 
	может применяться для приема пакетов размером > 64 байт. В данном варианте программы не используется. 
*/
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

/** 
	@function
	Обработчик внешнего прерывания. Запускается после превышения порогового значения RSSI при включенном приемнике.
	В данном варианте программы не используется.
*/
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
            break;
        case RFM69_TX :
            break;
    }
    EXTI_ClearITPendingBit(SyncAddr_Line);
}


/** 
	@function
	Запись регистра радиомодуля. Осуществляется по шине SPI.
	@param address - адрес регистра
	@param data	- данные, записываемые в регистр 
*/
void rfm69_write(uint8_t address, uint8_t data)
{
	__disable_irq();
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // ожидание пока SPI занят

	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, (address | 0x80));                               // отправка адреса регистра
    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );

    SPI_I2S_SendData(SPI1, data);                                           // отправка данных	

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);												// чтение данных в никуда

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);												// чтение данных в никуда

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // ожидание пока SPI занят
	GPIO_SetBits(NSS_Port, NSS_Pin);

	__enable_irq();
}

/** 
	@function
	Чтение регистра радиомодуля. Осуществляется по шине SPI.
	@param address - адрес регистра
	@return данные, считанные из регистра 
*/
uint8_t rfm69_read(uint8_t address)
{
	uint8_t data = 0x00;

	__disable_irq();
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // ожидание пока SPI занят

	GPIO_ResetBits(NSS_Port, NSS_Pin);

    SPI_I2S_SendData(SPI1, (address));                                      // отправка адреса
    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );

    SPI_I2S_SendData(SPI1, 0x87);                                           // отправка сдучайных данных

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    SPI_I2S_ReceiveData(SPI1);                                              // чтение полученных данных в никуда

    while ( (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) && \
            (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) );
    data = SPI_I2S_ReceiveData(SPI1);                                       // чтение полезных данных

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);           // ожидание пока SPI занят
	GPIO_SetBits(NSS_Port, NSS_Pin);

	__enable_irq();
	return data;
}

/** 
	@function
	Запись массива регистров, или запись массива в FIFO. Осуществляется по шине SPI. Работает плохо, использовать не рекомендуется.
	@param address - начальный адрес
	@param data - указатель на массив даных
	@param ndata - количество данных в массиве	 
*/
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

/** 
	@function
	Чтение массива регистров, или чтение массива из FIFO. Осуществляется по шине SPI. Работает плохо, использовать не рекомендуется.
	@param address - начальный адрес
	@param data - указатель на массив даных
	@param ndata - количество данных в массиве	 
*/
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


/** 
	@function
	Здесь осуществляется инициализация периферии микроконтроллера. А именно: включение тактирования периферии, 
	конфигурация портов ввода-вывода, конфигурация SPI, таймера и настройка внешних прерываний. 	 
*/
void rfm69_mcu_init(void)
{
//	Включение тактирования
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);		

//	Инициализация SPI
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

//	Настройка внешних прерываний
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
}

/** 
	@function
	Здесь осуществляется инициализация радиомодуля. Осуществляется путем передачи регистров, вычисленных при помощи макросов 
	из rfm69.h, в радиомодуль. Для того чтобы начать работать с радиомодулем, достаточно запустить эту функцию. Также в ней
	проверяется интерфейс, если SPI не работает, переменной состояния радиомодуля присваивается значение RFM69_SPI_FAILED.
	@return отрицательное значение, если интерфейс SPI не работает, иначе ноль.
*/
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

	if ( rfm69_read(REGRXBW) != REGRXBW_DEF ) {     // проверка SPI
		rfm69_condition = RFM69_SPI_FAILED;
		return -1;
	}
	else	return 0;
}

/** 
	@function
	Запуск передачи пакета.
	@param packet_size_loc - размер передаваемого пакета
	@param address - адрес устройства, которому необходимо  передать пакет
	При этом подразумевается что передаваемые данные должны находиться в глобальном массиве packet_buffer.
	@return отрицательное значение, если размер пакета слишком большой, иначе ноль
*/
int rfm69_transmit_start(uint8_t packet_size_loc, uint8_t address)
{
	int i;

	packet_size = packet_size_loc;
	if(packet_size > RFM69_BUFFER_SIZE)	return -1;							// проверка размера пакета

	rfm69_condition = RFM69_TX;
	rfm69_write(REGOPMODE, REGOPMODE_DEF | TX_MODE);                        // включить передатчик

	rfm69_write(REGFIFO, packet_size+1);									// передача размера пакета в FIFO
	rfm69_write(REGFIFO, address);											// передача адреса в FIFO
	for(i=0 ; i<packet_size ; ++i)	rfm69_write(REGFIFO, packet_buffer[i]);	// передача самого пакета в FIFO

	return 0;
}

/** 
	@function
	Запуск приемника.
*/
void rfm69_receive_start(void)
{
	rfm69_condition = RFM69_RX;
	rfm69_write(REGOPMODE, REGOPMODE_DEF | RX_MODE);
}

/** 
	@function
	Функция осуществляет чтение принятого пакета. Данные считываются в глобальный массив packet_buffer.
	@return размер считанного пакета, а если размер пакета превышает размер буфера возвращает -1
*/
int rfm69_receive_small_packet(void)
{
	int i;

	packet_size = rfm69_read(REGFIFO);										// считывание размера пакета
	if(packet_size > RFM69_BUFFER_SIZE)	return -1;							// проверка его размера

	rfm69_read(REGFIFO);													// отбрасывается адрес
	--packet_size;	

	for(i=0 ; i<packet_size ; ++i) packet_buffer[i] = rfm69_read(REGFIFO);	// чтение пакета из FIFO

	return packet_size;
}

/**
	@function
	Переключает радиомодуль в спящий режим.
*/
void rfm69_sleep(void)
{
	rfm69_write(REGOPMODE, REGOPMODE_DEF | SLEEP_MODE);
	rfm69_condition = RFM69_SLEEP;
}

/**
	@function
	Переключает радиомодуль в режим standby (режим ожидания).
*/
void rfm69_stby(void)
{
	rfm69_write(REGOPMODE, REGOPMODE_DEF | STBY_MODE);
	rfm69_condition = RFM69_STBY;
}

/**
	@function
	Обнуляет буфер радиомодуля.
*/
void rfm69_clear_fifo(void)
{
    int i;
    for(i=0 ; i<RFM69_BUFFER_SIZE ; ++i)   rfm69_read(REGFIFO);
    rfm69_write(REGIRQFLAGS2, 1<<FIFOOVERRUN);
}
