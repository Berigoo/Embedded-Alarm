# Task Alarm System
This solution contains two projects:
- [EmbeddedAlarm](https://github.com/Berigoo/Embedded-Alarm/tree/master/EmbeddedAlarm), which is an embedded program. and,
- [embedded-alarm](https://github.com/Berigoo/Embedded-Alarm/tree/master/embedded-alarm), which is a plugin for Obsidian, for managing the device
## Overview
This project (or solution) is a simple alarm system that can be used to set alarms and reminders. The system is composed of two parts:
- An embedded program that runs on a microcontroller, which is responsible for managing the alarms and triggering the alarm when it is time.
- A plugin for Obsidian, which is used to communicate with the embedded program and set the alarms.
## Projects
### Embedded Alarm Program
- **Location**: `./EmbeddedAlarm`
- **Description**: This project is for the embedded alarm device, which is programmed to ring when a scheduled task is due or scheduled. The device can store, remove, and execute tasks based on commands sent by the Obsidian plugin.
- **Current Features**: 
  - Rings when a task is due or scheduled.
  - Can store, remove tasks.
  - Multiple modes
### Obsidian Plugin
- **Location**: `./embedded-alarm`
- **Description**: This is an Obsidian plugin for managing tasks on the embedded alarm device. It allows users to add, delete, or update tasks, and sends these commands to the device.
- **Current Features**: 
  - send commands to the device. 
## Getting Started
### Prerequisites
- [Node.js](https://nodejs.org/en/download/) installed on your machine.
- [Obsidian](https://obsidian.md/) installed on your machine.
- [ESP IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) installed on your machine.
### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/Berigoo/Embedded-Alarm
   cd Embedded-Alarm
   ```
2. Navigate to [EmbeddedAlarm](https://github.com/Berigoo/Embedded-Alarm/tree/master/EmbeddedAlarm) for more instructions on how to set up the embedded program.
3. Navigate to [embedded-alarm](https://github.com/Berigoo/Embedded-Alarm/tree/master/embedded-alarm) for more instructions on how to set up the Obsidian plugin.
