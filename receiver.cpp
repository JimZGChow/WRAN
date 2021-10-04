/******************************************************************************
 * C++ source of oe-802.22
 *
 * File:   fm-rx.cpp
 * Author: Bernhard Isemann
 *
 * Created on 02 Aug 2021, 09:05
 * Updated on 03 Aug 2021, 14:00
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
#include <wiringPi.h>
#include <wiringSerial.h>
#include "lime/LimeSuite.h"
#include <chrono>
#include <math.h>
#include "liquid/liquid.h"
#include "fec.h"

using namespace std;
lms_device_t *device = NULL;
std::stringstream msg;
std::stringstream HEXmsg;
uint8_t setRX = 0x04;     //all other bit = 0 --> 6m
uint8_t setTXwoBP = 0x0B; //all other bit = 0 --> direct path without BP
uint8_t setTX6m = 0x08;   //all other bit = 0 --> 6m with BP
uint8_t setTX2m = 0x09;   //all other bit = 0 --> 2m with BP
uint8_t setTX70cm = 0x0A; //all other bit = 0 --> 70cm with BP
float centerFrequency = 52.8e6;
string mode = "RX";
float normalizedGain = 0;
float modFactor = 0.8f;
float deviation = 25e3;
int modeSelector = 1;
int duration = 30;
float toneFrequency = 2e3;
int subCarrier = 1024;
int cycl_pref = 4; // the fraction of cyclic prefix length (1/x), allowed values: 4, 8, 16 and 32
complex<float> j(0,1);

int error();
void print_gpio(uint8_t gpio_val);

// callback function
int mycallback(unsigned char *_header,
               int _header_valid,
               unsigned char *_payload,
               unsigned int _payload_len,
               int _payload_valid,
               framesyncstats_s _stats,
               void *_userdata)
{
    cout << endl;
    cout << "***** callback invoked!\n"
         << endl;
    cout << "  header " << _header_valid << endl;
    cout << "   payload " << _payload_valid << endl;

    unsigned int i;
    if (_header_valid)
    {
        cout << "Received header: \n"
             << endl;
        for (i = 0; i < 8; i++)
        {
            cout << _header[i] << endl;
        }
        cout << endl;
    }

    if (_payload_valid)
    {
        cout << "Received payload: \n"
             << endl;
        for (i = 0; i < _payload_len; i++)
        {
            if (_payload[i] == 0)
                break;
            cout << _payload[i] << endl;
        }
        cout << endl
             << endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting RPX-100 with default settings:\n";
        cout << "Mode: " << mode << endl;
        cout << "Frequency: " << centerFrequency << endl;
        cout << "Deviation: " << deviation << endl;
        cout << "Modulation Factor: " << modFactor << endl;
        cout << "Duration: " << duration << endl;
        cout << "Cyclic prefix [1/x]: " << cycl_pref << endl;
        cout << endl;
        cout << "type \033[36m'fm-rx help'\033[0m to see all options !" << endl;
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
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 0;
                }
                else if (mode == "APRS")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 1;
                }
                else if (mode == "OFDM")
                {
                    cout << "Starting RPX-100 with following setting:\n";
                    cout << "Mode: " << argv[c] << endl;
                    modeSelector = 2;
                }
                else if (mode == "help")
                {
                    cout << "Options for starting RPX-100: fm-rx \033[36mMODE CENTER-FREQUENCY DEVIATION MODULATION-FACTOR DURATION SAMPLE-RATE\033[0m" << endl;
                    cout << endl;
                    cout << "\033[36mMODE\033[0m:" << endl;
                    cout << "     \033[32mRX\033[0m for receiving Analog FM" << endl;
                    cout << "     \033[31mAPRS\033[0m for receiving APRS" << endl;
                    cout << "     \033[31mOFDM\033[0m for receiving OFDM frames" << endl;
                    cout << endl;
                    cout << "\033[36mCENTER-FREQUENCY\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mDEVIATION\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mMODULATION-FACTOR\033[0m:" << endl;
                    cout << "     in Hz, number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mDURATION\033[0m:" << endl;
                    cout << "     in sec, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mCYCLIC-PREFIX\033[0m:" << endl;
                    cout << "     number (4/8/16/32) of type int" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'fm-rx help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;

            case 2:
                cout << "Center Frequency: " << argv[c] << endl;
                centerFrequency = stof(argv[c]);
                break;

            case 3:
                cout << "Deviation: " << argv[c] << endl;
                deviation = atof(argv[c]);
                break;

            case 4:
                cout << "Modulation Factor: " << argv[c] << endl;
                modFactor = stof(argv[c]);
                break;

            case 5:
                cout << "Duration: " << argv[c] << endl;
                duration = stoi(argv[c]);
                break;
            case 6:
                cout << "Cyclic prefix: " << argv[c] << endl;
                if (stoi(argv[c]) == 4 ||
                    stoi(argv[c]) == 8 ||
                    stoi(argv[c]) == 16 ||
                    stoi(argv[c]) == 32)
                    cycl_pref = stoi(argv[c]);
                break;
            }
        }
    }

    // pid_t pid, sid;
    // pid = fork();
    // if (pid < 0)
    // {
    //     return 1;
    // }
    // if (pid > 0)
    // {
    //     return 1;
    // }

    // umask(0);

    // sid = setsid();
    // if (sid < 0)
    // {
    //     return 1;
    // }

    // if ((chdir("/")) < 0)
    // {
    //     return 1;
    // }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    LogInit();
    Logger("RPX-100 was started succesfully with following settings:");
    msg.str("");
    msg << "Mode: " << mode;
    Logger(msg.str());
    msg.str("");
    msg << "Center Frequency: " << centerFrequency;
    Logger(msg.str());
    msg.str("");
    msg << "Deviation: " << deviation;
    Logger(msg.str());
    msg.str("");
    msg << "Modulation factor: " << modFactor;
    Logger(msg.str());
    msg.str("");
    msg << "Duration: " << duration;
    Logger(msg.str());
    msg.str("");

    //number of useful symbols in OFDM frame
    int useful_symbols = 22; //for cycl prefix 1/4
    int sampleRate = 3328000;

    //define number of useful symbols with respect to cyclic prefix
    switch (cycl_pref)
    {
    case 8:
        useful_symbols = 24;
        sampleRate = 3225600;
        break;
    case 16:
        useful_symbols = 26;
        sampleRate = 3264000;
        break;
    case 32:
        useful_symbols = 27;
        sampleRate = 3273600;
        break;
    }

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
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
        {
            error();
        }
        break;

    case 2:
        if (LMS_GPIOWrite(device, &setRX, 1) != 0)
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

    // // Send single tone
    // const int tx_time = (const int)duration;
    // float f_ratio = toneFrequency / sampleRate;

    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();

    //Set center frequency to 800 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, centerFrequency) != 0)
        error();

    //Set sample rate to 8 MHz, ask to use 2x oversampling in RF
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, sampleRate, 2) != 0)
        error();

    //Streaming Setup

    //Initialize stream
    lms_stream_t streamId;                        //stream structure
    streamId.channel = 0;                         //channel number
    streamId.fifoSize = 1024 * 1024;              //fifo size in samples
    streamId.throughputVsLatency = 1.0;           //optimize for max throughput
    streamId.isTx = false;                        //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; //12-bit integers
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //Initialize data buffers
    const int sampleCnt = 1024 + 256;       //complex samples per buffer
    liquid_float_complex buffer[sampleCnt]; //buffer to hold complex values
    float receiving_buffer[sampleCnt];
    unsigned char p[subCarrier];            // subcarrier allocation (null/pilot/data)

    //subcarrier allocation
    for (int i = 0; i < 1024; i++)
    {
        if (i < 232)
            p[i] = 0; //guard band

        if (231 < i && i < 792)
            if (i % 7 == 0)
                p[i] = 1; //every 7th carrier pilot
            else
                p[i] = 2; //rest data

        if (i > 791)
            p[i] = 0; //guard band
    }

    // define frame parameters
    unsigned int cp_len = (int)subCarrier / cycl_pref; // cyclic prefix length for 1/4 case
    unsigned int taper_len = (int)cp_len / 4;          // taper length for 1/4 case

    // create frame synchronizers
    ofdmflexframesync fs = ofdmflexframesync_create(subCarrier, cp_len, taper_len, p, mycallback, NULL);

    //Start streaming
    LMS_StartStream(&streamId);

    //Streaming
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    //Start streaming
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(duration)) //run for 10 seconds
    {
        //Receive samples
        int samplesRead = LMS_RecvStream(&streamId, receiving_buffer, sampleCnt, NULL, 1000);
        //I and Q samples are interleaved in buffer: IQIQIQ...

        for (int i = 0; i < sampleCnt; i++)
        {
            buffer[i]=receiving_buffer[2*i]+j*receiving_buffer[2*i+1];
        }

        // receive symbol (read samples from buffer)
        ofdmflexframesync_execute(fs, buffer, sampleCnt);

        //Print data rate (once per second#)
        if (chrono::high_resolution_clock::now() - t2 > chrono::seconds(5))
        {
            t2 = chrono::high_resolution_clock::now();
            msg.str("");
            msg << "Received: " << samplesRead << " samples" << endl;
            Logger(msg.str());
        }
    }

    for (int i = 0; i < sampleCnt; i++)
    {
        msg.str("");
        msg << i << ":  " << buffer[i] << endl;
        Logger(msg.str());
    }

    sleep(1);

    //Stop streaming
    LMS_StopStream(&streamId);            //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used

    //Close device
    if (LMS_Close(device) == 0)
    {
        msg.str("");
        msg << "Closed" << endl;
        Logger(msg.str());
    }

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
