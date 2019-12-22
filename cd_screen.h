
#ifndef _CD_SCREEN_H_
#define _CD_SCREEN_H_
#pragma once

struct displayLines{
  char* line[7];  
};

void renderScreenP(Adafruit_SSD1306* d, displayLines* lines)
{
  Serial.println("arrived in renderScreenP");
  
  d->clearDisplay();
  d->setTextSize(1);
  d->setTextColor(WHITE);

  d->setCursor(0,0);
  d->print(lines->line[0]);

  d->setCursor(0,9);
  d->print(lines->line[1]);

  d->setCursor(0,18);
  d->print(lines->line[2]);

  d->setCursor(0,27);
  d->print(lines->line[3]);

  d->setCursor(0,36);
  d->print(lines->line[4]);
  
  d->setCursor(0,45);
  d->print(lines->line[5]);

  d->setCursor(0,54);
  d->print(lines->line[6]);
  
  d->display();

  Serial.println("Rendered");

}

#endif
