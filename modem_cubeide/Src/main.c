
#include "stm32f10x_conf.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"

#include "rfm69.h"

extern uint8_t USB_Rx_Buffer[], packet_buffer[64], rfm69_condition, packet_size;
uint8_t internal_packet_buffer[64], internal_pack_size;
void SetSysClockTo72(void);



void SetSysClockTo72()
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/
  /* Enable HSE */
  RCC->CR |= ((uint32_t)RCC_CR_HSEON | RCC_CR_HSEBYP);

  /* Wait till HSE is ready and if Time out is reached exit */
  do
  {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
  } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET)
  {
    HSEStatus = (uint32_t)0x01;
  }
  else
  {
    HSEStatus = (uint32_t)0x00;
  }

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTBE;

    /* Flash 2 wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;


    /* HCLK = SYSCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

    /* PCLK2 = HCLK */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;

    /* PCLK1 = HCLK/2 */
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;


    /*  PLL configuration: PLLCLK = HSE * 9 = 72 MHz */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);
//    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_PREDIV1 | RCC_CFGR_PLLMULL9);


    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

    /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock
         configuration. User can add here some code to deal with this error */
  }
}



int main(void)
{
    int i;

	RCC->CR |= RCC_CR_HSION;            // set system clock from internal oscillator
	while(!(RCC->CR & RCC_CR_HSIRDY));
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_HSI;

	for(i=0 ; i<99999 ; ++i);
	rfm69_init();

	SetSysClockTo72();                  // set sys clock from RFM69

	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();

	while(1)
	{
/*        afcm = rfm69_read(REGAFCMSB);
        afcl = rfm69_read(REGAFCLSB);
        regirq = rfm69_read(REGIRQFLAGS1);

        if( (afcl != prevafcl) || (afcm != prevafcm) || (regirq != prevregirq) )
        {
            internal_packet_buffer[internal_pack_size++] = prevafcm = afcm;
            internal_packet_buffer[internal_pack_size++] = prevafcl = afcl;
            internal_packet_buffer[internal_pack_size++] = prevregirq = regirq;
            internal_packet_buffer[internal_pack_size++] = rfm69_read(REGIRQFLAGS2);
            internal_packet_buffer[internal_pack_size++] = rfm69_read(REGOPMODE);
            internal_packet_buffer[internal_pack_size++] = 0xee;
            internal_packet_buffer[internal_pack_size++] = 0xee;
            internal_packet_buffer[internal_pack_size++] = 0xee;
        } */
    }
}



#ifdef USE_FULL_ASSERT
void assert_failed ( uint8_t * file , uint32_t line)
{
// Infinite loop
// Use GDB to find out why we're here
	while (1);
}
#endif
