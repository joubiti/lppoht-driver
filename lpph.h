/**
 * @file lpph.h
 * @author joubiti (github.com/joubiti)
 * @brief This file contains the implementation of the driver for LPPHOT03 photometric probe
 * @version 0.1
 * @date 2024-09-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include "stdint.h"

/**
 * @brief List of allowable baudrates for LPPHOT03 probe
 * 
 */
typedef enum{
    BAUDRATE_9600,
    BAUDRATE_19200,
    BAUDRATE_38400,
    BAUDRATE_57600,
    BAUDRATE_115200
}baudrate_e;

/**
 * @brief UART transmission modes
 * 
 */
typedef enum{
    MODE_8N1,
    MODE_8N2,
    MODE_8E1,
    MODE_8E2,
    MODE_8O1,
    MODE_802,
}transmission_mode_e;

/**
 * @brief Low range: (0 -> 20 000 Lux, 1 Lux resolution), High Range: (0 -> 200 000 Lux, 10 Lux resolution)
 * 
 */
typedef enum{
    LOW_RANGE,
    HIGH_RANGE
}photometric_range_e;

/**
 * @brief Status of probe
 * 
 */
typedef enum{
    STATUS_OK,
    STATUS_ERR
}probe_status_e;

/**
 * @brief Configuration structure for probe parameters
 * 
 */
typedef struct{
    uint8_t address; // device address (from 1 to 247)
    baudrate_e baudrate; // from 9600 to 115200
    transmission_mode_e mode;
    photometric_range_e range; // low or high
}config_t;

/**
 * @brief Structure for a photometric probe object, must provide API for uart_write and uart_read in the application code as well as RS485 Enable pin control API
 * 
 */
typedef struct{
    void(*uart_write)(const uint8_t* buf, uint8_t len);
    void(*uart_read)(uint8_t* buf, uint8_t len);
    void(*enable_transmission)(void);
    void(*disable_transmission)(void);
    float internal_temp_celsius;
    float internal_temp_fahrenheit;
    uint32_t illuminance;
    uint32_t avg_illuminance;
    config_t cfg;
}photometric_probe_obj;

/**
 * @brief Initializes (Factory) LPPHOT03 photometric probe with the given configuration parameters 
 * @note This function should only be called once, after detecting whether the device has previously been configured or not, 
 * this can be a flag set by the application and stored in non volatile storage. If device has already been configured preivously, then the non factory init API must be used.
 * 
 * @param obj: A pointer to a photometric probe object
 * @param cfg: A copy of the configuration structure
 * @return probe_status_e
 * @retval STATUS_OK if configuration successfull
 * @retval STATUS_ERR if configuration unsuccessfull
 */
probe_status_e photometric_probe_factory_init(photometric_probe_obj* obj, config_t cfg);

/**
 * @brief Initializes probe object
 * 
 * @param obj: A pointer to a photometric probe object
 * @return None
 */
void photometric_probe_init(photometric_probe_obj* obj, config_t cfg);

/**
 * @brief Reads internal probe temperature in Celsius
 * 
 * @param obj: A pointer to a photometric probe object
 * @return float 
 */
float photometric_probe_read_internal_temperature_celsius(photometric_probe_obj* obj);

/**
 * @brief Reads internal probe temperature in Fahrenheit
 * 
 * @param obj: A pointer to a photometric probe object 
 * @return float 
 */
float photometric_probe_read_internal_temperature_fahrenheit(photometric_probe_obj* obj);

/**
 * @brief Reads illuminance in Lux (0 -> 200 000 Lux depending on range)
 * 
 * @param obj 
 * @return uint32_t 
 */
uint32_t photometric_probe_read_illuminance(photometric_probe_obj* obj);

/**
 * @brief Updates illuminance and internal temperature measurements 
 * 
 * @param obj: A pointer to a photometric probe object
 * @retval STATUS_OK if measurements succesfully updated
 * @retval STATUS_ERR if CRC invalid
 */
probe_status_e photometric_probe_update_measurements(photometric_probe_obj* obj);


