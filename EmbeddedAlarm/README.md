| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# Embedded Alarm
## Getting Started
### Prerequisites
- **[ESP IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)** installed on your machine.
- **ESP32 board** compatible with ESP IDF.
- **Buzzer** for the alarm.
- **LCD 1602** for displaying the time and tasks.
### Setup Instructions
assuming you have intialize ESP-IDF on your terminal and on this directory
1. run `idf.py set-target ${YOUR_BOARD}`, to set the target board.
2. run `idf.py menuconfig`, and configure several user configurations you want to configure, on `User configurations` menu.
3. run `idf.py build` to build the project.
4. run `idf.py -p ${PORT} flash` to flash the project to your ESP32 board.
5. run `idf.py -p ${PORT} monitor` to monitor the serial output of the project. there several important output on it, like assigned IP address, etc. by default TCP service run on port 8080.

## Note
- make sure you set flash size to 4MB on `idf.py menuconfig` on `Serial flasher config` menu if there is an error. or you can reduce the size of storage on [partition table](https://github.com/Berigoo/Embedded-Alarm/blob/master/EmbeddedAlarm/partitions.csv).
