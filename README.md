# Urban Steering Firmware

## Purpose
Firmware running on the stm32L432kc microcontroller in the urban steering wheel as a node on the CAN bus interfacing with the driver.

# Compiling
Before compiling, make sure you've installed the platformio extension for vscode.  After opening the project, you should see the platformio toolbar appear on the bottom left, which you can use to compile, flash and open a serial port for debugging.

## Features
* Control vehicle ignition
* Generate and send motor control signals
* Control signals for accessories (lights, wipers, horn, hazards, blinkers) 
* Deadman safety switch 
* Display critical information to the driver on a LCD screen 

## Maintainers
[@cosparks](https://github.com/spennyp)

[@silviu-toderita](https://github.com/jchan34)

[@apoorvgargubc1998](https://github.com/apoorvgargubc1998)
