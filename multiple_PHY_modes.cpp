/******************************************************************************
 * C++ source of ofdm-frame
 *
 * File:    multiple_PHY_modes.cpp
 * Authors: Bernhard Isemann
 *          Marek Honek
 *          Christoph Mecklenbräucker
 *
 * Created on 03 Jun 2021, 11:35
 * Updated on 19 Jun 2021, 15:55
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
#include <complex>    //c++ not C !!!
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
string mode = "TXwoBP";
float normalizedGain = 1;
int dataCarrier = 480;
int modeSelector = 1;
int duration = 10;
float sampleRate = 1e6;
int subCarrier = 1024;
liquid_float_complex complex_i(0,1);
std::complex <float> z(0,1);
int cycl_pref = 4;      // the fraction of cyclic prefix length (1/x), allowed values: 4, 8, 16 and 32
int PHYmode = 1;        // The PHY mode according to IEEE 802.22, allowed values 1-13
string message = "Hello Christoph";


int error();
void print_gpio(uint8_t gpio_val);


// callback function
int mycallback(unsigned char *  _header,
               int              _header_valid,
               unsigned char *  _payload,
               unsigned int     _payload_len,
               int              _payload_valid,
               framesyncstats_s _stats,
               void *           _userdata)
{
    cout << endl;
    cout << "***** callback invoked!\n" << endl;
    cout << "  header " << _header_valid << endl;
    cout << "   payload " << _payload_valid << endl;
    
    unsigned int i;
    if (_header_valid)
    {
        cout << "Received header: \n" << endl;
        for (i = 0; i < 8; i++)
        {
            cout << _header[i] << endl;
        }
        cout << endl;
    }

    if (_payload_valid)
    {
        cout << "Received payload: \n" << endl;
        for (i = 0; i < _payload_len; i++)
        {
            if (_payload[i] == 0)
                break;
            cout << _payload[i] << endl;
        }
        cout << endl << endl;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "Starting WRAN Access Point with default settings:\n";
        cout << "Mode: " << mode << endl;
        cout << "Frequency: " << centerFrequency << endl;
        cout << "Gain: " << normalizedGain << endl;
        cout << "Number of subcarriers " << subCarrier << endl;
        cout << "Number of data carriers: " << dataCarrier << endl;
        cout << "Duration: " << duration << endl;
        cout << "Sample Rate: " << sampleRate << endl;
        cout << "PHY mode: " << PHYmode << endl;
        cout << "Cyclic prefix [1/x]: " << cycl_pref << endl;
        cout << "Message: " << message << endl;
        cout << endl;
        cout << "type \033[36m'multiple_PHY_modes help'\033[0m to see all options !" << endl;
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
                else if (mode == "help")
                {
                    cout << "Options for starting WRAN: multiple_PHY_modes \033[36mMODE CENTER-FREQUENCY GAIN SUBCARRIERS DATA-CARRIERS DURATION SAMPLE-RATE\033[0m" << endl;
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
                    cout << "\033[36mSUBCARRIERS\033[0m:" << endl;
                    cout << "     count, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mDATA-CARRIERS\033[0m:" << endl;
                    cout << "     count, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mDURATION\033[0m:" << endl;
                    cout << "     in sec, number of type int" << endl;
                    cout << endl;
                    cout << "\033[36mSAMPLE-RATE\033[0m:" << endl;
                    cout << "     number of type float" << endl;
                    cout << endl;
                    cout << "\033[36mPHY-MODE\033[0m:" << endl;
                    cout << "     number (1-13) of type int" << endl;
                    cout << endl;
                    cout << "\033[36mCYCLIC-PREFIX\033[0m:" << endl;
                    cout << "     number (4/8/16/32) of type int" << endl;
                    cout << endl;
                    cout << "\033[36mMESSAGE\033[0m:" << endl;
                    cout << "     string" << endl;
                    cout << endl;
                    return 0;
                }
                else
                {
                    cout << "Wrong settings, please type  \033[36m'multiple_PHY_modes help'\033[0m to see all options !" << endl;
                    return 0;
                }
                break;

            case 2:
                cout << "Center Frequency: " << argv[c] << endl;
                centerFrequency = stof(argv[c]);
                break;

            case 3:
                cout << "Gain: " << argv[c] << endl;
                normalizedGain = atof(argv[c]);
                break;

            case 4:
                cout << "Subcarriers: " << argv[c] << endl;
                subCarrier = stof(argv[c]);
                break;

            case 5:
                cout << "Data Carriers: " << argv[c] << endl;
                dataCarrier = stof(argv[c]);
                break;

            case 6:
                cout << "Duration: " << argv[c] << endl;
                duration = stoi(argv[c]);
                break;

            case 7:
                cout << "Sample Rate: " << argv[c] << endl;
                sampleRate = stof(argv[c]);
                break;
            case 8:
                cout << "PHY mode: " << argv[c] << endl;
                if (stoi(argv[c]) > 0 && stoi(argv[c]) < 15)
                    PHYmode = stoi(argv[c]);
                break;
            case 9:
                cout << "Cyclic prefix: " << argv[c] << endl;
                if (stoi(argv[c]) == 4 ||
                    stoi(argv[c]) == 8 ||
                    stoi(argv[c]) == 16 ||
                    stoi(argv[c]) == 32 )
                    cycl_pref = stoi(argv[c]);
                break;
            case 10:
                cout << "Payload: " << argv[c] << endl;
                message = argv[c];
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
    msg << "<Subcarriers>: " << subCarrier;
    Logger(msg.str());
    msg.str("");
    msg << "Data Carriers: " << dataCarrier;
    Logger(msg.str());
    msg.str("");
    msg << "Duration: " << duration;
    Logger(msg.str());
    msg.str("");
    msg << "Sample Rate: " << sampleRate;
    Logger(msg.str());
    msg << "PHY mode: " << PHYmode;
    Logger(msg.str());
    msg << "Message: " << message;
    Logger(msg.str());


    int serial_port;
    char dat;
    
    if ((serial_port = serialOpen("/dev/serial0", 57600)) < 0) /* open serial port */
    {
        msg.str("");
        msg << "Unable to open serial device: " << strerror(errno);
        Logger(msg.str());
        return 1;
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

    //Enable TX channel,Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
        error();

    //Set sample rate
    if (LMS_SetSampleRate(device, sampleRate, 0) != 0)
        error();
    msg.str("");
    msg << "Sample rate: " << sampleRate / 1e6 << " MHz" << endl;
    Logger(msg.str());

    //Set center frequency
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, centerFrequency) != 0)
        error();
    msg.str("");
    msg << "Center frequency: " << centerFrequency / 1e6 << " MHz" << endl;
    Logger(msg.str());

    //select Low TX path for LimeSDR mini --> TX port 2 (misslabed in MINI, correct in USB)
    if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX2) != 0)
        error();

    //set TX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, normalizedGain) != 0)
        error();

    //calibrate Tx, continue on failure
    LMS_Calibrate(device, LMS_CH_TX, 0, sampleRate, 0);

    //Wait 2sec and send status LoRa message
    sleep(2);

    if (modeSelector == 0)
    {
        msg.str("");
        msg << "Start Receiving at  " << centerFrequency / 1000000 << " MHz.";
        serialPuts(serial_port, msg.str().c_str());
        return 0;
    }
    else
    {
        msg.str("");
        msg << "Start transmitting OFDM frame at " << centerFrequency / 1000000 << " MHz.";
        serialPuts(serial_port, msg.str().c_str());
    }

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

    int i;
    int l;
    
    // define frame parameters
    unsigned int cp_len = (int)subCarrier / cycl_pref; // cyclic prefix length
    unsigned int taper_len = (int)cp_len / 4;  // taper length

    // number of bits per symbol
    float bits_per_symbol = 1;
    
    // initialize frame generator properties
    ofdmflexframegenprops_s fgprops;
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_NONE;
    fgprops.fec0 = LIQUID_FEC_NONE;
    fgprops.fec1 = LIQUID_FEC_NONE;
    fgprops.mod_scheme = LIQUID_MODEM_PSK2;
    cout << "FlexFrame properties set." << endl;
    switch(PHYmode)
    {
    case 1:         //presetted
            break;
    case 2:
                    //not supported
            break;
    case 3:
            fgprops.fec0 = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2.0f/2.0f;
            break;
    case 4:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2.0f*3.0f/2.0f;
            break;
    case 5:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2.0f*4.0f/3.0f;
            break;
    case 6:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2.0f*6.0f/5.0f;
            break;
    case 7:
            fgprops.fec0 = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4.0f/2.0f;
            break;
    case 8:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4.0f*3.0f/2.0f;
            break;
    case 9:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4.0f*4.0f/3.0f;
            break;
    case 10:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4.0f*6.0f/5.0f;
            break;
    case 11:
            fgprops.fec0 = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6.0f/2.0f;
            break;
    case 12:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6.0f*3.0f/2.0f;
            break;
    case 13:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6.0f*4.0f/3.0f;
            break;
    case 14:
            fgprops.fec0 = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6.0f*6.0f/5.0f;
            break;
    }


    //number of useful symbols in OFDM frame
    int useful_symbols = 22; //for cycl prefix 1/4
    
    //define number of useful symbols with respect to cyclic prefix
    switch(cycl_pref)
    {
    case 8: useful_symbols = 24; break;
    case 16: useful_symbols = 26; break;
    case 32: useful_symbols = 27; break;
    }

    // length of payload (bytes)
    unsigned int payload_len = floor(dataCarrier * useful_symbols * bits_per_symbol / 8); //may be a problem with mixing variable types
    unsigned int buffer_len = subCarrier + cp_len; // length of buffer

    // buffers
    liquid_float_complex buffer[buffer_len]; // time-domain buffer
    unsigned char header[8];                 // header data
    unsigned char payload[payload_len];      // payload data
    unsigned char p[subCarrier];             // subcarrier allocation (null/pilot/data)


    //subcarrier allocation
    for (i = 0; i < 1024; i++)
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

    // create frame generator
    ofdmflexframegen fg = ofdmflexframegen_create(subCarrier, cp_len, taper_len, p, &fgprops);

    cout << "Subcarrier allocation done." << endl;
    ofdmflexframegen_print(fg);
    
    // channel parameters
    float dphi  = 0.001f;                       // carrier frequency offset
    float SNRdB = 20.0f;                        // signal-to-noise ratio [dB]
    float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation
    float phi = 0.0f;                       // channel phase

    // create frame synchronizer
    ofdmflexframesync fs = ofdmflexframesync_create(subCarrier, cp_len, taper_len, p, mycallback, NULL);
    
    // ... initialize header/payload ...
    
    strcpy((char *)payload, message.c_str() );


    header[0] = '0';
    header[1] = '0';
    header[2] = '0';
    header[3] = '0';
    header[4] = '0';
    header[5] = '0';
    header[6] = '0';
    header[7] = '0';


    // assemble frame
    ofdmflexframegen_assemble(fg, header, payload, payload_len);
    cout << endl;
    ofdmflexframegen_print(fg);
    cout << endl;

    msg.str("");
    int ofdm_frame_len = ofdmflexframegen_getframelen(fg);
    msg << "OFDM frame length: " << ofdm_frame_len << endl;
    Logger(msg.str());

    // setup stream
    LMS_SetupStream(device, &tx_stream);
    LMS_StartStream(&tx_stream);

    // generate frame
    int last_symbol = 0;
    i = 0;
    l = 0;
    while (!last_symbol)
    {
        // generate each OFDM symbol
        last_symbol = ofdmflexframegen_write(fg, buffer, buffer_len);
        
        // transmitting the buffer
        if (last_symbol == 0)
            int ret = LMS_SendStream(&tx_stream, buffer, buffer_len-1, nullptr, 1000);

        cout << last_symbol;

        // stream status for debugging
        lms_stream_status_t status;
        LMS_GetStreamStatus(&tx_stream, &status); //Get stream status
        msg.str("");
        msg << "TX data rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate
        Logger(msg.str());
        
        // // channel impairments
        // for (i=0; i<buffer_len; i++) {
        //     buffer[i] = std::exp(z*phi);         // apply carrier offset  --> ue c++ complex definition !!!
        //     phi += dphi;                        // update carrier phase
        //     cawgn(&buffer[i], nstd);            // add noise
        // }

        // receive symbol (read samples from buffer)
        ofdmflexframesync_execute(fs, buffer, buffer_len);
   }

    cout << endl;

    ofdmflexframesync_print(fs);


    //Stop streaming
    // destroy the frame generator object
    ofdmflexframegen_destroy(fg);
    // destroy the frame synchroniser
    ofdmflexframesync_destroy(fs);

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
        serialPuts(serial_port, "Stop transmitting.\r");
    }

    serialClose((const int)serial_port);

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