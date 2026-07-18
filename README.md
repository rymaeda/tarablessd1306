<!---
 Copyright (c) 2017 Tara Keeling
 
 This software is released under the MIT License.
 https://opensource.org/licenses/MIT
-->

# SSD1306 Component for the ESP32 and ESP-IDF SDK

## About:  
This is a simple component for the SSD1306 display.  
It supports multiple display sizes on both i2c and spi interfaces.  
  
Check out the wiki where most of the relevant information is.

***Examples:*** https://github.com/TaraHoleInIt/tarablessd1306_examples

**Fork Summary**  

Updated the `tarablessd1306` component to improve compatibility with current ESP-IDF APIs V6.

**Main changes**
- Replaced the legacy `driver/i2c_master.h` usage with the current `driver/i2c.h` API in default_if_i2c.c.
- Added I2C bus initialization using `i2c_param_config` and `i2c_driver_install`.
- Implemented I2C write routines for SSD1306 commands and data.
- Added a helper to check whether the display is connected on the I2C bus.
- Integrated the default interface into the existing SSD1306 initialization flow.

**Goal**  
Keep the component working on newer ESP-IDF versions while reducing reliance on deprecated APIs and preserving compatibility with current projects.

**Validation**
- Workspace diagnostics were checked and no errors were reported.
- Examples run with minor changes.
