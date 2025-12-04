# OLED ESP32 GUI Skeleton

A simple Arduino project for building a menu-driven GUI on a **128x32 SSD1306 OLED** using a Wemos S2 Mini (ESP32-S2).  
This repo provides a non-blocking framework for navigating menus and multiple screens without using `delay()`.

## Features
- Compact GUI layout for 128x32 OLED
- Menu navigation with buttons (Up, Down, Select)
- Multiple screens: Menu, Status, Settings, Info
- Non-blocking timing using `millis()`
- Easy to extend with new screens or features

## Hardware
- Wemos S2 Mini (ESP32-S2)
- SSD1306 OLED (128x32, I²C address `0x3C`)
- Buttons or rotary encoder for navigation

## Getting Started
1. Clone the repo:
   ```bash
   git clone https://github.com/yourusername/oledesp32.git
   cd oledesp32
