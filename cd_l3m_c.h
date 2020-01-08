#ifndef _L3M_C_H_
#define _L3M_C_H_
#pragma once 

uint8_t sysex_route_reset[7] = {0xF0,0x77,0x77,0x78,0x0F,0x00,0xF7}; // 1. 0F - RouteReset
uint8_t sysex_hw_reset[6]    = {0xF0,0x77,0x77,0x78,0x0A,0xF7};      // 3. 0A - HW reset
uint8_t sysex_serial_boot[6] = {0xF0,0x77,0x77,0x78,0x08,0xF7};      // 4. 08 - Serialmode

struct controlCommand{
    uint8_t commandIndex;
    uint8_t parameterCount;    
    char* commandTitle;    
    char* parameterTitles[3];
};

const struct controlCommand cCmd[] = {
    {0x0, 0, "None",                {}                       }, // 0x1: No transform
    {0x1, 1, "Transp Nt",           {"Val",}                 }, // 0x1: Transpose note
    {0x2, 1, "Offset Vel",          {"Val",}                 }, // 0x2: Offset velocity
    {0x3, 1, "Set Vel",             {"Val",}                 }, // 0x3: Set velocity
    {0x4, 2, "C Transp Nt",         {"Val", "Chn",}          }, // 0x4: Channel Transpose note
    {0x5, 2, "C Offset Vel",        {"Val", "Chn",}          }, // 0x5: Channel Offset velocity
    {0x6, 2, "C Set Vel",           {"Val", "Chn",}          }, // 0x6: Channel Set velocity   
    {0x7, 2, "Map Chnl",            {"Chn >", "> Chn",}      }, // 0x7: Map channel
    {0x8, 1, "Set Chnl",            {"Chn",}                 }, // 0x8: Set channel
    {0x9, 2, "Map CC",              {"CC >", "> CC",}        }, // 0x9: Map CC
    {0xA, 2, "Map PC",              {"PC >", "> PC",}        }, // 0xA: Map ProgChg
    {0xB, 2, "Map Event",           {"Evt >","> Evt",}       }, // 0xB: Map Event
    {0xC, 3, "Split KB",            {"Note","ChnL","ChnH"}   }, // 0xC: Split Notes
    {0xD, 0, "None",                {}                       }, // 0xD: No transform
    {0xE, 0, "None",                {}                       }, // 0xE: No transform
    {0xF, 0, "None",                {}                       }  // 0xF: No transform
};

#endif
