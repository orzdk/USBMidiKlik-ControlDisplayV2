#include <IRremote.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>
#include <libMMM_c.h>

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
enum sc { MENU=0x0, ROUTES=0x1, TRANSFORMERS=0x2, SYSEX=0x3 };
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

uint8_t DISP_Transformers[2];

uint8_t DISP_Cmd[2];
uint8_t DISP_x[2];
uint8_t DISP_y[2];
uint8_t DISP_z[2];
uint8_t DISP_d[2];
uint8_t DISP_s[2];

uint8_t DISP_ParmSel = 0;
uint8_t DISP_ParmVals[3] = {0,0,0};
uint8_t DISP_ParmNegative[3] = {0,0,0};
      
uint16_t GLB_BMT_Cable;
uint16_t GLB_BMT_Jack;
uint8_t GLB_Filter = 0;

uint8_t pendingConfigPackets = 0;
uint8_t serialMessageBuffer[32];
uint8_t serialMessageBufferIDX = 0;

uint8_t targetsTxtPos = 0;

char cableTargetsTxt[20] = "";
char jackTargetsTxt[20] = "";
char targetsTxt[20] = "";

uint8_t messageInProgress;

XO GLB_Filter_XO;

/* Dial buffer stuff */

void resetRouteDialBuffer()
{
  dialBufferPos = 0;
  memset(dialBuffer, 0, sizeof(dialBuffer));
}

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

/* Dump Requests */

void requestPortRouteDump()
{ 
  uint8_t sysex_cableTargets[11] =  { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };
  uint8_t sysex_jackTargets[11] =   { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x1, 0xF7 };
  uint8_t sysex_filterTargets[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x2, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };   
  
  pendingConfigPackets = 3;
  
  Serial3.write(sysex_cableTargets, 11); delay(100);Serial3.flush();
  Serial3.write(sysex_jackTargets, 11);  delay(100);Serial3.flush();
  Serial3.write(sysex_filterTargets, 11);delay(100);Serial3.flush();    
}

void requestPortTransformerDump()
{
  uint8_t sysex_transformers_slot0[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 0, 0xF7 }; 
  uint8_t sysex_transformers_slot1[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 1, 0xF7 }; 

  pendingConfigPackets = 2;
   
  Serial3.write(sysex_transformers_slot0, 11);delay(100);Serial3.flush();  
  Serial3.write(sysex_transformers_slot1, 11);delay(100);Serial3.flush();    
}

void requestScreenData()
{
    Serial.println("requestScreenData()");
  
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
    Serial.print("updateScreen() DISP_SCREEN = ");
    Serial.println(DISP_Screen);
    
    char line2[20] = "";
    char line3[20] = "";
    char line4[20] = "";
    char line5[20] = "";
    
    char pbuffer[1];
    char slbuffer[1];
  
    sprintf(pbuffer, "%d", DISP_Port);
    sprintf(slbuffer, "%d", DISP_Slot);
  
    char cbuffer[1];
    char xbuffer[1];
    char ybuffer[1];
    char zbuffer[1];
    char dbuffer[1];
    char sbuffer[1];
  
    sprintf(cbuffer, "%d", DISP_Cmd[DISP_Slot]);
    sprintf(xbuffer, "%d", DISP_x[DISP_Slot]);
    sprintf(ybuffer, "%d", DISP_y[DISP_Slot]);
    sprintf(zbuffer, "%d", DISP_z[DISP_Slot]);
    sprintf(dbuffer, "%d", DISP_d[DISP_Slot]);
    sprintf(sbuffer, "%d", DISP_s[DISP_Slot]);
  
    char pv0buffer[1];
    char pv1buffer[1];
    char pv2buffer[1];
    
    sprintf(pv0buffer, "%d", DISP_ParmVals[0]);
    sprintf(pv1buffer, "%d", DISP_ParmVals[1]);
    sprintf(pv2buffer, "%d", DISP_ParmVals[2]);
  
    strncpy(DISP_LINE_1, screenTitles[DISP_Screen], sizeof(DISP_LINE_1));
    
    if (DISP_Screen == MENU){

      Serial.println("updateScreen() DISP_Screen == MENU ");
      
      strncpy(DISP_LINE_2, "> 1: Routes", 20);
      strncpy(DISP_LINE_3, "> 2: Transformers", 20);
      strncpy(DISP_LINE_4, "> 3: Sysex", 20);
      strncpy(DISP_LINE_5, "", 20);
      strncpy(DISP_LINE_6, "", 20);
      strncpy(DISP_LINE_7, "1-3,EQ", 20);
     
  
    } else if (DISP_Screen == ROUTES){
  
      strcat(line2, DISP_CableOrJack == CABLE ? "CBL " : "JCK "); 
      strcat(line2, pbuffer); 
      strcat(line2, GLB_Filter_XO.filter); 
  
      strcat(line3, "C:");
      strcat(line3, cableTargetsTxt);
      
      strcat(line4, "J:");
      strcat(line4, jackTargetsTxt);
      
      strncpy(DISP_LINE_2, line2, 20);
      strncpy(DISP_LINE_3, line3, 20);
      strncpy(DISP_LINE_4, line4, 20);
      strncpy(DISP_LINE_5, "", 20);
      strncpy(DISP_LINE_6, "", 20);
      strncpy(DISP_LINE_7, "", 20);
  
  
    } else if (DISP_Screen == TRANSFORMERS){
  
      if (DISP_Mode == VIEW){
  
        strcat(line2, DISP_CableOrJack == CABLE ? "CBL " : "JCK ");
        strcat(line2, pbuffer);     
        strcat(line2, " SLT "); 
        strcat(line2, slbuffer);
        strcat(line2, " CMD ");
        strcat(line2, cbuffer);
  
        strcat(line3,cCmd[DISP_Cmd[0]].commandTitle);
        if (DISP_Slot == 0 ) strcat(line3," <-");
  
        strcat(line4,cCmd[DISP_Cmd[1]].commandTitle);
        if (DISP_Slot == 1 ) strcat(line4," <-");
  
        strcat(line5,xbuffer);
        strcat(line5,",");
        strcat(line5,ybuffer);
        strcat(line5,",");
        strcat(line5,zbuffer);
        strcat(line5,",");
        strcat(line5,dbuffer);
        strcat(line5,",");
        strcat(line5,sbuffer);
        
        strncpy(DISP_LINE_2, line2, 20);
        strncpy(DISP_LINE_3, "", 20);
        strncpy(DISP_LINE_4, line3, 20);
        strncpy(DISP_LINE_5, line4, 20);
        strncpy(DISP_LINE_6, line5, 20);
        strncpy(DISP_LINE_7, "[100+] To config", 20);
  
      } else if (DISP_Mode == SET){
  
        if (cCmd[DISP_Cmd[DISP_Slot]].parameterCount > 0){
          strcat(line3, cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[0]);
          strcat(line3, ": ");
          strcat(line3, xbuffer);
          
          strcat(line3, "/");
          if (DISP_ParmNegative[0]) strcat(line3,"-");
          strcat(line3, pv0buffer);
          
          if (DISP_ParmSel == 0) strcat(line3," <-");
          
        }
        
        if (cCmd[DISP_Cmd[DISP_Slot]].parameterCount > 1){
          strcat(line4, cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[1]);
          strcat(line4, ": ");
          strcat(line4, ybuffer);
  
          strcat(line4, "/");
          if (DISP_ParmNegative[1]) strcat(line4,"-");
          strcat(line4, pv1buffer);
  
          if (DISP_ParmSel == 1) strcat(line4," <-");
        
        }
        
        if (cCmd[DISP_Cmd[DISP_Slot]].parameterCount > 2){
          strcat(line5, cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[2]);
          strcat(line5, ": ");
          strcat(line5, zbuffer);
  
          strcat(line5, "/");
          if (DISP_ParmNegative[2]) strcat(line5,"-");
          strcat(line5, pv2buffer);
          
          if (DISP_ParmSel == 2) strcat(line5," <-");
        
        }
  
        strncpy(DISP_LINE_1, cCmd[DISP_Cmd[DISP_Slot]].commandTitle, 20);
        strncpy(DISP_LINE_2, "", 20);    
        strncpy(DISP_LINE_3, line3, 20);
        strncpy(DISP_LINE_4, line4, 20);
        strncpy(DISP_LINE_5, line5, 20);   
        strncpy(DISP_LINE_6, "", 20);
        strncpy(DISP_LINE_7, "Q:[100+],S:[200+]", 20); 
  
    }
    else if (DISP_Screen == SYSEX){
        strncpy(DISP_LINE_2, "", 20);
        strncpy(DISP_LINE_3, "", 20);
        strncpy(DISP_LINE_4, "", 20);
        strncpy(DISP_LINE_5, "", 20);
        strncpy(DISP_LINE_6, "", 20);
        strncpy(DISP_LINE_7, "", 20);
    }    
  }
  
  displayLines dLines = { DISP_LINE_1, DISP_LINE_2, DISP_LINE_3, DISP_LINE_4, DISP_LINE_5, DISP_LINE_6, DISP_LINE_7 };
  renderScreenP(&display, &dLines);
}

/* Input Handling */

void saveTransformer()
{
  uint8_t signMask = 0x00 | DISP_ParmNegative[0] | DISP_ParmNegative[1] << 1 | DISP_ParmNegative[2] << 2;
  uint8_t encoded =  DISP_Cmd[DISP_Slot] == 0xB;  

  uint8_t sysex[16] = {
      0xF0, 0x77, 0x77, 0x78, 0x0F, 0x3,
      DISP_CableOrJack, DISP_Port, DISP_Slot, 
      DISP_Cmd[DISP_Slot], 
      DISP_ParmVals[0], DISP_ParmVals[1], DISP_ParmVals[2], 
      encoded, 
      signMask, 
      0xF7
  };

  Serial.println("SYSEX SEND");

  for (int i=0;i<16;i++){
    if (sysex[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(sysex[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  
  Serial3.write(sysex, 16);
}

void processIRKeypress(uint8_t inByte)
{  
    if (inByte == PURPLE_EQ) {
      
      DISP_Port = 0;
      DISP_Slot = 0;

      if (DISP_Screen == numScreens-1){
          DISP_Screen = 0;
      }
        else{
         DISP_Screen++;
      }  

    }
    else if (inByte == GREEN_PLAY){
      requestScreenData();
    }
    else {

      if (DISP_Screen == MENU){
        if (inByte > 0 && inByte < 4) DISP_Screen = inByte;
      } 
      else if (DISP_Screen == ROUTES){

        switch(inByte){

          case 0: case 1: case 2: case 3: case 4:
          case 5: case 6: case 7: case 8: case 9:      
            dialBuffer[dialBufferPos++] = (char)inByte;
          break;

          case RED_CH_MINUS:
            dialCableOrJack = CABLE;
          break;    
          
          case RED_CH_PLUS:    
            dialCableOrJack = JACK;
          break;

        }

      }
      else if (DISP_Screen == TRANSFORMERS){

        if (DISP_Mode == VIEW){     
             
          switch(inByte){
          
            case RED_CH_MINUS:
              if (DISP_Port > 0) DISP_Port--;
            break;    
            
            case RED_CH:
              DISP_CableOrJack = !DISP_CableOrJack;
            break;
            
            case RED_CH_PLUS:    
              if (DISP_Port < 15) DISP_Port++;
            break;
  
            case BLUE_PREV: 
              DISP_Slot = !DISP_Slot;
            break;    
  
            case BLUE_NEXT:     
              DISP_Slot = !DISP_Slot;
            break;
  
            case PLUS_100:   
              DISP_Mode = !DISP_Mode;
            break;
  
            case PURPLE_MINUS: 
              if (DISP_Cmd[DISP_Slot] > 0) DISP_Cmd[DISP_Slot]--;
            break;    
  
            case PURPLE_PLUS:               
              if (DISP_Cmd[DISP_Slot] < 0xD) DISP_Cmd[DISP_Slot]++;
            break;
  
          }  
        } else if (DISP_Mode == SET){

          switch(inByte){
                 
              case BLUE_PREV: 
                if (DISP_ParmSel > 0) DISP_ParmSel--;
              break;    
    
              case BLUE_NEXT:     
                if (DISP_ParmSel < 2) DISP_ParmSel++;
              break;
  
              case PURPLE_MINUS: 
                  DISP_ParmVals[DISP_ParmSel]--;
              break;    
    
              case PURPLE_PLUS:               
                DISP_ParmVals[DISP_ParmSel]++;
              break;

              case RED_CH_MINUS: 
                  DISP_ParmNegative[DISP_ParmSel] = 0;
              break;    
    
              case RED_CH_PLUS:               
                  DISP_ParmNegative[DISP_ParmSel] = 1;
              break;
              
              case PLUS_100:   
                DISP_Mode = !DISP_Mode;
              break;

              case PLUS_200:   
                saveTransformer();
                DISP_Mode = !DISP_Mode;
              break;
          }
        }
        
      }
      else if (DISP_Screen == SYSEX){
          
      }

    }

    updateScreen();
}

/* Proces Incoming - */

void resetSerialBuffer()
{
    serialMessageBufferIDX = 0;
    messageInProgress = 0;
    memset(serialMessageBuffer, 0, sizeof(serialMessageBuffer));
}

void processSerialBuffer()
{

    uint8_t RCV_RouteOrFilter = serialMessageBuffer[5];
    uint8_t RCV_CableOrJackSrc = serialMessageBuffer[6];
    uint8_t RCV_Port = serialMessageBuffer[7];
    uint8_t RCV_Target = serialMessageBuffer[8];

    if (RCV_RouteOrFilter == ROUTE) { 

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
    
       if (RCV_CableOrJackSrc == CABLE){
          strncpy(cableTargetsTxt, targetsTxt, sizeof(cableTargetsTxt));  
       } else 
       if (RCV_CableOrJackSrc == JACK){
          strncpy(jackTargetsTxt, targetsTxt, sizeof(jackTargetsTxt));
       }
 
    }
    else if (RCV_RouteOrFilter == FILTER) {  
      
      for (int i=0;i<4;i++) {
        GLB_Filter_XO.filter[i] = RCV_Target & (1 << i) ? 'X' : '-';
      }
  
    }  
    else if (RCV_RouteOrFilter == TRANSFORMER){

      uint8_t RCV_Slot = RCV_Target;

      DISP_Cmd[RCV_Slot] = serialMessageBuffer[9];
      DISP_x[RCV_Slot] = serialMessageBuffer[10];
      DISP_y[RCV_Slot] = serialMessageBuffer[11];
      DISP_z[RCV_Slot] = serialMessageBuffer[12];
      DISP_d[RCV_Slot] = serialMessageBuffer[13];
      DISP_s[RCV_Slot] = serialMessageBuffer[14];
         
    }      

    resetSerialBuffer(); 
}

/* Main  */

void processSerialData(uint8_t dataByte) 
{  
   
   if (dataByte == 0xF7 && messageInProgress ) {
      
      serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
      processSerialBuffer();

      if (--pendingConfigPackets == 0){
         updateScreen();
      }
   }
   else if (dataByte == 0xF0) {

      serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
      messageInProgress = 1;

   }
   else if ( messageInProgress ){

      serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
      Serial.print("Added Databyte ");Serial.println(dataByte, HEX); 

   }
}

void processSerial()
{
  if (Serial3.available() > 0){
    char inByte = Serial3.read();
    processSerialData(inByte);
  }
}

void loop()
{
  if (irrecv.decode(&results)) {
    uint8_t v = irTranslate(results.value);
    processIRKeypress(v); 
    irrecv.resume();
  }
  processSerial(); 
}

void setup()
{
  Serial.begin(9600);
Serial.println("alive");
  
  Serial3.begin(31250);
  delay(3000);
  
  Serial.println("alive");
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    
  display.clearDisplay();
  display.display();

  Serial.println("Calling update Screen");
  updateScreen();
  
  irrecv.enableIRIn();
}
