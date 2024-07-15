# Nixie Clock ESP32-C3

This project showcases a Nixie Tube Clock built with the ESP32-C3, which I have been developing over the past few months. Below is a photo of the completed clock in its case.
The clock is powered via USB-C and includes several advanced features: WiFi provisioning through Bluetooth, a web server for adjusting settings (such as time zone, 12-hour or 24-hour format, etc.), automatic sleep mode when no motion is detected, and sound capabilities.
TODO -- ADD PHOTO OF COMPLETED CLOCK IN ITS CASE

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

