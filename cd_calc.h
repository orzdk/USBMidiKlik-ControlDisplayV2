#ifndef _CD_CALC_H_
#define _CD_CALC_H_
#pragma once

uint8_t countSetBits(uint16_t n) 
{ 
    uint8_t count = 0; 
    while (n) 
    { 
      n &= (n-1); 
      count++; 
    } 
    return count; 
} 

uint8_t getdigit(uint8_t num, uint8_t n)
{
    uint8_t r, t1, t2;
 
    t1 = pow(10, n+1);
    r = num % t1;
 
    if (n > 0)
    {
        t2 = pow(10, n);
        r = r / t2;
    }
 
    return r;
}

/*
uint8_t bufferHexCharToHex(uint8_t offset)
{
  char msbchar = dialBuffer[offset];
  char lsbchar = dialBuffer[offset+1];

  uint8_t msb = (msbchar & 15) +( msbchar >> 6) * 9;
  uint8_t lsb = (lsbchar & 15) +( lsbchar >> 6) * 9;

  return (msb << 4) | lsb;
}
*/

#endif
