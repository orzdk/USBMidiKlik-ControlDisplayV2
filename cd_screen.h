#ifndef _CD_SCREEN_H_
#define _CD_SCREEN_H_
#pragma once

void renderScreenP(Adafruit_SSD1306* d, char lines[][20])
{
  Serial.println("renderScreenP()");
  
  d->clearDisplay();
  d->setTextSize(1);
  d->setTextColor(WHITE);

  d->setCursor(0,0);
  d->print(lines[0]);

  d->setCursor(0,9);
  d->print(lines[1]);

  d->setCursor(0,18);
  d->print(lines[2]);

  d->setCursor(0,27);
  d->print(lines[3]);

  d->setCursor(0,36);
  d->print(lines[4]);
  
  d->setCursor(0,45);
  d->print(lines[5]);

  d->setCursor(0,54);
  d->print(lines[6]);
  
  d->display();

}

#endif

/*
      Serial.println("");
      for (int i=0;i<10;i++) {
        if (sysexConfig2[i]<10) Serial.print("0");
        Serial.print(sysexConfig2[i],HEX);
      }
      Serial.println("");

*/