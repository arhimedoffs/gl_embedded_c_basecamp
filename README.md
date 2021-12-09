# Training projects from GlobalLogic Embedded C Basecamp

All project for use on _STM32F407-Disco_ evaluation board with BSP for GlobalLogic Starter Kit.

Additional data for demonstration located on
[GL_embedded_C_basecamp GDrive](https://drive.google.com/drive/folders/1_uaCfp0XaYhpI_Bhh97B6zyh9HJ5HY6c)

[Additional material and tasks](https://drive.google.com/drive/folders/1ykWPKRX9-aB7cf1Uezi-jmffZY9AW5Ec)

## Training 01

Blink 4 leds on _STM32F407-Disco_ evaluation board.

## Training 02

Blink leds from with keys control. Use six blink modes: 1..3 leds and 2 directions of rotation.

* Left/Right switch blink mode
* Up/Down control speed
* Center toggle animation on/off

## Training 03

Generate PWM with TIM4 on same 4 leds as in prev trainings. Buttons functions:

* Left/Rigth decrease/increase PWM duty cycle by 5%
* Up/Down increase/decrease PWM frequency by 100 kHz
* Center switch generation to next led

## Training 04

Measure 3 voltages with ADC1 (potentiometer, external temperature sensor, MCU internal temperature sensor)
and output proportional PWM signals. Signalize over/under voltage with LED blinking.
Reference voltage for ADC is filtered VDD (`3.3 V`).

* External potentiometer with range 0-VDD. Manual position control.
* External temperature sensor. According schematics: `-24C = 2.5v / 0C = 2.02v / 50C = 1.02v / 100C = 0.02v`.
As result `T = -50*V+101`, where `T` is temperature in °C, `V` is voltage in V.
* Internal temperature sensor. `Temperature (in °C) = {(V_SENSE – V_25 ) / Avg_Slope} + 25`,
`V_25 = 760 mV`, `Avg_Slope = 2.5 mV/°C`, minimum sampling time `10 µs`.

* Potentiometer voltage is displayed via blue led brightness. `0V - off, 3V - full`.
Limit value set to `(1.5 V)`, hysteresys `+-100 mV`.
* External temperature is displayed via green led brightness. Internal temperature sensor is displayed via oragne led.
`20°C - off`, `60°C - full`. Limit value set to `40 °C` with hysteresys `+-1 °C`.
* Warning is displayed via red led blinking. If not limits is present - led is off,
if 1 limit - blinking `1 Hz`, if 2 limits - blinkink `2.5 Hz`, if all 3 limits - blinking `5 Hz`.

### Technical description t04

HSI source is used as system clock and feeded to all perepherial without division.
As result we have 16 MHz as base frequency. ADC prescaler is `/2` and ADC base frequency is `8 MHz`.
Single ADCCLK tick is 0.125 uS. ADC resolution is 12 bit (12 ADC ticks),
sampling time is set to 480 ticks on all channels (60 uS). Single channel conversion is `61.5 uS`.
All 3 channel if measured in `184.5 uS` (5.4 kHz).

Timer triggering and DMA cicrular reading is used for ADC sampling. TIM3 is configured to
ADC sampling rate and emmit output on `UpdateEvent`. This event causes single sample
via all 3 channels. Sampling result is putted to circular buffer via DMA. DMA emmit two interrupts:
when half of buffer is full and when all buffer is full. In interrupt handler code set buffer state flag.
According this flag average value of measurement is calculated in main function. After averaging
measurements are converted to physical values. Results are displayed on LEDs (PWM duty circle control)
and compared with limits. Resulted comparing flags used for red LED blinking.

TIM3 frequency is `100 Hz`. Averaging is done on `25` measumert points. PWM and limit update rate is `4 Hz`.

## Training 05

Use UART3 (_115200/8N1_) for communication with PC. Communication consists of:

* Control 4 LED states with SWT keys. Each press toggle LED
* Control 4 LED states with PC keys `w`,`s`,`a`,`d` according SWT positions. Each press toggle LED
* On each LED state change print message to UART
* Every 5 second print temperature measured from external analog sensor

### Technical description t05

For UART transmission DMA mode is used. For receiving - interrupt mode.

For temperature measument TIM3/DMA/ADC is used in same mode as in _training 04_, but without circular mode.
Every 5 second (software timer based on Systick) main program start DMA and TIM3. When DMA full interrupt is called
TIM3 is stopped and buffer ready flag is set. In main program all measurement averaged and converted to temperature.

All text output to UART is done by `printf` function, for which `_write` is redefined.

## Training 06

Use UART3 (_115200/8N1_) for communication with PC.
Use I2C bus to control PWM LED driver _PCA9685_. Used separate `*.c/*.h` for PWM driver code.

Communication consists of:

* Demo mode. Press _Enter_ to toggle demo mode.
* Control each output with command in format `l <led> b <brightness>`.
  `<led>` is LED output port number, use `0` to control all LEDs at once.
  `<brightness>` is new output brightness from range 0..100. Maximum brightness PWM duty cycle is fixed in code.

## Training 07

Use UART3 (_115200/8N1_) for output to PC.
Use SPI bus to control flash memory chip _SST25VF016B_. Used separate `*.c/*.h` for memory driver code.

Control consists of:

* Read 20 strings from begin of 4k pages with maximum length of 80 chars
* Erase all memory
* Write new 20 strings to memory

__NOTE__ Chip was empty at begining
