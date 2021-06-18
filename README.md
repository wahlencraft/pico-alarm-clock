# Pico Alarm Clock

## Introduction
This is an alarm clock that is running on Raspberry Pi Pico and is displaying
on a TM1637 7-segment display. In this project I try to stay close to the 
hardware and implement what I can from scratch. For example, I wrote my own 
library for the display and I use real hardware interrupts for button clicks and
time based events.

## Dependencies
* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk): The
  standard library for working with Pico.
* [Pico Extras](https://github.com/raspberrypi/pico-extras): Some additional 
  libraries. I needed this for putting the Pico in sleep mode.
* [TM1637-pico](https://github.com/wahlencraft/TM1637-pico): My own library for
  writing to the 7-segment display.

## Setup
### Hardware

<img style="float: right;" src="images/components.jpg"/>

* Raspberry Pi Pico
* TM1637 7-segment display
* Passive buzzer
* 3 Buttons
* Led (I used a yellow one)
* 4 Resistors: 3 for button debouncing (1k) and 1 for the LED (2k). Exact
  sizes are not important and might vary depending on the buttons and LED.
* 3 Ceramic capacitors (100 nF) for button debouncing.
* A micro USB power cable and 5V power supply.


### Circuit
TODO

### Load Code
If loading code over UART (from for example a Raspberry Pi) some unexpected
behavior arises.

The openocd reset command does not work with this code. I don't know why but a
workaround is to remove that from the command. Load code with: 
`openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program src/alarm-clock.elf verify exit"`
Then restart the Pico manually by using a reset switch or un- and re-plugging the
power.

## Usage
Under normal circumstances will the display show current time and wait for
alarms to fire. When the clock is turned on the time will be `Monday 00:00:00`
and there is no set alarms. To change the time or add an alarm, open the menu.
### Menu
If any button is pressed the menu will open. In the menu you can change the
display brightness, set clock time and set alarms. See the flowchart below for
the entire state machine.
![image](images/MenuFlow.png)
### Alarms
New alarms can be set from the menu. In addition to having a time every alarm
also has a song associated with it. That way you can have different sounds for
different alarms.

When an alarm fires, press any putton to stop it.
