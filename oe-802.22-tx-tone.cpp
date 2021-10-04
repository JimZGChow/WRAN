/******************************************************************************
 * C++ source of oe-802.22-tx-tone
 *
 * File:   OE-802.22-Atx-tone.cpp
 * Author: Bernhard Isemann
 *
 * Created on 02 May 2021, 11:35
 * Updated on 24 May 2021, 15:55
 * Version 1.00
 *****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <syslog.h>
#include <string.h>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <chrono>
#include <cstring>
#include <bitset>
#include "ini.h"
#include "log.h"
#include "lime/LimeSuite.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include <chrono>
#include <math.h>
#include "liquid/liquid.h"

using namespace std;
lms_device_t *device = NULL;
std::stringstream msg;
std::stringstream HEXmsg;
uint8_t setRX = 0x04;     //all other bit = 0 --> 6m
uint8_t setTXwoBP = 0x0B; //all other bit = 0 --> direct path without BP
uint8_t setTX6m = 0x08;   //all other bit = 0 --> 6m with BP
uint8_t setTX2m = 0x09;   //all other bit = 0 --> 2m with BP
uint8_t setTX70cm = 0x0A; //all other bit = 0 --> 70cm with BP
double centerFrequency = 52.8e6;
string mode = "TXwoBP";
float normalizedGain = 1;
double toneFrequency = 2e3;
int modeSelector = 1;
int duration = 10;
double sampleRate = 1e6;

int error();
void print_gpio(uint8_t gpio_val);

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting WRAN Access Point with default settings:\n";
        cout << "Mode: " << mode << endl;
        cout << "Frequency: " << centerFrequency << endl;
        cout << "Gain: " << normalizedGain << endl;
        cout << "Tone Frequency: " << toneFrequency << endl;
        cout << "Duration: " << duration << endl;
        cout << "Sample Rate: " << sampleRate << endl;
        cout << endl;
        cout << "type \033[36m'oe-802.22-tx-tone help'\033[0m to see all options !" << endl;
    }
    else if (argc >= 2)
    {
        for (int c = 0; c < argc; c++)
        {
            switch (c)
            {
            case 1:
                mode = (string)argv[c];
                if (mode == "RX")
                {
                    cout << "Starting WRAN Access Point with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 0;
                }
                else if (mode == "TXwoBP")
                {
                    cout << "Starting WRAN Access Point with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 1;
                }
                else if (mode == "TX6m")
                {
                    cout << "Starting WRAN Access Point with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 2;
                }
                else if (mode == "TX2m")
                {
                    cout << "Starting WRAN Access Point with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 3;
                }
                else if (mode == "TX70cm")
                {
                    cout << "Starting WRAN Access Point with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 4;
                }
                else if (mode== "help")
                {
                    cout << "Options for starting WRAN: oe-802.22-tx-tone \033[36mMODE CENTER-FREQUENCY GAIN TONE-FREQUENCY DURATION SAMPLE-RATE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receive mode" << endl;
                    cout << "     \033[31mTXwoBP\033[0m for transmit mode without bandpass filter" << endl;
                    cout << "     \033[31mTX6m\033[0m for transmit mode with bandpass filter for 50-54 MHz" << endl;
                    cout << "     \033[31mTX2m\033[0m for transmit mode with bandpass filter for 144-146 MHz" << endl;
                    cout << "     \033[31mTX70cm\033[0m for transmit mode with bandpass filter for 430-440 MHz" << endl;
                    cout << endl;
                    cout << "\033[36mCENTER-FREQUENCY\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mGAIN\033[0m:" << endl;
                    cout << "     0 - 1.0, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mTONE-FREQUENCY\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mDURATION\033[0m:" << endl;
                    cout << "     in sec, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mSAMPLE-RATE\033[0m:" << endl;
                    cout << "     number of type float" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'oe-802.22-tx-tone help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;

            case 2:
                cout << "Center Frequency: " << argv[c] << endl;
                centerFrequency = stod(argv[c]);
                break;

            case 3:
                cout << "Gain: " << argv[c] << endl;
                normalizedGain = atof(argv[c]);
                break;

            case 4:
                cout << "Tone Frequency: " << argv[c] << endl;
                toneFrequency = stod(argv[c]);
                break;

            case 5:
                cout << "Duration: " << argv[c] << endl;
                duration = stoi(argv[c]);
                break;

                case 6:
                cout << "Sample Rate: " << argv[c] << endl;
                sampleRate = stod(argv[c]);
                break;
            }
        }
    }

    pid_t pid, sid;
    pid = fork();
    if (pid < 0)
    {
        return 1;
    }
    if (pid > 0)
    {
        return 1;
    }

    umask(0);

    sid = setsid();
    if (sid < 0)
    {
        return 1;
    }

    if ((chdir("/")) < 0)
    {
        return 1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    LogInit();
    Logger("OE-802.22 was started succesfully with following settings:");
    msg.str("");
    msg << "Mode: " << mode;
    Logger(msg.str());
    msg.str("");
    msg << "Center Frequency: " << centerFrequency;
    Logger(msg.str());
    msg.str("");
    msg << "Gain: " << normalizedGain;
    Logger(msg.str());
    msg.str("");
    msg << "Tone Frequency: " << toneFrequency;
    Logger(msg.str());
    msg.str("");
    msg << "Duration: " << duration;
    Logger(msg.str());
    msg.str("");
    msg << "Sample Rate: " << sampleRate;
    Logger(msg.str());

    // int serial_port;
    // char dat;
    // if ((serial_port = serialOpen("/dev/serial0", 57600)) < 0) /* open serial port */
    // {
    //     msg.str("");
    //     msg << "Unable to open serial device: " << strerror(errno);
    //     Logger(msg.str());
    //     return 1;
    // }

    if (wiringPiSetup() == -1) /* initializes wiringPi setup */
    {
        msg.str("");
        msg << "Unable to start wiringPi: " << strerror(errno);
        return 1;
    }

    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0)
    {
        error(); //NULL can be passed to only get number of devices
    }
    msg.str("");
    msg << "Number of devices found: " << n;
    Logger(msg.str()); //print number of devices
    if (n < 1)
    {
        return -1;
    }

    if (LMS_Open(&device, list[0], NULL)) //open the first device
    {
        error();
    }
    sleep(1);
    //Initialize device with default configuration
    if (LMS_Init(device) != 0)
    {
        error();
    }
    sleep(1);

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "Read current GPIO state.\n";
    Logger(msg.str());

    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(device, &gpio_dir, 1) != 0)
    {
        error();
    }

    if (LMS_GPIODirRead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "Set GPIOs direction to output.\n";
    Logger(msg.str());

    switch (modeSelector)
    {
    case 0:
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
        {
            error();
        }
        break;

    case 1:
        if (LMS_GPIOWrite(device, &setTXwoBP, 1) != 0)
        {
            error();
        }
        break;

    case 2:
        if (LMS_GPIOWrite(device, &setTX2m, 1) != 0)
        {
            error();
        }
        break;

    case 3:
        if (LMS_GPIOWrite(device, &setTX6m, 1) != 0)
        {
            error();
        }
        break;

    case 4:
        if (LMS_GPIOWrite(device, &setTX70cm, 1) != 0)
        {
            error();
        }
        break;
    }

    if (LMS_GPIORead(device, &gpio_val, 1) != 0)
    {
        error();
    }
    msg.str("");
    msg << "GPIO Output to High Level:\n";
    print_gpio(gpio_val);
    Logger(msg.str());

    msg.str("");
    msg << "LimeRFE set to " << mode << endl;
    Logger(msg.str());

    // Send single tone
    const double frequency = centerFrequency;             //center frequency to 50MHz
    const double sample_rate = (const double)sampleRate;                       //sample rate to 1 MHz
    const double tone_freq = (const double)toneFrequency; //tone frequency
    const double f_ratio = tone_freq / sample_rate;
    const int tx_time = duration;

    msg.str("");
    msg << "f_ratio: : " << f_ratio << endl;
    Logger(msg.str());

    //Enable TX channel,Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
        error();

    //Set sample rate
    if (LMS_SetSampleRate(device, sample_rate, 0) != 0)
        error();
    msg.str("");
    msg << "Sample rate: " << sample_rate / 1e6 << " MHz" << endl;
    Logger(msg.str());

    //Set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, frequency) != 0)
        error();
    msg.str("");
    msg << "Center frequency: " << frequency / 1e6 << " MHz" << endl;
    Logger(msg.str());

    //select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX2) != 0)
        error();

    //set TX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normalizedGain) != 0)
        error();

    //calibrate Tx, continue on failure
    LMS_Calibrate(device, LMS_CH_TX, 0, sample_rate, 0);

    //Wait 12sec and send status LoRa message
    sleep(2);

    // if (modeSelector == 0)
    // {
    //     msg.str("");
    //     msg << "Start Receiving at  " << centerFrequency / 1000000 << " MHz.";
    //     serialPuts(serial_port, msg.str().c_str());
    //     return 0;
    // }
    // else
    // {
    //     msg.str("");
    //     msg << "Start transmitting at " << centerFrequency / 1000000 << " MHz.";
    //     serialPuts(serial_port, msg.str().c_str());
    // }

    //Streaming Setup

    lms_stream_t tx_stream;                        //stream structure
    tx_stream.channel = 0;                         //channel number
    tx_stream.fifoSize = 256 * 1024;               //fifo size in samples
    tx_stream.throughputVsLatency = 0.8;           //0 min latency, 1 max throughput
    tx_stream.dataFmt = lms_stream_t::LMS_FMT_F32; //set dataformat for tx_stream to uint16 or floating point samples
    tx_stream.isTx = true;

    //lms_stream_meta_t meta_tx;
    //meta_tx.waitForTimestamp = true;
    //meta_tx.flushPartialPacket = true;
    //meta_tx.timestamp = 0;

    //modulator
    float kf = 0.1f;                 // modulation factor
    unsigned int num_samples = 1024; // number of samples
    //freqmod mod = freqmod_create(kf); // modulator

    //Initialize data buffers
    const int buffer_size = 1024 * 8;
    float tone_buffer[2 * buffer_size];        //baseband buffer to hold complex values (2*samples))
    liquid_float_complex mod_buffer[buffer_size]; //TX buffer to hold complex values - liquid library)
    uint16_t tx_buffer[2 * buffer_size];          //baseband buffer to hold complex values (2*samples))
    for (int i = 0; i < buffer_size; i++)
    { //generate TX tone
        const double pi = acos(-1);
        double w = 2 * pi * i * f_ratio;
        tone_buffer[2 * i] = cos(w);
        tone_buffer[2 * i + 1] = sin(w);
    }

    msg.str("");
    msg << "Tx tone frequency: " << tone_freq / 1e3 << " kHz" << endl;
    Logger(msg.str());

    //freqmod_modulate_block(mod, tone_buffer, 2 * buffer_size, mod_buffer);

    const int send_cnt = int(buffer_size * f_ratio) / f_ratio;
    msg.str("");
    msg << "sample count per send call: " << send_cnt << std::endl;
    Logger(msg.str());

    LMS_SetupStream(device, &tx_stream);
    LMS_StartStream(&tx_stream); //Start streaming
    //Streaming
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(tx_time)) //run for 10 seconds
    {
        //Transmit samples
        int ret = LMS_SendStream(&tx_stream, tone_buffer, send_cnt, nullptr, 1000);
        if (ret != send_cnt)
        {
            msg.str("");
            msg << "error: samples sent: " << ret << "/" << send_cnt << endl;
            Logger(msg.str());
        }

        //Print data rate (once per second)
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(5))
        {
            t2 = chrono::high_resolution_clock::now();
            lms_stream_status_t status;
            LMS_GetStreamStatus(&tx_stream, &status); //Get stream status
            msg.str("");
            msg << "TX data rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate
            Logger(msg.str());
        }
    }
    sleep(1);
    //Stop streaming
    LMS_StopStream(&tx_stream);
    LMS_DestroyStream(device, &tx_stream);

    //Disable TX channel
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, false) != 0)
        error();

    //Close device
    if (LMS_Close(device) == 0)
    {
        msg.str("");
        msg << "Closed" << endl;
        Logger(msg.str());
        // serialPuts(serial_port, "Stop transmitting.\r");
    }

    // serialClose((const int)serial_port);

    return 0;
}

int error()
{
    msg.str("");
    msg << "ERROR: " << LMS_GetLastErrorMessage();
    Logger(msg.str());
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

void print_gpio(uint8_t gpio_val)
{
    for (int i = 0; i < 8; i++)
    {
        bool set = gpio_val & (0x01 << i);
        msg << "GPIO" << i << ": " << (set ? "High" : "Low") << std::endl;
    }
}
