# Radio-Rpi
This is a C++ program that uses the RF24 library to establish a wireless connection between two devices, for transmitting audio data. The program captures audio from a specified input device using the SFML library, and sends it to the receiver device using the NRF24L01+ radio module. The program also receives audio data sent from the receiver device, and plays it back using SFML.

The loop checks if a push button connected to GPIO 17 is pressed, indicating that the user wants to start recording. When the button is pressed, the program starts recording audio using SFML's SoundBufferRecorder class, and when the button is released, the program stops recording and transmits the audio data to the receiver device using the NRF24L01+ radio module. The loop also receives audio data from the receiver device, stores it in a buffer, and plays it back using SFML's Sound class when a stop signal is received.

###Build Instructions
```sh
g++ radio.cpp -lrf24 -lpigpio -lsfml-audio -o radio
```
