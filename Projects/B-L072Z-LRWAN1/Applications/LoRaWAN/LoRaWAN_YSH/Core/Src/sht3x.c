#include "sht3x.h"
#pragma once

#include <stdbool.h>

#if (defined STM32L011xx) || (defined STM32L021xx) || \
	(defined STM32L031xx) || (defined STM32L041xx) || \
	(defined STM32L051xx) || (defined STM32L052xx) || (defined STM32L053xx) || \
	(defined STM32L061xx) || (defined STM32L062xx) || (defined STM32L063xx) || \
	(defined STM32L071xx) || (defined STM32L072xx) || (defined STM32L073xx) || \
	(defined STM32L081xx) || (defined STM32L082xx) || (defined STM32L083xx)
#include "stm32l0xx_hal.h"
#elif defined (STM32L412xx) || defined (STM32L422xx) || \
	defined (STM32L431xx) || (defined STM32L432xx) || defined (STM32L433xx) || defined (STM32L442xx) || defined (STM32L443xx) || \
	defined (STM32L451xx) || defined (STM32L452xx) || defined (STM32L462xx) || \
	defined (STM32L471xx) || defined (STM32L475xx) || defined (STM32L476xx) || defined (STM32L485xx) || defined (STM32L486xx) || \
    defined (STM32L496xx) || defined (STM32L4A6xx) || \
    defined (STM32L4R5xx) || defined (STM32L4R7xx) || defined (STM32L4R9xx) || defined (STM32L4S5xx) || defined (STM32L4S7xx) || defined (STM32L4S9xx)
#include "stm32l4xx_hal.h"
#else
#error Platform not implemented
#endif

#ifndef SHT3X_I2C_TIMEOUT
#define SHT3X_I2C_TIMEOUT 30
#endif

#define SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW 0x44
#define SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_HIGH 0x45

#include <assert.h>

#include "sys_app.h"

sht3x_value_t sht_3x_value;

extern I2C_HandleTypeDef hi2c1;
/**
 * Registers addresses.
 */
typedef enum
{
	SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH = 0x2c06,
	SHT3X_COMMAND_CLEAR_STATUS = 0x3041,
	SHT3X_COMMAND_SOFT_RESET = 0x30A2,
	SHT3X_COMMAND_HEATER_ENABLE = 0x306d,
	SHT3X_COMMAND_HEATER_DISABLE = 0x3066,
	SHT3X_COMMAND_READ_STATUS = 0xf32d,
	SHT3X_COMMAND_FETCH_DATA = 0xe000,
	SHT3X_COMMAND_MEASURE_HIGHREP_10HZ = 0x2737,
	SHT3X_COMMAND_MEASURE_LOWREP_10HZ = 0x272a
} sht3x_command_t;

static uint8_t calculate_crc(const uint8_t *data, size_t length)
{
	uint8_t crc = 0xff;
	for (size_t i = 0; i < length; i++) {
		crc ^= data[i];
		for (size_t j = 0; j < 8; j++) {
			if ((crc & 0x80u) != 0) {
				crc = (uint8_t)((uint8_t)(crc << 1u) ^ 0x31u);
			} else {
				crc <<= 1u;
			}
		}
	}
	return crc;
}

static bool sht3x_send_command(sht3x_handle_t *handle, sht3x_command_t command)
{
	uint8_t command_buffer[2] = {(command & 0xff00u) >> 8u, command & 0xffu};

	if (HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address << 1u, command_buffer, sizeof(command_buffer),
	                            SHT3X_I2C_TIMEOUT) != HAL_OK) {
		return false;
	}

	return true;
}

static uint16_t uint8_to_uint16(uint8_t msb, uint8_t lsb)
{
	return (uint16_t)((uint16_t)msb << 8u) | lsb;
}

bool sht3x_init(sht3x_handle_t *handle)
{
	//assert(handle->i2c_handle->Init.NoStretchMode == I2C_NOSTRETCH_DISABLE);
	// TODO: Assert i2c frequency is not too high

//	if (handle->i2c_handle->Init.NoStretchMode == I2C_NOSTRETCH_DISABLE)
//	{
//		return false;
//	}
	
	uint8_t status_reg_and_checksum[3];
	if (HAL_I2C_Mem_Read(handle->i2c_handle, handle->device_address << 1u, SHT3X_COMMAND_READ_STATUS, 2, (uint8_t*)&status_reg_and_checksum,
					  sizeof(status_reg_and_checksum), SHT3X_I2C_TIMEOUT) != HAL_OK) {
							APP_PRINTF("i2c not connect");
		return false;
	}
	APP_PRINTF("i2c connect\r\n \r\n");
	uint8_t calculated_crc = calculate_crc(status_reg_and_checksum, 2);

	if (calculated_crc != status_reg_and_checksum[2]) {
		return false;
	}

	return true;
}

bool sht3x_read_temperature_and_humidity(sht3x_handle_t *handle, float *temperature, float *humidity)
{
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

//	HAL_Delay(1);

	uint8_t buffer[6];
	if (HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), SHT3X_I2C_TIMEOUT) != HAL_OK) {
		APP_PRINTF("not receive\n\r");
		return false;
	}
	

	uint8_t temperature_crc = calculate_crc(buffer, 2);
	uint8_t humidity_crc = calculate_crc(buffer + 3, 2);
	if (temperature_crc != buffer[2] || humidity_crc != buffer[5]) {
		return false;
	}

	int16_t temperature_raw = (int16_t)uint8_to_uint16(buffer[0], buffer[1]);
	uint16_t humidity_raw = uint8_to_uint16(buffer[3], buffer[4]);

	*temperature = -45.0f + 175.0f * (float)temperature_raw / 65535.0f;
	*humidity = 100.0f * (float)humidity_raw / 65535.0f;

	return true;
}

bool sht3x_set_header_enable(sht3x_handle_t *handle, bool enable)
{
	if (enable) {
		return sht3x_send_command(handle, SHT3X_COMMAND_HEATER_ENABLE);
	} else {
		return sht3x_send_command(handle, SHT3X_COMMAND_HEATER_DISABLE);
	}
}

uint8_t Get_sht_temperature_raw(sht3x_handle_t *handle)
{
	
	uint8_t buffer[6];
	int16_t temperature_raw = (int16_t)uint8_to_uint16(buffer[0], buffer[1]);
	uint8_t temperature_crc;

	uint8_t temperature=0;

	//sht3x_init(handle);
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

//	HAL_Delay(1);

	if (HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), SHT3X_I2C_TIMEOUT) != HAL_OK)
	 {
		if(SHT3X_DEBUG==true)
		{
			APP_PRINTF("not receive\n\r");
		}
		return false;
	}

	temperature_crc = calculate_crc(buffer, 2);

	if (temperature_crc != buffer[2]) {
		return false;
	}
	
	temperature = (-45.0f + 175.0f * (float)temperature_raw / 65535.0f);
	if(SHT3X_DEBUG==true)
	{	
		APP_LOG(TS_ON, VLEVEL_L, "SHT TEMP : %d \r\n",temperature);
	}
	return temperature;
}

float Get_sht_humidity_raw(sht3x_handle_t *handle)
{
//	float humidity;
	uint16_t humidity_raw =0;
	uint8_t humidity_crc ;
	uint8_t buffer[6];
	
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

	//HAL_Delay(1);

	
	if (HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), SHT3X_I2C_TIMEOUT) != HAL_OK) {
		if(SHT3X_DEBUG==true)
	{	
		APP_PRINTF("not receive\n\r");
	}
		return false;
	}
	

	 humidity_crc = calculate_crc(buffer + 3, 2);
	if ( humidity_crc != buffer[5]) 
	{
		return false;
	}


	humidity_raw = uint8_to_uint16(buffer[3], buffer[4]);

	
//	humidity = 100.0f * (float)humidity_raw / 65535.0f;

	return humidity_raw;
}

void RUN_SHT_30(sht3x_handle_t *handle)
{

		uint8_t SHT_30_buf;
	
		 handle->i2c_handle = &hi2c1 ;
		handle->device_address= SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW ;
  
		float temperature, humidity;

		if(sht3x_read_temperature_and_humidity(handle, &temperature, &humidity)==true)
		{
			APP_LOG(TS_ON, VLEVEL_L, "SHT-30 TEMP : %d \n\r",(char) temperature);
			APP_LOG(TS_ON, VLEVEL_L, "SHT-30 HUM: %d \r\n",(char) humidity)
		}
		else
		APP_PPRINTF("SHT NOT READ\r\n");

}

bool LoRa_sht3x_read_temperature_and_humidity(sht3x_handle_t *handle,sht3x_value_t *value)
{
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

//	HAL_Delay(1);

	uint8_t buffer[6];
	if (HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), SHT3X_I2C_TIMEOUT) != HAL_OK) {
		APP_PRINTF("not receive\n\r");
		return false;
	}
	

	uint8_t temperature_crc = calculate_crc(buffer, 2);
	uint8_t humidity_crc = calculate_crc(buffer + 3, 2);
	if (temperature_crc != buffer[2] || humidity_crc != buffer[5]) {
		return false;
	}
	value->SHT_30_Temperature_H = buffer[0];
	value->SHT_30_Temperature_L = buffer[1];
	value->SHT_30_Humidity_H = buffer[3];
	value->SHT_30_Humidity_L = buffer[4];
	
	if(SHT3X_DEBUG==true)
	{	
//	APP_LOG(TS_ON, VLEVEL_L, "sht_temp_H= %d\n\r",buffer[0] );
//	APP_LOG(TS_ON, VLEVEL_L, "sht_temp_L= %d\n\r",buffer[1] );
	

	int16_t temperature_raw = (int16_t)uint8_to_uint16(buffer[0], buffer[1]);
	uint16_t humidity_raw = uint8_to_uint16(buffer[3], buffer[4]);

	float temperature = -45.0f + 175.0f * (float)temperature_raw / 65535.0f;
	float humidity = 100.0f * (float)humidity_raw / 65535.0f;
	//APP_LOG(TS_ON, VLEVEL_L, "sht_temp= %d\n\r",(uint8_t) temperature );
	
	}
	
	return true;

}
