#include "DS18B20.h"
//#pragma once

#include <stdbool.h>


ds18b20_handle_t ds18b20;


void ds18b20_delay_us(ds18b20_handle_t *handle, uint16_t us )
{
 __HAL_TIM_SET_COUNTER(handle->tim_handle,0);
 while (__HAL_TIM_GET_COUNTER(handle->tim_handle)< us); 
}

/* TIM2 init function */
void ds18b20_tim_Init(ds18b20_handle_t  *handle_init)
{
  /* USER CODE BEGIN TIM2_Init 0 */
	
  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
	
  handle_init->tim_handle->Instance=TIM2;
  handle_init->tim_handle->Init.Prescaler = 32-1;
  handle_init->tim_handle->Init.CounterMode = TIM_COUNTERMODE_UP;
  handle_init->tim_handle->Init.Period = 0xffff-1;
  handle_init->tim_handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  handle_init->tim_handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  
	if (HAL_TIM_Base_Init(handle_init->tim_handle) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(handle_init->tim_handle, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(handle_init->tim_handle, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
	
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}


void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

uint8_t DS18B20_Start (ds18b20_handle_t  *handle_init)
{
	
	uint8_t Response = 0;
	Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);   // set the pin as output
	HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);  // pull the pin low
	ds18b20_delay_us ( handle_init, 480);   // delay according to datasheet

	Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);    // set the pin as input
	ds18b20_delay_us (handle_init,80);    // delay according to datasheet

	if (!(HAL_GPIO_ReadPin (DS18B20_PORT, DS18B20_PIN))) Response = 1;    // if the pin is low i.e the presence pulse is detected
	else Response = 0;

	ds18b20_delay_us (handle_init,400); // 480 us delay totally.

	return Response;
}

void DS18B20_Write (ds18b20_handle_t  *handle_init ,uint8_t data)
{
	Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);  // set as output

	for (int i=0; i<8; i++)
	{

		if ((data & (1<<i))!=0)  // if the bit is high
		{
			// write 1

			Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);  // set as output
			HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);  // pull the pin LOW
			ds18b20_delay_us (handle_init,1);  // wait for 1 us

			Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);  // set as input
			ds18b20_delay_us (handle_init,60);  // wait for 60 us
		}

		else  // if the bit is low
		{
			// write 0

			Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);
			HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);  // pull the pin LOW
			ds18b20_delay_us (handle_init,60);  // wait for 60 us

			Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);
		}
	}
}

uint8_t DS18B20_Read (ds18b20_handle_t  *handle_init)
{
	uint8_t value=0;

	Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);

	for (int i=0;i<8;i++)
	{
		Set_Pin_Output(DS18B20_PORT, DS18B20_PIN);   // set as output

		HAL_GPIO_WritePin (DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);  // pull the data pin LOW
		ds18b20_delay_us (handle_init,1);  // wait for > 1us

		Set_Pin_Input(DS18B20_PORT, DS18B20_PIN);  // set as input
		if (HAL_GPIO_ReadPin (DS18B20_PORT, DS18B20_PIN))  // if the pin is HIGH
		{
			value |= 1<<i;  // read = 1
		}
		ds18b20_delay_us (handle_init,60);  // wait for 60 us
	}
	return value;
}



ds18b20_data_t sensor_t;

void print_DS18B20(ds18b20_handle_t  *handle)
{
	
	uint8_t buf[35];
	 uint8_t stastus = 0;
	
	uint8_t Rh_byte1=0;
	uint8_t Rh_byte2=0;
	uint8_t	Temp_byte1=0;
uint8_t Temp_byte2=0;;
	uint16_t TEMP=0;
	
	DS18B20_Start(handle);
	
	//stastus = DS18B20_Start (handle);
		if(stastus == true)
		{
			HAL_Delay (1);
			DS18B20_Write (handle,0xCC);  // skip ROM
			DS18B20_Write (handle,0x44);  // convert t
			HAL_Delay (800);
		}
		else
		{
			sprintf((char*)buf,"convert Fail \r\n");
		//	HAL_UART_Transmit(handle->uart_handle,buf,sizeof(buf),1000);
			HAL_Delay(10);
		}
		
	  stastus = DS18B20_Start (handle);
		if(stastus==true)
		{
   		   	HAL_Delay(1);
  			DS18B20_Write (handle,0xCC);  // skip ROM
   		 	DS18B20_Write (handle,0xBE);  // Read Scratch-pad

   			Temp_byte1 = DS18B20_Read(handle);
	 	 	Temp_byte2 = DS18B20_Read(handle);
		  	TEMP = (Temp_byte2<<8)|Temp_byte1;
			sensor_t.Temperature = (float)TEMP/16;
			
			sprintf((char*)buf,"DS18B20 Temperature : %3.3f \r\n",sensor_t.Temperature);
		//	HAL_UART_Transmit(handle->uart_handle,buf,sizeof(buf),1000);
		}
		else
		{
			sprintf((char*)buf,"Read Fail \r\n");
		//	HAL_UART_Transmit(handle->uart_handle,buf,sizeof(buf),1000);
			HAL_Delay(10);
		}
		
}

