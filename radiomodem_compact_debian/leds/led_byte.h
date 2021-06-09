#ifndef LED_BYTE_H_INCLUDED
#define LED_BYTE_H_INCLUDED

/*** MCU peripherial defines *****/

#define LED_BYTE_PORT       GPIOA

#define LED_BYTE_CLK_PIN    GPIO_Pin_11
#define LED_BYTE_DI_PIN     GPIO_Pin_12

#define LED_BYTE_CLK_DIV    1
/*********************************/

/* Internal functions */
inline void nextClock(void);

/* Userful functions  */

void led_byte_init(void); // init peripherial
void updateByte(uint8_t led_byte); // fill led byte with appropriate data


#endif // LED_BYTE_H_INCLUDED
