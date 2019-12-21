  for (int i=0;i<11;i++){
    if (sysex_transformers_slot0[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(sysex_transformers_slot0[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  void toggleFilter(uint8_t filterBit)
{
  
    GLB_Filter ^= (1 << filterBit);
    uint8_t sysexConfig[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x2, DISP_CableOrJack, DISP_Port, GLB_Filter, 0xF7};

    delay(250);
  
    resetRouteDialBuffer();
    requestPortRouteDump();
}

void printSerialBuffer(){

    for (int i=0;i<32;i++){
      if (serialMessageBuffer[i] < 0x10) {
        Serial.print("0");
      }
      Serial.print(serialMessageBuffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");

}

           case 0: case 1: case 2: case 3: case 4:
           case 5: case 6: case 7: case 8: case 9:      
             DISP_Transformer = inByte;
           break;
           

void toggleFilter(uint8_t filterBit)
{
  
    GLB_Filter ^= (1 << filterBit);
    uint8_t sysexConfig[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x2, DISP_CableOrJack, DISP_Port, GLB_Filter, 0xF7};

    delay(250);
  
    resetRouteDialBuffer();
    requestPortRouteDump();
   
}


void processRouteDialBuffer()
{  
    uint8_t src_id = bufferDecCharToInt(0);
    uint16_t* GLB_BMT = dialCableOrJack == CABLE ? &GLB_BMT_Cable : &GLB_BMT_Jack;
  
    *GLB_BMT ^= (1 << src_id);
    
    uint8_t numChannels = countSetBits(*GLB_BMT);
    uint8_t sysex[numChannels];
    uint8_t sz = numChannels + 10;
    uint8_t numChannel = 9;
    uint8_t sysexConfig[9] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x1, DISP_CableOrJack, DISP_Port, dialCableOrJack};
    uint8_t sysexEnd[1] = {0xF7};
    uint8_t sysexComplete[sz];
  
    memcpy(sysexComplete,sysexConfig,9*sizeof(uint8_t));
    memcpy(sysexComplete+sz-1,sysexEnd,sizeof(uint8_t)); 
    
    for (uint8_t i=0;i<=15;i++){
      if (*GLB_BMT & (1 << i)){
        sysexComplete[numChannel++] = i;
      }
    }
   
    Serial2.write(sysexComplete, sz);
    delay(250);
    
    resetRouteDialBuffer();
    requestPortRouteDump();
}
