#include "stm32f10x_conf.h"
#include "leds/led_byte.h"


inline void nextClock(void)
{
    GPIO_SetBits(LED_BYTE_PORT,LED_BYTE_CLK_PIN);

    GPIO_ResetBits(LED_BYTE_PORT,LED_BYTE_CLK_PIN);
}


void led_byte_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = LED_BYTE_CLK_PIN | LED_BYTE_DI_PIN;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(LED_BYTE_PORT, &gpio);

    GPIO_ResetBits(LED_BYTE_PORT, LED_BYTE_CLK_PIN | LED_BYTE_DI_PIN);
    updateByte(0x00);
}


void updateByte(uint8_t led_byte)
{
    int i;

    for( i = 0 ; i < 8 ; ++i )  {
        if( led_byte & (1<<i) )
            GPIO_SetBits(LED_BYTE_PORT,LED_BYTE_DI_PIN);
        else
            GPIO_ResetBits(LED_BYTE_PORT,LED_BYTE_DI_PIN);

        nextClock();
    }

    GPIO_ResetBits(LED_BYTE_PORT, LED_BYTE_DI_PIN);
    nextClock();
}
