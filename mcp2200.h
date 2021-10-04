/******************************************************************************
 * C++ source of OE-802.22-AP
 *
 * File:   mcp2200.h
 * Author: DI Bernhard Isemann
 *         Koglerstrasse 20
 *         3443 Sieghartskirchen
 *         Austria
 *
 * Created on 22 Aug 2020, 08:15
 * Updated on 22 Aug 2020, 08:06
 * Version 1.00
 *****************************************************************************/

// USB - reverse LEDs
// USBRXLED_ON     0x08 0 0 0 0 0 0 0 0 0 0 0 0x40 
// USBRXLED_OFF    0x08 0 0 0 0 0 0 0 0 0 0 0x40 0
// USBTXLED_ON     0x08 0 0 0 0 0 0 0 0 0 0 0 0x80
// USBTXLED_OFF    0x08 0 0 0 0 0 0 0 0 0 0 0x80 0

// RF - LEDs
// TXLED_OFF      0x08 0 0 0 0 0 0 0 0 0 0 0 0x20
// TXLED_ON       0x08 0 0 0 0 0 0 0 0 0 0 0x20 0  
// RXLED_OFF      0x08 0 0 0 0 0 0 0 0 0 0 0 0x05
// RXLED_ON       0x08 0 0 0 0 0 0 0 0 0 0 0x05 0  

// RF - SWITCH
// SWITCH_A_ON    0x08 0 0 0 0 0 0 0 0 0 0 0x10 0
// SWITCH_A_OFF   0x08 0 0 0 0 0 0 0 0 0 0 0 0x10
// SWITCH_B_ON    0x08 0 0 0 0 0 0 0 0 0 0 0x08 0
// SWITCH_B_OFF   0x08 0 0 0 0 0 0 0 0 0 0 0 0x08    