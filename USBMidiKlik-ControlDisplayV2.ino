#include <IRremote.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>
#include <libMMM_cli.h>

#include "hardware_config.h"
#include "cd_midi.h"
#include "cd_ir.h"
#include "cd_calc.h"
#include "cd_screen.h"

#define RECV_PIN 8
#define TRANSFORMERS_PR_CHANNEL 2

Adafruit_SSD1306 display(-1);

IRrecv irrecv(RECV_PIN);
decode_results results;

struct XO{
  char filter[4];
};

enum cj { CABLE=0x0 , JACK=0x1 };
enum rf { ROUTE=0x1 , FILTER=0x2, TRANSFORMER=0x3 };
enum sc { MENU=0x0, ROUTES=0x1, TRANSFORMERS=0x2, SYSEXRAW=0x3 };
enum dm { VIEW=0x0, SET=0x1 };

char* screenTitles[4] = { "Menu", "---- Routes ---- ", "-- Transformers -- ", "----- Sysex -----" };
uint8_t numScreens = 4;

char DISP_LINE_1[20] = "";
char DISP_LINE_2[20] = "";
char DISP_LINE_3[20] = "";
char DISP_LINE_4[20] = "";
char DISP_LINE_5[20] = "";
char DISP_LINE_6[20] = "";
char DISP_LINE_7[20] = "";

displayLines DISP_LINES[7];

char dialBuffer[20];
uint8_t dialBufferPos = 0;
uint8_t dialCableOrJack = 0;

uint8_t DISP_Screen = 0;
uint8_t DISP_CableOrJack = 0;
uint8_t DISP_Port = 0;
uint8_t DISP_Slot = 0;
uint8_t DISP_Transformer = 0;
uint8_t DISP_Mode = 0;

uint8_t DISP_Mode = 0;
uint8_t DISP_Mode = 0;

uint8_t pendingConfigPackets = 0;
uint8_t serialMessageBuffer[32];
uint8_t serialMessageBufferIDX = 0;

uint8_t targetsTxtPos = 0;

char cableTargetsTxt[20] = "";
char jackTargetsTxt[20] = "";
char targetsTxt[20] = "";

XO GLB_Filter_XO;


/* Dial buffer stuff */

void resetRouteDialBuffer()
{
  dialBufferPos = 0;
  memset(dialBuffer, 0, sizeof(dialBuffer));
}

/* Dump Requests */

void requestPortRouteDump()
{
  uint8_t sysex_cableTargets[11] =  { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };
  uint8_t sysex_jackTargets[11] =   { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x1, 0xF7 };
  uint8_t sysex_filterTargets[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x2, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };   
  
  pendingConfigPackets = 3;
  
  Serial1.write(sysex_cableTargets, 11);delay(100);Serial1.flush();
  Serial1.write(sysex_jackTargets, 11);delay(100);Serial1.flush();
  Serial1.write(sysex_filterTargets, 11);delay(100);Serial1.flush();  
}

void requestPortTransformerDump()
{
  uint8_t sysex_transformers_slot[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, DISP_Slot, 0xF7 }; 

  pendingConfigPackets = 1;

  Serial1.write(sysex_transformers_slot, 11);delay(100);Serial1.flush();  
}

void requestScreenData()
{
    if ( DISP_Screen == ROUTES ) {      
      requestPortRouteDump();
    } else 
    if ( DISP_Screen == TRANSFORMERS ) {     
      requestPortTransformerDump();
    } 
}

/* Screen Handling */

void updateScreen()
{

  char line1[20] = "";
  char line2[20] = "";
  char line3[20] = "";
  
  char tbuffer[1];
  char pbuffer[1];
  char sbuffer[1];
 
  sprintf(pbuffer, "%d", DISP_Port);
  sprintf(sbuffer, "%d", DISP_Slot);
  sprintf(tbuffer, "%d", DISP_Transformer);
 
  strncpy(DISP_LINE_1, screenTitles[DISP_Screen], sizeof(DISP_LINE_1));
  
  if (DISP_Screen == MENU){

    strncpy(DISP_LINE_2, "> 1: Routes", 20);
    strncpy(DISP_LINE_3, "> 2: Transformers", 20);
    strncpy(DISP_LINE_4, "> 3: Send Sysex", 20);
    strncpy(DISP_LINE_5, "", 20);
    strncpy(DISP_LINE_6, "", 20);
    strncpy(DISP_LINE_7, "1-3,EQ", 20);
   

  } else if (DISP_Screen == ROUTES){

    strcat(line1, DISP_CableOrJack == CABLE ? "CBL " : "JCK "); 
    strcat(line1, pbuffer); 
    strcat(line1, GLB_Filter_XO); 

    strcat(line2, "C:");
    strcat(line2, cableTargetsTxt);
    
    strcat(line3, "J:");
    strcat(line3, jackTargetsTxt);
    
    strncpy(DISP_LINE_2, line1, 20);
    strncpy(DISP_LINE_3, line2, 20);
    strncpy(DISP_LINE_4, line3, 20);
    strncpy(DISP_LINE_5, "", 20);
    strncpy(DISP_LINE_6, "", 20);
    strncpy(DISP_LINE_7, "", 20);


  } else if (DISP_Screen == TRANSFORMERS){

    if (DISP_Mode == VIEW){

      strcat(line1, DISP_CableOrJack == CABLE ? "CBL " : "JCK "); //11
      strcat(line1, pbuffer);     
      strcat(line1, " SLT "); 
      strcat(line1, sbuffer);
      strcat(line1, " TRF ");
      strcat(line1, tbuffer);
      
      strncpy(DISP_LINE_2, line1, 20);
      strncpy(DISP_LINE_3, cCmd[DISP_Transformer].commandTitle, 20);
      strncpy(DISP_LINE_4, "", 20);
      strncpy(DISP_LINE_5, "", 20);
      strncpy(DISP_LINE_6, "", 20);
      strncpy(DISP_LINE_7, "[>||] Config", 20);

    } else if (DISP_Mode == SET){

      strncpy(DISP_LINE_1, cCmd[DISP_Transformer].commandTitle, 20);
      strncpy(DISP_LINE_2, cCmd[DISP_Transformer].parameterTitles[0], 20);
      strncpy(DISP_LINE_3, cCmd[DISP_Transformer].parameterTitles[1], 20);
      strncpy(DISP_LINE_4, cCmd[DISP_Transformer].parameterTitles[2], 20);   
      strncpy(DISP_LINE_5, "", 20);
      strncpy(DISP_LINE_6, "", 20);
      strncpy(DISP_LINE_7, "[>||] Cancel", 20);  

    }

  }

  displayLines dLines = { DISP_LINE_1, DISP_LINE_2, DISP_LINE_3, DISP_LINE_4, DISP_LINE_5, DISP_LINE_6, DISP_LINE_7 };
  renderScreen(&display, &dLines);
}

/* Input Handling */

void processRemoteKeypress(uint8_t inByte)
{

    if (inByte == 18) {
      
      DISP_Port = 0;
      DISP_Slot = 0;

      if (DISP_Screen == numScreens-1){
          DISP_Screen = 0;
      }
        else{
         DISP_Screen++;
      }  

    }
    else if (inByte == 20) {
      requestScreenData();
    }
    else {

      if (DISP_Screen == MENU){
        if (inByte > 0 && inByte < 4) DISP_Screen = inByte;
      } 
      else if (DISP_Screen == ROUTES){

      }
      else if (DISP_Screen == TRANSFORMERS){

        switch(inByte){

          case 0: case 1: case 2: case 3: case 4:
          case 5: case 6: case 7: case 8: case 9:      
            DISP_Transformer = inByte;
          break;
        
          case 10: // CH-
            if (DISP_Port > 0) DISP_Port--;
          break;    
          
          case 11: // CH-
            DISP_CableOrJack = !DISP_CableOrJack;
          break;
          
          case 12: // CH+     
            if (DISP_Port < 15) DISP_Port++;
          break;

          case 13: // Blue Reverse
            if (DISP_Slot > 0) DISP_Slot--;
          break;    

          case 14: //Purple +     
            if (DISP_Slot < 1) DISP_Slot++;
          break;

          case 15: //Green Play/Pause   
            DISP_Mode = !DISP_Mode;
          break;

          case 16: // Purple -
            if (DISP_Transformer > 0) DISP_Transformer--;
          break;    

          case 17: //Purple +     
            if (DISP_Transformer < 11) DISP_Transformer++;
          break;



        }  

      }
      else if (DISP_Screen == SYSEXRAW){

      }

    }

    updateScreen();
}

/* Proces Incoming */

void createTargetTextLine(uint8_t cableOrJack)
{
    uint8_t dtstrt = 9;
    uint16_t BMT = 0;
  
    while (serialMessageBuffer[dtstrt] != 0xF7){
       BMT |= (1 << serialMessageBuffer[dtstrt++]);
    }

   for(uint8_t i=0;i<16;i++){
      if (BMT & (1 << i)){  
              
        char dig1 = getdigit(i,0)+'0';
        char dig2 = getdigit(i,1)+'0'; 

        if (i>=10){       
          targetsTxt[targetsTxtPos++] = dig2;
          targetsTxt[targetsTxtPos++] = dig1;
          targetsTxt[targetsTxtPos++] = ' ';
        } else {
          targetsTxt[targetsTxtPos++] = dig1;
          targetsTxt[targetsTxtPos++] = ' ';
        }
      }
   } 

   if (cableOrJack == CABLE){
      strncpy(cableTargetsTxt, targetsTxt, sizeof(cableTargetsTxt));  
   } else 
   if (cableOrJack == JACK){
      strncpy(jackTargetsTxt, targetsTxt, sizeof(jackTargetsTxt));
   }
}

void createFilterXO(uint8_t filterMask)
{
  for (int i=0;i<4;i++) {
    GLB_Filter_XO.filter[i] = filterMask & (1 << i) ? 'X' : '-';
  }
}

void createTransformerTextLine(uint8_t tSlot)
{

  strncpy(DISP_LINE_3,"RECEIVED", 20);
}

void processMKSerial(uint8_t dataByte) 
{
   serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
   
   if (dataByte == 0xF7) {
      
       serialMessageBufferIDX = 0;
        
       uint8_t RCV_RouteOrFilter = serialMessageBuffer[5];
       uint8_t RCV_CableOrJackSrc = serialMessageBuffer[6];
       uint8_t RCV_Port = serialMessageBuffer[7];
       uint8_t RCV_Byte8 = serialMessageBuffer[8];
        
       if (RCV_RouteOrFilter == ROUTE) { 
           createTargetTextLine(RCV_CableOrJackSrc);
       } else
       if (RCV_RouteOrFilter == FILTER) {  
           createFilterXO(RCV_Byte8);
       } else 
       if (RCV_RouteOrFilter == TRANSFORMER){
           createTransformerTextLine(RCV_Byte8);
       }

       if (--pendingConfigPackets == 0){
         updateScreen();
       }
      
   } 
}

/* Main --------------------------------------------------------- */

void processSerial()
{
  if (Serial1.available() > 0){
    char inByte = Serial1.read();
    processMKSerial(inByte);
  }
}

void loop()
{

  if (irrecv.decode(&results)) {
    uint8_t v = irTranslate(results.value);
    Serial.println(v, DEC);    
    processRemoteKeypress(v); 
    irrecv.resume();
  }

  processSerial(); 
}

void setup()
{
  Serial.begin(9600);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    
  display.clearDisplay();
  display.display();

  updateScreen();
  
  irrecv.enableIRIn();
  
}
