uint8_t bufferDecCharToInt(uint8_t offset)
{
  uint8_t db10 = (uint8_t)(dialBuffer[offset]-'0') * 10;
  uint8_t db1 = (uint8_t)(dialBuffer[offset+1]-'0');

  return db10 + db1;;
}

uint8_t bufferHexCharToHex(uint8_t offset)
{

  char msbchar = dialBuffer[offset];
  char lsbchar = dialBuffer[offset+1];

  uint8_t msb = (msbchar & 15) +( msbchar >> 6) * 9;
  uint8_t lsb = (lsbchar & 15) +( lsbchar >> 6) * 9;

  return (msb << 4) | lsb;
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

void processTransformerDialBuffer()
{

  uint8_t setOrClear = 0x1;
  uint8_t slot = bufferHexCharToHex(0);
  uint8_t command = bufferHexCharToHex(2);
  uint8_t x,y,z,d,c,s;

  x = bufferHexCharToHex(4);
  y = bufferHexCharToHex(6);
  z = bufferHexCharToHex(8);
  d = bufferHexCharToHex(10);
  s = bufferHexCharToHex(12);
  c = bufferHexCharToHex(14);

  uint8_t sysex[18] = {
      0xF0, 0x77, 0x77, 0x78, 0x0F, 0x3,
      setOrClear, DISP_CableOrJack, DISP_Port, slot, command, x, y, z, d, s, c,
      0xF7
  };

  Serial.write(sysex,18);
  Serial2.write(sysex, 18);
   
  resetRouteDialBuffer();
  requestPortRouteDump();
}

void toggleFilter(uint8_t filterBit)
{
  
    GLB_Filter ^= (1 << filterBit);
    uint8_t sysexConfig[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x2, DISP_CableOrJack, DISP_Port, GLB_Filter, 0xF7};

    delay(250);
  
    resetRouteDialBuffer();
    requestPortRouteDump();
   
}
