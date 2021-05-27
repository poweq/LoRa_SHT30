
#include "stm32l0xx_hal.h"
//#include "gpio.h"
//#include "tim.h"
//#include "usart.h"


#define DS18B20_PORT 					GPIOA
#define DS18B20_PIN 					GPIO_PIN_0

/**
 * Structure defining a handle describing a DS18B20 device.
 */
typedef struct {

	/**
	 * The handle to the I2C bus for the device.
	 */
	UART_HandleTypeDef *uart_handle;
  TIM_HandleTypeDef  *tim_handle;
    
} ds18b20_handle_t;

typedef struct {

	float Temperature;

}ds18b20_data_t;


void ds18b20_delay_us(ds18b20_handle_t *handle, uint16_t us );

void ds18b20_tim_Init(ds18b20_handle_t  *handle_init);

uint8_t DS18B20_Start (ds18b20_handle_t  *handle_init);

void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

void print_DS18B20(ds18b20_handle_t  *handle);

void DS18B20_Write (ds18b20_handle_t  *handle_init ,uint8_t data);
uint8_t DS18B20_Read (ds18b20_handle_t  *handle_init);

