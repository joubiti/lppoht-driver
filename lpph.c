/**
 * @file lpph.c
 * @author joubiti (github.com/joubiti)
 * @brief This file contains the implementation of the driver for LPPHOT03 photometric probe
 * @version 0.1
 * @date 2024-09-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "lpph.h"
#include "stdio.h"
#include "string.h"


/**
 * @brief Calculates CRC of Modbus frame
 * 
 * @param buf: buffer array
 * @param len: length of buffer
 * @return uint16_t
 */
static uint16_t ModRTU_CRC(uint8_t* buf, int len);


/**
 * @brief Validates CRC of frame
 * 
 * @param buf: buffer array
 * @param size: length of buffer
 * @return probe_status_e 
 * @retval STATUS_OK if CRC valid
 * @retval STATUS_ERR if CRC invalid
 */
static probe_status_e crc_check(uint8_t* buf, uint8_t size);


#define CELSIUS_TEMP_ADDR           0x00
#define FAHRENHEIT_TEMP_ADDR        0x01
#define ILLUMINANCE_ADDR            0x02

/**
 * @brief Reads a Modbus register
 * 
 * @param obj: pointer to probe object
 * @param reg_addr: address of holding register
 * @param buf: buffer to which response frame will be copied
 * @return probe_status_e
 * @retval STATUS_ERR if CRC not OK
 * @retval STATUS_OK if register successfully read
 */
static probe_status_e read_register(photometric_probe_obj* obj, uint8_t reg_addr, uint8_t* buf);



probe_status_e photometric_probe_factory_init(photometric_probe_obj* obj, config_t cfg){
    // at power up, entering user configuration mode
    obj->uart_write((const uint8_t*) "@", 1);
    obj->uart_write((const uint8_t*) "CAL USER ON", 11);
    // configuring device address
    char bdrate_cfg[10];
    sprintf(bdrate_cfg, "CMA%03d", cfg.address);
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) bdrate_cfg, strlen(bdrate_cfg));
    obj->disable_transmission();
    // configuring device baudrate
    char param_cfg[8];
    sprintf(param_cfg, "CMB%d", cfg.baudrate);
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) param_cfg, strlen(param_cfg));
    obj->disable_transmission();
    // configuring UART transmission mode
    sprintf(param_cfg, "CMP%d", cfg.mode);
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) param_cfg, strlen(param_cfg));
    obj->disable_transmission();
    // verify parameters
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) "RMA", 3);
    obj->disable_transmission();
    uint8_t rsp;
    obj->uart_read(&rsp, 1);
    if(rsp != cfg.address){
        return STATUS_ERR;
    }
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) "RMB", 3);
    obj->disable_transmission();
    obj->uart_read(&rsp, 1);
    if(rsp != cfg.baudrate){
        return STATUS_ERR;
    }
    obj->enable_transmission();
    obj->uart_write((const uint8_t*) "RMP", 3);
    obj->disable_transmission();
    obj->uart_read(&rsp, 1);
    if(rsp != cfg.mode){
        return STATUS_ERR;
    }
    obj->cfg = cfg;
    return STATUS_OK;
}

void photometric_probe_init(photometric_probe_obj* obj, config_t cfg){
    // sets parameters to 0
    obj->avg_illuminance = 0;
    obj->illuminance = 0;
    obj->internal_temp_celsius = 0;
    obj->internal_temp_fahrenheit = 0;
    // set configuration
    obj->cfg = cfg;
}

float photometric_probe_read_internal_temperature_celsius(photometric_probe_obj* obj){
    uint8_t rxBuf[7] = {};
    if(read_register(obj, CELSIUS_TEMP_ADDR, rxBuf) == STATUS_ERR){
        return 0;
    }
	// Decode temperature
	uint16_t tmp = 0;
	tmp = rxBuf[3] << 8;
	tmp |= rxBuf[4];
	float temperature = ((float) tmp)/10;
	return temperature;
}

float photometric_probe_read_internal_temperature_fahrenheit(photometric_probe_obj* obj){
    uint8_t rxBuf[7] = {};
    if(read_register(obj, FAHRENHEIT_TEMP_ADDR, rxBuf) == STATUS_ERR){
        return 0;
    }
	// Decode temperature
	uint16_t tmp = 0;
	tmp = rxBuf[3] << 8;
	tmp |= rxBuf[4];
	float temperature = ((float) tmp)/10;
	return temperature;
}

uint32_t photometric_probe_read_illuminance(photometric_probe_obj* obj){
    uint8_t rxBuf[7] = {};
    if(read_register(obj, ILLUMINANCE_ADDR, rxBuf) == STATUS_ERR){
        return 0;
    }
	// Decode illuminance
	uint16_t tmp = 0;
	tmp = rxBuf[3] << 8;
	tmp |= rxBuf[4];
    uint32_t illuminance = 0;
    switch(obj->cfg.range){
        case LOW_RANGE:
            illuminance = tmp;
            break;
        case HIGH_RANGE:
            illuminance = tmp * 10;
            break;
        default:
            break;
    }
	return illuminance;
}


probe_status_e photometric_probe_update_measurements(photometric_probe_obj* obj){
    obj->internal_temp_celsius = photometric_probe_read_internal_temperature_celsius(obj);
    obj->internal_temp_fahrenheit = photometric_probe_read_internal_temperature_fahrenheit(obj);
    obj->illuminance = photometric_probe_read_illuminance(obj);
    if((obj->internal_temp_celsius == 0) || (obj->internal_temp_fahrenheit == 0) || (obj->illuminance == 0)){
        return STATUS_ERR;
    }
    return STATUS_OK;
}


static uint16_t ModRTU_CRC(uint8_t* buf, int len){
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
        crc >>= 1;
    }
  }
  return crc;
}


static probe_status_e crc_check(uint8_t* buf, uint8_t size){
	uint16_t crc_to_verify = 0;
	crc_to_verify = ModRTU_CRC(buf, size - 2);
	uint8_t lb_crc = crc_to_verify & 0xFF;
	uint8_t hb_crc = crc_to_verify >> 8;
	if(buf[size - 2] == lb_crc && buf[size - 1] == hb_crc){
		return STATUS_OK;
	}
	return STATUS_ERR;
}


static probe_status_e read_register(photometric_probe_obj* obj, uint8_t reg_addr, uint8_t* buf){
	uint8_t buffer[8] = {obj->cfg.address, 0x04, 0x00, reg_addr, 0x00, 0x01};
	uint16_t crc = ModRTU_CRC(buffer, 6);
	uint8_t crc_low_byte = crc & 0xFF;
	uint8_t crc_high_byte = crc >> 8;
	buffer[6] = crc_low_byte;
	buffer[7] = crc_high_byte;
	obj->enable_transmission();
    obj->uart_write((const uint8_t*) buffer, 8);
	obj->disable_transmission();
	// Receiving buffer will be 7 bytes long, (1 * 2) + 5 initial bytes
	uint8_t rxBuf[7] = {};
    obj->uart_read(rxBuf, 7);
	if(crc_check(rxBuf, 7) != STATUS_OK){
		return STATUS_ERR;
	}
    for(uint8_t j = 0; j < 7 ; j ++){
        buf[j] = rxBuf[j];
    }
    return STATUS_OK;
}