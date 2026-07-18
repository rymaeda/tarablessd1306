/**
 * Copyright (c) 2017-2018 Tara Keeling
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 *
 * Updated for ESP-IDF 5.x / 6.x new I2C master driver API.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "ssd1306.h"
#include "ssd1306_default_if.h"

/*
 * HACKHACKHACK:
 * Conditional compiling in component.mk does not seem to be working.
 * This workaround looks ugly, but should work.
 */
#if defined CONFIG_SSD1306_ENABLE_DEFAULT_I2C_INTERFACE

static const int I2CDisplaySpeed = CONFIG_SSD1306_DEFAULT_I2C_SPEED;
static const int I2CPortNumber   = CONFIG_SSD1306_DEFAULT_I2C_PORT_NUMBER;
static const int SCLPin          = CONFIG_SSD1306_DEFAULT_I2C_SCL_PIN;
static const int SDAPin          = CONFIG_SSD1306_DEFAULT_I2C_SDA_PIN;

static const uint8_t SSD1306_I2C_COMMAND_MODE = 0x00;
static const uint8_t SSD1306_I2C_DATA_MODE    = 0x40;

/* Handles for the new ESP-IDF I2C master driver */
static i2c_master_bus_handle_t I2CBusHandle = NULL;
static i2c_master_dev_handle_t I2CDevHandle = NULL;

static bool I2CDefaultWriteBytes( int Address, bool IsCommand, const uint8_t* Data, size_t DataLength );
static bool I2CDefaultWriteCommand( struct SSD1306_Device* Display, SSDCmd Command );
static bool I2CDefaultWriteData( struct SSD1306_Device* Display, const uint8_t* Data, size_t DataLength );
static bool I2CDefaultReset( struct SSD1306_Device* Display );

/*
 * Initializes the i2c master bus with the parameters specified
 * in the component configuration in sdkconfig.h.
 *
 * Returns true on successful init of the i2c bus.
 */
bool SSD1306_I2CMasterInitDefault( void ) {
    i2c_master_bus_config_t BusConfig = {
        .clk_source             = I2C_CLK_SRC_DEFAULT,
        .i2c_port               = (i2c_port_num_t) I2CPortNumber,
        .scl_io_num             = (gpio_num_t) SCLPin,
        .sda_io_num             = (gpio_num_t) SDAPin,
        .glitch_ignore_cnt      = 7,
        .flags.enable_internal_pullup = true,
    };

    if ( i2c_new_master_bus( &BusConfig, &I2CBusHandle ) != ESP_OK ) {
        return false;
    }
    return true;
}

/*
 * Checks to see if the device at the given address is connected.
 *
 * Returns true if device is connected.
 */
bool SSD1306_IsDisplayAttached( int I2CAddress ) {
    if ( I2CBusHandle == NULL ) {
        return false;
    }
    return i2c_master_probe( I2CBusHandle, (uint16_t) I2CAddress, 1000 ) == ESP_OK;
}

/*
 * Attaches a display to the I2C bus using default communication functions.
 *
 * Params:
 * DisplayHandle: Pointer to your SSD1306_Device object
 * Width: Width of display
 * Height: Height of display
 * I2CAddress: Address of your display
 * RSTPin: Optional GPIO pin to use for hardware reset, if none pass -1.
 *
 * Returns true on successful init of display.
 */
bool SSD1306_I2CMasterAttachDisplayDefault( struct SSD1306_Device* DisplayHandle, int Width, int Height, int I2CAddress, int RSTPin ) {
    i2c_device_config_t DevConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = (uint16_t) I2CAddress,
        .scl_speed_hz    = (uint32_t) I2CDisplaySpeed,
    };

    NullCheck( DisplayHandle, return false );

    if ( i2c_master_bus_add_device( I2CBusHandle, &DevConfig, &I2CDevHandle ) != ESP_OK ) {
        return false;
    }

    if ( RSTPin >= 0 ) {
        ESP_ERROR_CHECK_NONFATAL( gpio_set_direction( (gpio_num_t) RSTPin, GPIO_MODE_OUTPUT ), return false );
        ESP_ERROR_CHECK_NONFATAL( gpio_set_level( (gpio_num_t) RSTPin, 1 ), return false );
    }

    return SSD1306_IsDisplayAttached( I2CAddress ) && SSD1306_Init_I2C( DisplayHandle,
        Width,
        Height,
        I2CAddress,
        RSTPin,
        I2CDefaultWriteCommand,
        I2CDefaultWriteData,
        I2CDefaultReset
    );
}

static bool I2CDefaultWriteBytes( int Address, bool IsCommand, const uint8_t* Data, size_t DataLength ) {
    uint8_t* Buffer = NULL;
    bool Result     = false;
    (void) Address; /* Address is encoded in the device handle */

    NullCheck( Data, return false );

    if ( I2CDevHandle == NULL ) {
        return false;
    }

    /* Prepend the SSD1306 control byte (command or data mode) */
    Buffer = (uint8_t*) malloc( DataLength + 1 );
    if ( Buffer == NULL ) {
        return false;
    }

    Buffer[0] = IsCommand ? SSD1306_I2C_COMMAND_MODE : SSD1306_I2C_DATA_MODE;
    memcpy( &Buffer[1], Data, DataLength );

    Result = i2c_master_transmit( I2CDevHandle, Buffer, DataLength + 1, 1000 ) == ESP_OK;
    free( Buffer );
    return Result;
}

static bool I2CDefaultWriteCommand( struct SSD1306_Device* Display, SSDCmd Command ) {
    uint8_t CommandByte = (uint8_t) Command;

    NullCheck( Display, return false );
    return I2CDefaultWriteBytes( Display->Address, true, &CommandByte, 1 );
}

static bool I2CDefaultWriteData( struct SSD1306_Device* Display, const uint8_t* Data, size_t DataLength ) {
    NullCheck( Display, return false );
    NullCheck( Data, return false );

    return I2CDefaultWriteBytes( Display->Address, false, Data, DataLength );
}

static bool I2CDefaultReset( struct SSD1306_Device* Display ) {
    NullCheck( Display, return false );

    if ( Display->RSTPin >= 0 ) {
        ESP_ERROR_CHECK_NONFATAL( gpio_set_level( (gpio_num_t) Display->RSTPin, 0 ), return true );
            vTaskDelay( pdMS_TO_TICKS( 100 ) );
        ESP_ERROR_CHECK_NONFATAL( gpio_set_level( (gpio_num_t) Display->RSTPin, 1 ), return true );
    }

    return true;
}

#endif /* CONFIG_SSD1306_ENABLE_DEFAULT_I2C_INTERFACE */
