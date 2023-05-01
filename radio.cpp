#include <iostream>
#include <RF24/RF24.h>
#include <SFML/Audio.hpp>
#include <string>
#include <pigpio.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>

using namespace std;

const int pushToTalk = 17;

// Generic
RF24 radio(22,0);
string inputDevice = "";

bool setupPeripherals();
void handleTransmission(const sf::Int16* samples, size_t count);
void playSound(const sf::Int16* dataArray, int size);

int main(int argc,  char** argv)
{
    if(!setupPeripherals()) {
        return 1;
    }

    radio.startListening();
    sf::SoundBufferRecorder recorder;
    if (!recorder.setDevice(inputDevice))
    {
        std::cout << "Device selection failed!" << '\n';
        return 1;
    }
    vector<sf::Int16> audioBuffer;
    bool isRecording = false;
    bool receivedStop = false;
    bool receiveStandby = false;

    while(true) {
        int isTalking = gpioRead(pushToTalk);
        if(!isRecording && isTalking == 1) 
        {
            if(!recorder.start()) {
                std::cout << "Failed to start audio capture!" << '\n';
                continue;
            }  
            isRecording = true;
        }

        if(isRecording && isTalking == 0) {
            recorder.stop();
            isRecording = false;
            const sf::SoundBuffer& buffer = recorder.getBuffer();
            const sf::Int16* samples = buffer.getSamples();
            size_t count = buffer.getSampleCount();
            std::cout << "STANDBY TX";
            std::cout.flush();
            handleTransmission(samples, count);
        }
        sf::Int16* receivedData = new sf::Int16[16];  
        uint8_t pipe;
        if (radio.available(&pipe)) {                        
            radio.read(receivedData, 32);
            int zeros = 0;
            for(int i = 0; i < 16; i++) {
                if(receivedData[i] == 0) {
                    zeros++;
                }
                audioBuffer.push_back(receivedData[i]);
            }
            if(receiveStandby == false) {
                std::cout << "STANDBY RX ";
                std::cout.flush();
            }
            receiveStandby = true;
            if(zeros == 16) {
                receivedStop = true;
            }                                          
        }
        delete[] receivedData;
        if(receivedStop) {
            receiveStandby = false;
            const sf::Int16* dataArray = audioBuffer.data();
            playSound(dataArray, audioBuffer.size());
            receivedStop = false;
            audioBuffer.clear();
        }
    }
}

void playSound(const sf::Int16* dataArray, int size) 
{           
    sf::SoundBuffer audio;
    if (!audio.loadFromSamples(dataArray, size, 1, 11100)) 
    {
        std::cout << "Failed to load sound buffer" << '\n';
        return;
    }

    sf::Sound sound;
    sound.setBuffer(audio);
    sound.play();

    // wait until the sound finishes playing
    while (sound.getStatus() == sf::Sound::Playing) {
        // do nothing
    }
    std::cout << " OK" << '\n';
}

void handleTransmission(const sf::Int16* samples, size_t count)  
{
    radio.stopListening();
    vector<sf::Int16*> packets;
    int amountOfPackets = floor(count/64);  
    for(int x = 0; x < amountOfPackets; x++) {
        sf::Int16* packet = new sf::Int16[16]; 
        int index = 0;
        for(int i = 64*x; i < 64*(x+1); i += 4) {
            packet[index] = samples[i]*2;
            index++;
        }
        packets.push_back(packet);
    }   

    for(sf::Int16* element : packets) {
        //std::cout << "Sent packet: ";
        //for(int i = 0; i < 16; i++) {
        //    std::cout << element[i] << ",";
        //}
        //std::cout << '\n';
        radio.write(element, 32);
    }

    this_thread::sleep_for(chrono::milliseconds(50));
    uint8_t emptyBuffer[32] = {};

    radio.write(&emptyBuffer, 32);
    std::cout << " OK" << '\n';

    for(sf::Int16* element : packets) {
        delete[] element;
    }
    radio.startListening();
}

bool setupPeripherals() 
{
    std::cout << "Configuring Radio... ";
    std::cout.flush();
    if(!radio.begin()) {
        std::cout << "Radio hardware not responding!" << '\n';
        return false;
    }

    std::cout << "Set data rate: (0: 250kbps | 1: 1mbps | 2: 2mbps)" << endl;
    int num = 555;
    while(num > 2 && num >= 0) {
        std::cin >> num;
    }

    rf24_datarate_e dataRate;
    switch(num) {
        case 0: 
            dataRate = RF24_250KBPS;
            break;
        case 1: 
            dataRate = RF24_1MBPS;
            break;
        case 2: 
            dataRate = RF24_2MBPS;
            break;
        default: 
            dataRate = RF24_1MBPS;
            break;
    }

    radio.setPALevel(RF24_PA_MAX,1);
    radio.setDataRate(dataRate);
    radio.setChannel(42);
    
    radio.setCRCLength(RF24_CRC_16);
    radio.setAutoAck(false);
    radio.setAddressWidth(3);
    uint8_t addresses[] = {"abc"}; 

    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[0]);

    std::cout << "OK" << '\n' << '\n';

    std::cout << "Configuring Audio... ";
    std::cout.flush();
    if (!sf::SoundBufferRecorder::isAvailable())
    {
        // error: audio capture is not available on this system
        std::cout << "Audio caputre unavailable!" << '\n';
        return false;
    }   

    vector<string> availableDevices = sf::SoundRecorder::getAvailableDevices();

    std::cout << "Select Microphone:" << '\n';
    for(string s : availableDevices) {
        std::cout << s << '\n';
    }

    int choice = 555;
    while(choice >= availableDevices.size() && choice >= 0) {
        std::cin >> choice;
    }
    inputDevice = availableDevices.data()[choice];

    std::cout << "OK" << '\n';

    std::cout << "Configuring I/O... ";
    std::cout.flush();
    if (gpioInitialise() < 0)
    {
        std::cout << "pigpio initialisation failed!" << '\n';
        return false;
    }

    gpioSetMode(pushToTalk, PI_INPUT);

    std::cout << "OK gpio" << pushToTalk << '\n';

    return true;
}
