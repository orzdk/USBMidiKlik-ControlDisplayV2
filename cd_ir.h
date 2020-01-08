#ifndef _CD_IR_H_
#define _CD_IR_H_
#pragma once

#define RED_CH_MINUS  10
#define RED_CH        11
#define RED_CH_PLUS   12
#define BLUE_PREV     13
#define BLUE_NEXT     14
#define GREEN_PLAY    15
#define PURPLE_MINUS  16
#define PURPLE_PLUS   17
#define PURPLE_EQ     18
#define PLUS_100      19
#define PLUS_200      20

uint8_t irTranslate(uint64_t val)
{
  uint8_t rv = 99;
  if (val == ((0xffffffff | val) < 0xFFFF)) return rv;

  switch (val) {

    case 0xFFA25D:
    case 0xE318261B:
      rv = 10;
      break;

    case 0xFF629D:
    case 0x511DBB:
      rv = 11;
      break;

    case 0xFFE21D:
    case 0xEE886D7F:
      rv = 12;
      break;

    case 0xFF22DD:
    case 0x52A3D41F:
      rv = 13;
      break;

    case 0xFF02FD:
    case 0xD7E84B1B:
      rv = 14;
      break;

    case 0xFFC23D:
    case 0x20FE4DBB:
      rv = 15;
      break;

    case 0xFFE01F:
    case 0xF076C13B:
      rv = 16;
      break;

    case 0xFFA8570:
    case 0xFFA857:
    case 0xA3C8EDDB:
      rv = 17;
      break;

    case 0xFF906F:
    case 0xE5CFBD7F:
      rv = 18;
      break;

    case 0xFF6897:
    case 0xC101E57B:
      rv = 0;
      break;

    case 0xFF9867:
    case 0x97483BFB:
      rv = 19;
      break;

    case 0xFFB04F:
    case 0xF0C41643:
      rv = 20;
      break;

    case 0xFF30CF:
    case 0x9716BE3F:
      rv = 1;
      break;

    case 0xFF18E7:
    case 0x3D9AE3F7:
      rv = 2;
      break;

    case 0xFF7A85:
    case 0x6182021B:
      rv = 3;
      break;

    case 0xFF10EF:
    case 0x8C22657B:
      rv = 4;
      break;

    case 0xFF38C7:
    case 0x488F3CBB:
      rv = 5;
      break;

    case 0xFF5AA5:
    case 0x449E79F:
      rv = 6;
      break;

    case 0xFF42BD:
    case 0x32C6FDF7:
      rv = 7;
      break;

    case 0xFF4AB5:
    case 0x1BC0157B:
      rv = 8;
      break;

    case 0xFF52AD:
    case 0x3EC3FC1B:
      rv = 9;
      break;

  }
  return rv;

}


#endif
