# interrupt-read-esp32

When recording data from a sensor, I'd like to do this interrupt read pattern:
1. mcu waits for sensor
2. sensor sets interrupt line high
3. mcu gpio interrupt triggers start of I2C/SPI transaction
4. end of transaction interrupt triggers data handling by a task

However, I don't seem to be able to do that with the `esp-idf` peripherals API. What I found to be possible was:

1. have a dedicated task, t1, to perform transactions when notified (waiting with `ulTaskNotifyTake`)
1. mcu waits for sensor
2. sensor sets interrupt line high
3. mcu gpio interrupt notifies t1 (`vTaskNotifyGiveFromISR`)
4. t1 runs blocking and triggers data handling when it's done

which is not as performant as the first flow. This repo is an effort to solve this problem.

The current situation is:

|         | SPI | I2C |
|---------|-----|-----|
| ESP32   |  ❌   | ❌    |
| ESP32S3 |  🚧   |  ❌   |

❌: didn't start

🚧: ongoing effort

## SPI read with ESP32S3

### Setup

This example of SPI interrupt read with ESP32S3 was build using the ESP32-S3-DevKitC-1 v1.0 using SPI in an echo setup (DO and DI are shorted) and with a GPIO output as an interrupt generator shorted to a GPIO input as interrupt source.

The pinout is:

|              | CS | DO | DI | SCLK | INT     | INT_GEN |
|--------------|----|----|----|------|---------|---------|
| Pin          | 13 | 7  | 8  | 6    | 36      | 37      |
| Connected to | -  | DI | DO | -    | INT_GEN | INT     |

### Logs

The current implementation fails to run with this error:

```
I (295) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (316) gpio: GPIO[36]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 1| Intr:1 
I (316) gpio: GPIO[37]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.

Core  1 register dump:
PC      : 0x42007f8f  PS      : 0x00060030  A0      : 0x8037d6d4  A1      : 0x3fc9b2c0  
0x42007f8f: data_handler_task at /Users/apre/aisight/experiments/interrupt-read/main/interrupt-read.c:45 (discriminator 1)

A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000001  A5      : 0x00000000  
A6      : 0x00000001  A7      : 0x00000000  A8      : 0x00000000  A9      : 0x3fc9a4d0  
A10     : 0x00060023  A11     : 0x3fc9b2c0  A12     : 0xffffffff  A13     : 0x00000001  
A14     : 0x00060323  A15     : 0x00000000  SAR     : 0x00000000  EXCCAUSE: 0x0000001c  
EXCVADDR: 0x00000000  LBEG    : 0x00000000  LEND    : 0x00000000  LCOUNT  : 0x00000000  


Backtrace: 0x42007f8c:0x3fc9b2c0 0x4037d6d1:0x3fc9b2f0
0x42007f8c: data_handler_task at /Users/apre/aisight/experiments/interrupt-read/main/interrupt-read.c:45 (discriminator 1)

0x4037d6d1: vPortTaskWrapper at /Users/apre/aisight/resources/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:154





ELF file SHA256: 242986d0c8c9386a

Rebooting...
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x4037d84e
0x4037d84e: spinlock_acquire at /Users/apre/aisight/resources/esp-idf/components/esp_hw_support/include/spinlock.h:103
 (inlined by) xPortEnterCriticalTimeout at /Users/apre/aisight/resources/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:311

SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3810,len:0x1684
load:0x403c9700,len:0xbf4
load:0x403cc700,len:0x2da8
entry 0x403c9904
```