
#ifndef _L3M_C_H_
#define _L3M_C_H_
#pragma once 


struct controlCommand{
    uint8_t commandIndex;
    uint8_t parameterCount;    
    char* commandTitle;    
    char* parameterTitles[3];
};

const struct controlCommand cCmd[] = {
    {0x0, 0, "None",                {}                                  }, // 0x1: No transform
    {0x1, 1, "Transp Nt",           {"Value",}                          }, // 0x1: Transpose note
    {0x2, 1, "Offset Vel",          {"Value",}                          }, // 0x2: Offset velocity
    {0x3, 1, "Set Vel",             {"Value",}                          }, // 0x3: Set velocity
    {0x4, 2, "C Transp Nt",         {"Value", "Channel",}               }, // 0x4: Channel Transpose note
    {0x5, 2, "C Offset Vel",        {"Value", "Channel",}               }, // 0x5: Channel Offset velocity
    {0x6, 2, "C Set Vel",           {"Value", "Channel",}               }, // 0x6: Channel Set velocity   
    {0x7, 2, "Map Channel",         {"ChannelIN", "ChannelOUT",}        }, // 0x7: Map channel
    {0x8, 1, "Set Channel",         {"Channel",}                        }, // 0x8: Set channel
    {0x9, 2, "Map CC",              {"CCIN", "CCOUT",}                  }, // 0x9: Map CC
    {0xA, 2, "Map PC",              {"PCIN", "PCOUT",}                  }, // 0xA: Map ProgChg
    {0xB, 2, "Map Event",           {"EventIN","EventOUT",}             }, // 0xB: Map Event
    {0xC, 3, "Split KB",            {"SplitPnt","ChnLow","ChnHigh"}     }, // 0xC: Split Notes
    {0xD, 0, "None",                {}                                  }, // 0xD: No transform
    {0xE, 0, "None",                {}                                  }, // 0xE: No transform
    {0xF, 0, "None",                {}                                  }, // 0xF: No transform
};

#endif
