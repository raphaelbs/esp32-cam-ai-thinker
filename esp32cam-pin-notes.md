## Pin Notes
You'll discover most of the pins on the ESP32 cam are already used or have wonky behaviors.  
To make matters a bit more confusing the layout seems to be done by a inebriated engineer or (more likely) a trace auto-router. 
Some of those poor decisions (e.g. not exposing the I2C pins) significantly cripple the possible applications. 
However pins listed as "not exposed" could be accessible using a soldering iron, or a custom FPC in the camera interface. 

* GPIO0 - CAM_PIN_XCLK or CSI_MCLK 
  * Pull to ground (at reset) to put board into flash mode
  * Internally has a 3.3v 10k pullup resistor (R19)
  * CSI_MCLK is used by Camera (line 12 on FPC) 
  * strapping pin - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](ESP32_S datasheet)
  ** default:pull-up spi-boot:1, download-boot:0
  * Supports Capactive Sensing T1 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * ADC2_CH1- 12bit SAR ADC
  * Signal: HSPIHD - Parallel QSPI
  * Signal: EMAC_TX_CLK - Ethernet MAC MII/RII interface
  * listed in ESP32_S datasheet as "RTC_Function2" I2C_SDA
  * designated as a (wpu) "weak pull up" by ESP32_S datasheet v3.4 pg53 IO/Mux Addendum
* GPIO1 - U0TXD used for serial output
  * designated as OUTPUT ONLY
* GPIO2 - HS2_DATA0 (IO2) 
  * Used by SD-Card
  * might be usable as a SPI MISO (need to verify)
  * strapping pin - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](ESP32_S datasheet)
  ** default:pull-down -- must be down to download boot on GPIO0)
  * Supports Capactive Sensing T2 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * ADC2_CH2- 12bit SAR ADC, SD Memory card v3.01 
  * Signal: HSPIWP - Parallel QSPI
  * designated as a (wpd) "weak pull down" by ESP32_S datasheet v3.4 pg53 IO/Mux Addendum
* GPIO3 - U0RXD used for serial input
  * Signal: EMAC_RXD2 - Ethernet MAC MII/RII interface
  * designated as I1? (input only?) 
* GPIO4 - HS_DATA1 
  * Used by SD Card
  * has a 47Kohm resistor (R11) on the SD1/MicroSD line
  * connected to onboard 3030 SMD LED (Flashlight) 
  * might be usable as a SPI MOSI (need to verify)
  * Supports Capactive Sensing T0 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * ADC2_CH0- 12bit SAR ADC, Supports SD Memory Card v3.01
  * Signal: EMAC_TX_ER - Ethernet MAC MII/RII interface
  * listed in ESP32_S datasheet as "RTC_Function2" I2C_SCL
  * designated as a (wpd) "weak pull down" by ESP32_S datasheet v3.4 pg53 IO/Mux Addendum
  * 1-bit SD Card 'hack' initialize the microSD card as follows, then the microSD card won’t use the GPIO4, GPIO12, GPIO13 data lines (HS_DATA1, HS_DATA2, HS_DATA3 respectively)!  
    ```
    // enable SD_MMC in menuconfig > Arduino section
    #include "SD_MMC.h"
    ...
    SD_MMC.begin("/sdcard", true)
    ```
    https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/ 
* GPIO5 - (not exposed) CSI_D0 esp_camera.h:CAM_PIN_D0 -> Camera FPC Y2
  * strapping pin - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](ESP32_S datasheet)
  ** default:pull-up w/GPIO5 controls Timing of SDIO slave (see documentation) 
  * Signal: VSPICS0 - Parallel QSPI
  * Signal: EMAC_RX_CLK - Ethernet MAC MII/RII interface
* GPIO6 - (not exposed) SD_CLK, HS1_CLK
  * SPICLK - Parallel QSPI
* GPIO7 - (not exposed) SD_DATA0, HS1_DATA0
* GPIO8 - (not exposed) SD_DATA1, HS1_DATA1
* GPIO9 - (not exposed) SD_DATA2, HS1_DATA2
  * SPIHD - Parallel QSPI
* GPIO10 - (not exposed) SD_DATA3, HS1_DATA3
  * SPIWP - Parallel QSPI
* GPIO11 - (not exposed) SD_CMD, HS1_CMD
  * SPICS0 - Parallel QSPI
* GPIO12 - HS2_DATA2 
  * ESP32_S datasheet reference: MTDI
  * Hint: use the SD-Card 1bit mode trick described on GPIO4 to use this pin!
  * used as strapping pin - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](ESP32_S datasheet)
    ** default:pull-down selects voltage (3.3v:0, 1.8v:1)    
  * Supports Capactive Sensing T5 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * JTAG, ADC2_CH5- 12bit SAR ADC
  * Signal HSPIQ - Parallel QSPI
  * Signal: EMAC_TXD3 - Ethernet MAC MII/RII interface
  * also RTC_GPIO15
  * MTDI (GPIO12) is used as a bootstrapping pin to select the output voltage of an internal regulator (VDD_SDIO) which powers the flash chip.
  * when connected as SDA (for I2C) causes "RTCWDT_RTC_REST"exception ets_main.c 371 at boot due to flash voltage not being set properly. 
  * To resolve: it is (Recommended) by EspressIf to Burn the flash voltage selection eFuses. This will permanently configure the internal regulator’s output voltage to 3.3 V, and GPIO12 will not be used as a bootstrapping pin. After that, connect a pull-up resistor to GPIO12.
  * Reference & Instructions: [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd\_pullup\_requirements.html#compatibility-overview-espressif-hw-sdio](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/sd_pullup_requirements.html#compatibility-overview-espressif-hw-sdio)
* GPIO13 - HS2_DATA3
  * ESP32_S datasheet: MTCK
  * Hint: use the SD-Card 1bit mode trick described on GPIO4 to use this pin!
  * Supports Capactive Sensing T4 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * JTAG, ADC2_CH4- 12bit SAR ADC, SD Memory card v3.01
  * Signal HSPID - Parallel QSPI
  * Signal: EMAC_RX_ER - Ethernet MAC MII/RII interface
* GPIO14 - HS2_CLK
  * ESP32_S datasheet: MTMS 
  * might be usable as an SPI CLK (need to verify)
  * Supports Capactive Sensing T6 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * JTAG, ADC6_CH0- 12bit SAR ADC, SD memory card v3.01
  * Signal: HSPICLK - Parallel QSPI
  * Signal: EMAC_TXD2 - Ethernet MAC MII/RII interface
  * also RTC_GPIO16
* GPIO15 - HS2_CMD
  * ESP32_S datasheet: MTDO
  * might be usable as an SPI CS/CMD (need to verify)
  * strapping pin - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](ESP32_S datasheet)
  ** default:pull-up enable/disable debugging log print over U0TXD during boot (active:1, silent:0)
  * Supports Capactive Sensing T3 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * JTAG, ADC2_CH3- 12bit SAR ADC
  * Signal HSPICS0 - Parallel QSPI
  * Signal: EMAC_RXD3 - Ethernet MAC MII/RII interface
  * designated as a (wpu) "weak pull up" by ESP32_S datasheet v3.4 pg53 IO/Mux Addendum
* GPIO16 - U2RXD "useless gpio" 
  * is not RTC (can't be used for pwm or precision measurement) 
  * Note that GPIO 16 is not an ADC pin, so you can’t read analog sensors on this pin.
  * Additionally, GPIO 16 is not an RTC GPIO, so it can’t be used as an external wake-up source. 
  * has a 10Kohm pull-up resistor
  * is connected to CS# pin1 of onboard PSRAM64 (64Mbit/8mb CMOS SRAM) that is required for *most* high resolution camera applications
  * Signal: EMAC_CLK_OUT - Ethernet MAC MII/RII interface
* GPIO17 - (not exposed) PSRAM_CLK 
  * Signal: EMAC_CLK_OUT_180 - Ethernet MAC MII/RII interface
* GPIO18 - (not exposed) CSI_D1 esp_camera.h:CAM_PIN_D1 -> Camera FPC Y3
  * Signal VSPICLK - Parallel QSPI
* GPIO19 - (not exposed) CSI_D2 esp_camera.h:CAM_PIN_D2 -> Camera FPC Y4 
  * Signal VSPIQ - Parallel QSPI
  * Signal: EMAC_TXD0 - Ethernet MAC MII/RII interface
* GPIO20 - ??
* GPIO21 - (not exposed) esp_camera.h:CAM_PIN_D3 -> Camera FPC Y5
  * Signal VSIHD - Parallel QSPI
  * Signal: EMAC_TX_EN - Ethernet MAC MII/RII interface
* GPIO22 - (not exposed) CS1_PCLK   esp_camera.h:CAM_PIN_PCLK
  * Signal VSPIWP - Parallel QSPI
  * Signal: EMAC_TXD1 - Ethernet MAC MII/RII interface
* GPIO23 - (not exposed) CS1_HSYNC  esp_camera.h:CAM_PIN_HREF
  * Signal VSPID - Parallel QSPI
* GPIO24 - (not exposed) 
* GPIO25 - (not exposed) esp_camera.h:CAM_PIN_VSYNC -> Camera FPC CS1_VSYNC
  * DAC_1, ADC2_CH8- 12bit SAR ADC
  * Signal: EMAC_RXD0 - Ethernet MAC MII/RII interface
  * also RTC_GPIO6
* GPIO26 - (not exposed) TW1_SDA esp_camera.h:CAM_PIN_SIOD -> Camera FPC SIO_D TW1_SCK (line 20)
  * could be used for I2C SDA (if it was exposed) 
  * has 4.7k (R17) resistor, maybe retrofittable with a wire (but unsure of location)
  * DAC_2, ADC2_CH9- 12bit SAR ADC
  * Signal: EMAC_RXD1 - Ethernet MAC MII/RII interface
  * also RTC_GPIO7
* GPIO27 - (not exposed) TW1_SCK esp_camera.h:CAM_PIN_SIOC ->  Camera FPC SIO_C TW1_SCK (line 22)
  * could be used for I2C SCL (if it was exposed) 
  * has 4.7k (R18) resistor, maybe retrofittable with a wire (but unsure of location)
  * Supports Capactive Sensing T7 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * ADC2_CH7- 12bit SAR ADC
  * Signal: EMAC_RX_DV - Ethernet MAC MII/RII interface
  * also RTC_GPIO17
* GPIO28 - (not exposed) 
  * ?? not defined in the ESP32_S specifications
* GPIO29 - (not exposed) 
  * ?? not defined in the ESP32_S specifications
* GPIO30 - (not exposed) 
  * ?? not defined in the ESP32_S specifications
* GPIO31 - (not exposed) 
  * ?? not defined in the ESP32_S specifications
* GPIO32 - (not exposed) esp_camera.h:CAM_PIN_PWDN - Reset Switch "K1"
  * Analog Pin name: 32K_XP 
  * esp32cam pcb line has a 0.1uF coupled to GND
  * esp32cam pcb line has 3.3v 10Kohm (R6) pullup
  * ADC1_CH4
  * Supports Capactive Sensing T9 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * also RTC_GPIO9
* GPIO33 - (sort of exposed) pcb LED
  * Analog pin name: 32K_XN 
  * Supports Capactive Sensing T8 (see ESP32_S datasheet section 4.1.5 "Touch Sensor")
  * Is connected to the internal BLUE(authentic)/RED(clone) LED on the PCB.
  * If this is on, the WIFI won't work. 
  * ADC1_CH5
  * also RTC_GPIO8
* GPIO34 - (not exposed) esp_camera.h:CAM_PIN_D6 -> Camera FPC Y8
  * Analog pin name: VDET_1
  * ADC1_CH6
  * also RTC_GPIO4
* GPIO35 - (not exposed) esp_camera.h:CAM_PIN_D7 
  * Analog pin: VDET_2
  * ADC1_CH7
  * also RTC_GPIO5
* GPIO36 - (not exposed) esp_camera.h:CAM_PIN_D4
  * Analog pin name: SENSOR_VP
  * ADC1_CH0
  * also RTC_GPIO0
* GPIO37 - (not exposed) 
  * Analog pin name: SENSOR_CAPP
  * ADC1_CH1
  * also RTC_GPIO1
* GPIO38 - (not exposed) 
  * Analog pin name: SENSOR_CAPN
  * ADC1_CH2
  * also RTC_GPIO2
* GPIO39 - (not exposed) esp_camera.h:CAM_PIN_D5 -> Camera FPC Y9
  * Analog pin name: SENSOR_VN
  * ADC1_CH3
  * also RTC_GPIO3

### GPIO General Purpose
According to the ESP32_S docs "Any GPIO Pins" can be used to for:
* Motor PWM 
  * Three channels of 16bit timers generate PWM waveforms, three fault detection signals, three event capture signals, three sync signals
* Two UART Devices with hardware flow control & DMA
* I2C
  * devices in slave or master mode
* I2S
  * Stereo input/output, Parallel LCD data output, Parallel Camera Data input
* Infrared Remote Controller
  * Eight channels for an IR transmitter & receiver of various waveforms
* General Purpose SPI
* LED PWM (16 independent channels @80mhz with duty accuracy of 16bits), 
* Pulse Counter (pcnt_sig_ch[0-1]_in[0-7])
* Signals: EMAC_MDC_out, EMAC_MDI_in, EMAC_MDO_out, EMAC_CRS_out, EMAC_COL_out - Ethernet MAC MII/RII interface
