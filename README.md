# AI-Meter project

#### Inspired with a project: [AI-on-the-edge-device](https://github.com/jomjol/AI-on-the-edge-device)

<img src="Images/logo.png" width="150px" alt="logo">

## Overview

This project is a try to create a unified platform that can receive and 
handle telemetry data such as water, gas, and electricity meter readings autonomously and in an easily configurable way.

<img src="Images/intro.png" width="717" alt="esp32-cam">

[//]: # (<p float="left">)

[//]: # (  <img src="Images/esp32-cam.png" width="300" alt="esp32-cam">)

[//]: # (  <img src="Images/green-right-arrow-600x320.png" width="100" height="150" alt="arrow"/> )

[//]: # (  <img src="Images/meter_photo.png" width="300" alt="meter_photo"/>)

[//]: # (</p>)

## Features

- Easy configuration and setup
- Modern Web View using [Bootstrap 5](https://getbootstrap.com/docs/5.0/getting-started/introduction/) framework
- Dynamic web pages with [Jquery](https://jquery.com/) and [CSP](https://github.com/ximtech/CSP) templates
- Telegram integration. Receive meter reading with bot support
- Flexible cron job configuration
- Timezone auto resolving using Geo IP service. Also timezone can be setup from embedded DB
- Using an advanced [Date-time](https://github.com/ximtech/GlobalDateTime) library that supports timezones and DST rules
- Embedded database(Sqlite) that stores timezones and DST rules
- Self-sufficient file server with all containing images, icons, css, js, fonts etc
- Long autonomous work from inner battery (up to 3 years)

## In progress

- Tensorflow Lite integration and image recognition
- Caption DNS instead hardcoded IP for web view
- Wi-Fi password and other sensitive data asymmetric encryption
- File system control in an admin panel

## Table of Contents

- [How it works](#how-it-works)
  * [First time setup](#first-time-setup)
  * [Workflow](#workflow)
  * [Setup as needed](#setup-as-needed)
- [Hardware](#hardware)
  * [Adapter board](#adapter-board)
  * [Design notes](#design-notes)
  * [Power supply](#power-supply)
  * [Batteries](#batteries)
  * [External antenna](#external-antenna)
  * [Wiring](#wiring)
- [Firmware](#firmware)
  - [Prerequisites](#prerequisites)
  - [Build](#build)
- [Housing](#housing)
- [Admin page](#admin-page)


## How it works

### First time setup

<img src="Images/init_setup.png" width="800" alt="init-setup">

When `wlan.properties` is empty then a device enables access point witch user can connect and moving step by step set all required parameters.
At the last `summary.csp` page, all steps have been validated and viewed. Also, each item in the list can be changed by clicking on it.
If all ok, then 'Finish' button will be displayed, and after pressing on it, the device enables cron and go to the deep sleep. 

### Workflow

<img src="Images/workflow.png" width="800" alt="workflow">

The device takes photos of a meter at a defined cron scheduler.
After initial setup, the device calculates how long need sleep;
then after wakeup it verifies if it's a time and if all ok, then starting job execution.

### Setup as needed

<img src="Images/manual_setup.png" width="500" alt="manual-setup">

When there is a need to reconfigure or update the device while it is in sleep mode, press config button.
Then wait when led starts blinking and search Wi-Fi access point with name assigned from initial setup.

## Hardware

- ESP32 CAM Camera Module Kit
- OV2640 Camera Module, 120 or 160 Degree, 2MP
- External 4 or 8 MB PSRAM
- Micro SD card from 2 to 8 GB
- USB download board
- Battery power supply, adapter board (see below)

<img src="Images/adapter_board.webp" width="250" alt="adapter-board">

### Adapter board

ESP32 module kit does not support to be powered from batteries by default.
So to use the device autonomously without an external power supply, there is a need to update `USB download board`
to a custom solution.
The directory `esp32-cam-shield` is an Altium project that contains all sources for this board.

<img src="Images/pcb-3d-view.png" width="180" alt="pcb-3d-view">

The PCB contains: 
- Almost all parts from the original `USB download board`
- Li-ion charger
- Battery protection
- Step-down converter

Altium project also already contains prepared fabrication files (gerber files, NC drill etc.) in one archive `esp32-cam-shield.rar`
Just place it, for example, in jlcpcb quote as follows:

<img src="Documents/jlcpcb_order_1.png" width="250" alt="pcb-3d-view">

More info (schematic, PCB, BOM etc.) in `esp32-cam-shield` project

### Design notes

- For current housing, push button should be used as follows:<br/>
<img src="Images/push_button.png" width="70" alt="push-button"><br/>
***3x4x2 mm SMD Switch 4 Pin Micro Switch Push Button***

### Power supply

With the adapter board, there are two types of power supply available: 
- External 5V via USB and should support minimum 500mA. Solder two pins together to switch external power supply:<br/>
<img src="Images/power_switch_5v.png" width="100" alt="push-button">
- From inner li-ion battery, also can be charged from USB. Solder this pins:<br/>
<img src="Images/power_switch_3_3_v.png" width="100" alt="push-button">

### Batteries

<img src="Images/battery.png" width="200" alt="battery">

- Type: 13300
- Size: 13mm x 30mm
- Li-ion 3.7V 400mAh
- Count: 3

Such batteries are widely used at one-time vapes and can be extracted from used electronic smoke.

### External antenna

***Optional***, but highly recommended to use external antenna,
because using an external antenna can solve problems related to connectivity problems and speed up data transmission.

- Take a look at your board to see if it is set to use the on-board antenna or the IPEX connector.<br/>
<img src="Images/antena_resistor.png" width="400" alt="antenna-resistor">
- To enable or disable the on-board antenna, you just need to unsolder that resistor and solder it in the desired configuration. 
You can also drop some solder to connect those points (you don’t necessarily need to add the resistor as long as the pads are connected)
- Next, there is a need to connect flexible patch antenna with cable length about 10 cm:<br/>
<img src="Images/patch_antenna.png" width="250" alt="antenna-resistor">

## Wiring

After soldering all together, it should look like on a photo:<br/>
<img src="" width="200" alt="wired-and-soldered">

## Firmware

### Prerequisites

- Esp-idf and Platformio should be already installed 

### Build

The most common way to get firmware is to build it from sources:
- Clone this repository and open `MCU` directory as a project:
- In `Visual Studio Code` -> Open Folder -> MCU
- Then run PlatformIO build: <br/>
<img src="Images/platformio-build.png" width="200" alt="platformio-build">
- If all ok, plug the device:<br/>

|                                  With Adapter                                  |                                    Default                                     |
|:------------------------------------------------------------------------------:|:------------------------------------------------------------------------------:|
| <img src="Images/upload_connection.png" width="150" alt="upload_connection_1"> | <img src="Images/upload_connection.png" width="150" alt="upload_connection_2"> |

- Then upload firmware and flash the device
- When the process is finished, turn on the monitor to see log outputs:<br/>
<img src="Images/serial-monitor.png" width="300" alt="battery">
- By default, console log enabled and `debug` level is set (can be changed in config)

## Housing

The `3D Model` directory contains SolidWorks source models and `STL` directory with printing ready files



## Admin page
