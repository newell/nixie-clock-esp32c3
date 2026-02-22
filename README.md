# Nixie Clock ESP32-C3 [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![License: CERN-OHL-W](https://img.shields.io/badge/License-CERN_OHL_W-blue.svg)](https://cern-ohl.web.cern.ch/)

## Features:

- **CAD 3D Model of Case and Components using [build123d](https://github.com/gumyr/build123d)**
- **Six IN-12A Nixie Tubes and Four Colon Indicators**
- **ESP32-C3 microcontroller**
- **Powered via USB-C**
- **[High Voltage Flyback Converter](https://github.com/newell/hv-flyback-converter)**
- **WiFi Provisioning via Bluetooth**
- **Sound**
- **Motion Sensing for Sleep Mode**
- **Webserver and Client to Control Settings**:
  - WiFi Credentials
  - 12-hour or 24-hour format
  - NTP server
  - Colon Indicators (Blinking, Always On, Off)
  - Color of LEDs (Spectrum Cycle or Static Mode)
  - Timezone

![DSC03044](https://github.com/user-attachments/assets/07f993d6-80e4-4be8-91cf-ca07ee2fea44)

![DSC03037](https://github.com/user-attachments/assets/44a4a0a0-5de9-4502-906c-f3cac13a6c4e)

![DSC03038](https://github.com/user-attachments/assets/37b1b0a1-37b4-4d95-b2a9-a45ee8a8bafb)

![DSC03041](https://github.com/user-attachments/assets/7cb6dc60-0e32-4339-a29e-1f2e0bf4dc62)

![DSC03039](https://github.com/user-attachments/assets/f31533b9-1901-465e-a1a9-93a7f5385e33)

![DSC03040](https://github.com/user-attachments/assets/585f6704-fa03-43b6-aafe-2fae70a9242d)

https://github.com/user-attachments/assets/77bc2790-672e-41c1-a4d4-c8e7e0b7ab1b

https://github.com/user-attachments/assets/c225c358-999a-4b4a-92b9-9c901be83272

 ### TODO
- [x] Hourly Slot Machine Effect
- [x] LED Spectrum Cycle
- [ ] Motion Sleep Mode

## CAD Model of Clock Case

To design the clock case I used [build123d](https://github.com/gumyr/build123d) (you can [read the docs here](https://build123d.readthedocs.io/en/latest/)).  

![build123d1](https://github.com/user-attachments/assets/34beb862-cac3-4001-b182-0c05eaf4e648)

![build123d2](https://github.com/user-attachments/assets/6d905a4e-8f06-46c9-aa65-b445b4101efb)

![build123d3](https://github.com/user-attachments/assets/4a8ca054-bb4b-4820-a1e7-e0d810ef57d7)

## Clock Control Interface

This project contains a web-based interface to control settings for the clock. The interface is served by the ESP32-C3 and can be accessed at [esp-home.local](http://esp-home.local).

### Features

- **Real-Time Control**: Adjust clock settings in real-time using WebSockets.
- **User-Friendly Interface**: A clean and responsive web interface built with HTML, CSS, and JavaScript.
- **Customizable Settings**: Change various clock settings like time, timezone, 24 or 12 hour format, display color and brightness, and more.

[ClockControlInterface.webm](https://github.com/user-attachments/assets/71b5c2da-ff7b-42fa-be0a-e68aa2519f6b)

## Design

This section contains some behind the scenes of the various stages of designing the clock, including prototyping, debugging, and iterative improvements. 

### Handsoldered PCB Testing and Debugging

- [x] Display current time synced with NTP
- [x] LEDs
- [x] Sound

![DSC00820](https://github.com/user-attachments/assets/b7c6caf7-d2b7-44de-bebc-66ae702bd61a)

![DSC00824](https://github.com/user-attachments/assets/1beff864-1065-4d9b-b666-368eb919fa4f)

![DSC00825](https://github.com/user-attachments/assets/e6425b82-5ede-4e77-bb7d-64c5438d0f17)

### PCBs

Black PCBs arrived from JLCPCB:

![clock-pcbs](https://github.com/user-attachments/assets/8e31dfbc-8cd0-4aaf-a7f6-d063d01f86a6)

### Kicad 3D Model

![clock](https://github.com/user-attachments/assets/745a5776-db51-4f5b-979e-c3a7e6d8c8c4)

![clock-back](https://github.com/user-attachments/assets/593558b8-ba2a-457c-b399-670f584f445b)

### Single Tube

Before receiving my ESP32-C3 development board, I tested the ability to toggle numbers on a Nixie tube and verified the functionality of my homemade high voltage power supply. I created a boost converter using the toner transfer method. This DC-DC converter takes a +12VDC input and outputs approximately +170VDC. I used an Arduino Due board to cycle through the numbers on the Nixie tube.

https://github.com/user-attachments/assets/79ee931c-0b5a-45fc-ba34-53a26bd78811

### Six Tubes Toggling

Upon receiving my ESP32-C3-DevKitM-1 development board, I wired up all six Nixie tubes on a breadboard using K155ID1 1601 DIP16 chips to drive the tubes. On the final PCB, I used SMT transistors to drive each pin of the tubes individually through shift registers.

https://github.com/user-attachments/assets/ffbe7d87-d1e9-45e2-91e5-0040cae557c9

### Six Tubes NTP Time and Sound

Next, I wrote code for the ESP32-C3 to update its time via NTP and drive the tubes accordingly (details are in the code). Additionally, I successfully integrated sound using codecs and a power amplifier.

https://github.com/user-attachments/assets/b822d686-eec8-4e32-874c-11a30fe7fb55

### LEDs

Added code to have the ESP32-C3 drive some LEDs.  There are LEDs underneath each tube on the clock.

https://github.com/user-attachments/assets/37ba964b-dcfa-438d-87e9-e53fea1e2fda

## License

This project is dual-licensed to address both software and hardware components:

- **Software**: The software in this project, other than iro.js, is licensed under the MIT License. You can use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software, provided that the original copyright notice and this permission notice are included in all copies or substantial portions of the software. For more details, see the [LICENSE](./LICENSE) file.

- **iro.js**: Mozilla Public License 2.0 (https://github.com/jaames/iro.js)

- **Hardware**: The hardware schematics and designs in this project are licensed under the CERN Open Hardware License (CERN OHL). You are free to use, modify, and distribute the hardware designs, provided that any modifications are also shared under the same license. For more details, see the [CERN OHL License](./CERN_OHL_LICENSE) file.
