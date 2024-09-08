# LPPHOT03 Photometric Probe Library

The library provides a C API for interfacing with the LPPHOT03 photometric probe. It allows users to configure and read data from the photometric probe via UART, manage different operational modes, and retrieve various measurements such as internal temperature and illuminance. The driver is portable to any architecture and embedded platform, through the use of function pointers to define hardware dependent interfaces (UART, RS485 HW control).

## How to use the library

1. Include the library header files in your application source code.
   ```c
   #include "lpph.h"
   ```

2. Implement the UART write and read functions along with RS485 enable/disable functions required by `photometric_probe_obj`. Below is an example for an STM32 microcontroller using ST's HAL.
   ```c
   void uart_write(const uint8_t* buf, uint8_t len) {
       HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, HAL_MAX_DELAY);
   }

   void uart_read(uint8_t* buf, uint8_t len) {
       HAL_UART_Receive(&huart1, buf, len, HAL_MAX_DELAY);
   }

   void enable_transmission(void) {
       // Implement RS485 transmission enable
   }

   void disable_transmission(void) {
       // Implement RS485 transmission disable
   }
   ```

3. Create an instance of `photometric_probe_obj` and initialize it.
   ```c
   photometric_probe_obj probe;
   config_t cfg = {
       .address = 1,
       .baudrate = BAUDRATE_9600,
       .mode = MODE_8N1,
       .range = LOW_RANGE
   };

   probe.uart_write = &uart_write;
   probe.uart_read = &uart_read;
   probe.enable_transmission = &enable_transmission;
   probe.disable_transmission = &disable_transmission;

   photometric_probe_init(&probe, cfg);
   ```
   **Note: To configure the probe with specific configuration parameters (for the first time), use factory initialization API instead, and recycle the device afterwards, then use normal initialization API**
   ```c
   photometric_probe_factory_init(&probe, cfg);
   ```

5. Use the API to read temperature and illuminance values.
   ```c
   float temp_celsius = photometric_probe_read_internal_temperature_celsius(&probe);
   uint32_t lux = photometric_probe_read_illuminance(&probe);
   ```
   Or use Update_Measurements API.
   ```c
   photometric_probe_update_measurements(&probe);
   printf("%d", probe.illuminance);
   ```



## License

This project is licensed under the MIT License. 
