# Nixie Clock ESP32-C3

This project showcases a Nixie Tube Clock built with the ESP32-C3, which I have been developing over the past few months.  The clock is powered via USB-C and is controlled from an ESP32-C3 using the ESP-IDF development framework.  It includes several advanced features: WiFi provisioning through Bluetooth, a web server and client application for adjusting settings (such as time zone, 12-hour or 24-hour format, etc.), automatic sleep mode when no motion is detected, and sound capabilities. 

TODO -- ADD PHOTO OF COMPLETED CLOCK IN ITS CASE

### PCBs

PCBs arrived from JLCPCB:

![clock-pcbs](https://github.com/user-attachments/assets/8e31dfbc-8cd0-4aaf-a7f6-d063d01f86a6)

### Kicad 3D Model

![clock](https://github.com/user-attachments/assets/ce624f02-e655-42a6-9b03-0ae97f553ffc)

![clock-back](https://github.com/user-attachments/assets/b2c6a7f0-005e-44f2-92e9-5081ccc9c044)

## Prototype

The videos below demonstrate the prototyping setup I created using available materials in my lab. These are quick recordings from my cellphone, initially not intended for sharing, but they effectively illustrate the steps taken to get the various components of the clock working.

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

- **Software**: The software in this project is licensed under the MIT License. You can use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software, provided that the original copyright notice and this permission notice are included in all copies or substantial portions of the software. For more details, see the [LICENSE](./LICENSE) file.

- **Hardware**: The hardware schematics and designs in this project are licensed under the CERN Open Hardware License (CERN OHL). You are free to use, modify, and distribute the hardware designs, provided that any modifications are also shared under the same license. For more details, see the [CERN OHL License](./CERN_OHL_LICENSE) file.
