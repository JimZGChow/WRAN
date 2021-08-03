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
#include <chrono>
#include "liquid/liquid.h"
#include "fec.h"


using namespace std;



int i, l;
int dataCarrier = 480;
int subCarrier = 1024;
liquid_float_complex complex_i(0,1);
std::complex <float> z(0,1);
int cycl_pref = 4;      // the fraction of cyclic prefix length (1/x), allowed values: 4, 8, 16 and 32
int PHYmode = 1;        // The PHY mode according to IEEE 802.22, allowed values 1-13
string message = "Hello Christoph";

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

        cout << "PHY mode: " << PHYmode << endl;
        cout << "Cyclic prefix [1/x]: " << cycl_pref << endl;
        cout << "Message: " << message << endl;
        cout << endl;
    }
    else if (argc >= 2)
    {
        for (int c = 0; c < argc; c++)
        {
            switch (c)
            {
            case 1:
                cout << "PHY mode: " << argv[c] << endl;
                if (stoi(argv[c]) > 0 && stoi(argv[c]) < 15)
                    PHYmode = stoi(argv[c]);
                break;

            case 2:
                cout << "Cyclic prefix: " << argv[c] << endl;
                if (stoi(argv[c]) == 4 ||
                    stoi(argv[c]) == 8 ||
                    stoi(argv[c]) == 16 ||
                    stoi(argv[c]) == 32 )
                    cycl_pref = stoi(argv[c]);
                break;

            case 3:
                cout << "Payload: " << argv[c] << endl;
                message = argv[c];
                
                break;
               
            }
        }
    }
    
  
    
    // define frame parameters
    unsigned int cp_len = (int)subCarrier / cycl_pref; // cyclic prefix length
    unsigned int taper_len = (int)cp_len / 4;  // taper length

    // number of bits per symbol
    float bits_per_symbol = 1;
    
    // initialize frame generator properties
    ofdmflexframegenprops_s fgprops;
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_NONE;
    fec_scheme used_FEC_scheme = LIQUID_FEC_NONE;
    fgprops.fec1 = LIQUID_FEC_NONE;
    fgprops.mod_scheme = LIQUID_MODEM_PSK2;

    switch(PHYmode)
    {
    case 1:         //presetted
            break;
    case 2:
                    //not supported
            break;
    case 3:
            used_FEC_scheme = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2*1/2;
            break;
    case 4:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2.0f*2.0f/3.0f;
            break;
    case 5:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2*3/4;
            break;
    case 6:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QPSK;
            bits_per_symbol = 2*5/6;
            break;
    case 7:
            used_FEC_scheme = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4*1/2;
            break;
    case 8:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4*2/3;
            break;
    case 9:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4*3/4;
            break;
    case 10:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QAM16;
            bits_per_symbol = 4*5/6;
            break;
    case 11:
            used_FEC_scheme = LIQUID_FEC_CONV_V27;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6*1/2;
            break;
    case 12:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P23;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6*2/3;
            break;
    case 13:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P34;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6*3/4;
            break;
    case 14:
            used_FEC_scheme = LIQUID_FEC_CONV_V27P56;
            fgprops.mod_scheme = LIQUID_MODEM_QAM64;
            bits_per_symbol = 6*5/6;
            break;
    }

    fgprops.fec0 = used_FEC_scheme;
    
    
    //number of useful symbols in OFDM frame
    int useful_symbols = 22; //for cycl prefix 1/4
    
    //define number of useful symbols with respect to cyclic prefix
    switch(cycl_pref)
    {
    case 8: useful_symbols = 24; break;
    case 16: useful_symbols = 26; break;
    case 32: useful_symbols = 27; break;
    }

    cout << "bits per symbol: " << bits_per_symbol << endl;

    // length of payload (bytes)
    unsigned int payload_len = dataCarrier * useful_symbols * bits_per_symbol / 8 - 1; //may be a problem with mixing variable types
    unsigned int buffer_len = subCarrier + cp_len; // length of buffer

    cout << "Encoded message length: " << fec_get_enc_msg_length(used_FEC_scheme, payload_len);
    cout << endl;

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
    header[7] = '0';
    header[8] = '0';

    // assemble frame
    cout << endl;
    ofdmflexframegen_assemble(fg, header, payload, payload_len);
    cout << endl;
    ofdmflexframegen_print(fg);
    cout << endl;

    // generate frame
    int last_symbol = 0;
    i = 0;
    l = 0;
    while (!last_symbol)
    {
        // generate each OFDM symbol
        last_symbol = ofdmflexframegen_write(fg, buffer, buffer_len);
       
        cout << last_symbol;

        
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

    

      return 0;
}

