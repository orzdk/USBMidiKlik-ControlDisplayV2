#include <IRremote.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>
#include <libMMM_c.h>

#include "cd_ir.h"
#include "cd_calc.h"
#include "cd_screen.h"

#define RECV_PIN 8
#define TRANSFORMERS_PR_CHANNEL 2
#define AUTOREFRESH 1

Adafruit_SSD1306 display(-1);

IRrecv irrecv(RECV_PIN);
decode_results results;

enum cj { CABLE=0x0, JACK=0x1 };
enum rf { ROUTE=0x1, FILTER=0x2, TRANSFORMER=0x3 };
enum sc { MENU=0x0, ROUTES=0x1, TRANSFORMERS=0x2, MISC=0x3, SYSEX=0x4, MONITOR=0x5 };
enum dm { VIEW=0x0, SET=0x1 };

uint8_t numScreens = 6;

uint8_t dialBuffer[3];
uint8_t dialBufferPos = 0;

uint8_t dialHexBuffer[32];
uint8_t dialHexBufferPos = 0;

char slines[7][20] = {"","","","","","",""};
uint8_t DISP_Cmd[2];
uint8_t DISP_ParmVals[4][1];
uint8_t DISP_ParmVals_Sign[3][2];

char filter[5];

uint8_t DISP_Screen = 0;
uint8_t DISP_CableOrJack = 0;
uint8_t DISP_Port = 0;
uint8_t DISP_Slot = 0;
uint8_t DISP_Mode = 0;
uint8_t DISP_ParmSel = 0;

char DISP_CableTargets[20] = "";
char DISP_JackTargets[20] = "";

uint16_t GLB_BMT_Cable;
uint16_t GLB_BMT_Jack;
uint8_t GLB_Filter = 0;

uint8_t pendingConfigPackets = 0;
uint8_t serialMessageBufferIDX = 0;
uint8_t serialMessageBuffer[32];

uint8_t messageInProgress;

/* Dial buffer stuff */

void resetRouteDialBuffer()
{
  dialBufferPos = 0;
  memset(dialBuffer, 0, sizeof(dialBuffer));
}

void resetSerialBuffer()
{
    serialMessageBufferIDX = 0;
    messageInProgress = 0;
    memset(serialMessageBuffer, 0, sizeof(serialMessageBuffer));
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

  GLB_BMT_Cable = 0;
  GLB_BMT_Jack = 0;
  GLB_Filter = 0;
  memset(DISP_CableTargets,0,sizeof(DISP_CableTargets));
  memset(DISP_JackTargets,0,sizeof(DISP_JackTargets));
        
  uint8_t sysex_cableTargets[11] =       { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };
  uint8_t sysex_jackTargets[11] =        { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x1, DISP_CableOrJack, DISP_Port, 0x1, 0xF7 };
  uint8_t sysex_filterTargets[11] =      { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x2, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 };   
  
  pendingConfigPackets = 3;
 
  Serial3.write(sysex_cableTargets, 11); delay(100);Serial3.flush();
  Serial3.write(sysex_jackTargets, 11);  delay(100);Serial3.flush();
  Serial3.write(sysex_filterTargets, 11);delay(100);Serial3.flush();    
}

void requestPortTransformerDump()
{
  memset(DISP_ParmVals, 0, sizeof(DISP_ParmVals[0][0]) * 3 * 2);
  memset(DISP_ParmVals_Sign, 0, sizeof(DISP_ParmVals_Sign[0][0]) * 3 * 2);
  
  uint8_t sysex_transformers_slot0[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 }; 
  uint8_t sysex_transformers_slot1[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 0x1, 0xF7 }; 

  pendingConfigPackets = 2;
   
  Serial3.write(sysex_transformers_slot0, 11);delay(100);Serial3.flush();  
  Serial3.write(sysex_transformers_slot1, 11);delay(100);Serial3.flush();    

}

/* Screen Handling */

struct PARMVAL{
  char val[3];
};

void updateScreen()
{ 
    memset(slines, 0, sizeof(slines[0][0]) * 20 * 7);
    
    char portbuffer[2];
    char slotbuffer[2];
    char cmdbuffer[2];
    
    char dialvals_buffer[2][2] = {0,0};

    PARMVAL pv1;
    PARMVAL pv2;
    PARMVAL pv3;
      
    sprintf(portbuffer, "%d", DISP_Port);
    sprintf(slotbuffer,"%d", DISP_Slot);
    sprintf(cmdbuffer, "%d", DISP_Cmd[DISP_Slot]);
    
    sprintf(dialvals_buffer[0], "%d", dialBuffer[0]);
    sprintf(dialvals_buffer[1], "%d", dialBuffer[1]);

    Serial.println("DISP_ParmVals[0][DISP_Slot]");
    Serial.println(DISP_ParmVals[0][DISP_Slot]);
    
    sprintf(pv1.val, "%03i", DISP_ParmVals[0][DISP_Slot]);
    sprintf(pv2.val, "%03i", DISP_ParmVals[1][DISP_Slot]);
    sprintf(pv3.val, "%03i", DISP_ParmVals[2][DISP_Slot]);

    Serial.println("pv1");
    Serial.println(pv1.val);
    
    if (DISP_Screen == MENU){
      strcat(slines[0], "-- UMK --");
      strcat(slines[1], "> 1: Routes"); 
      strcat(slines[2], "> 2: Transformers"); 
      strcat(slines[3], "> 3: Options"); 
      strcat(slines[4], "> 4: (sysex)"); 
      strcat(slines[5], "> 5: (monitor)"); 
      strcat(slines[6], "1-5,EQ");   
  
    } 
    else if (DISP_Screen == ROUTES){
    
      strcat(slines[0], "-- Routes --");
      strcat(slines[1], DISP_CableOrJack == CABLE ? "CABLE " : "JACK "); 
      strcat(slines[1], portbuffer);
      strcat(slines[1], "   ");
      strcat(slines[1], filter);  
      strcat(slines[3], "C: ");
      strcat(slines[3], DISP_CableTargets);
      strcat(slines[4], "J: ");
      strcat(slines[4], DISP_JackTargets);

      if (dialBufferPos == 0) strcat(slines[6], "[0-9]");
      if (dialBufferPos > 0) strcat(slines[6], dialBuffer[0] == CABLE ? "CABLE " : "JACK ");  
      if (dialBufferPos > 1) strcat(slines[6], dialvals_buffer[1]);  
      if (dialBufferPos > 2) strcat(slines[6], dialvals_buffer[2]);  
  
    } 
    else if (DISP_Screen == TRANSFORMERS){

        uint8_t parmCount = cCmd[DISP_Cmd[DISP_Slot]].parameterCount;
        
        if (DISP_Mode == VIEW){
          strcat(slines[0], "-- Transformers --");
          strcat(slines[1], DISP_CableOrJack == CABLE ? "CBL " : "JCK ");
          strcat(slines[1], portbuffer);     
          strcat(slines[1], " SLT "); 
          strcat(slines[1], slotbuffer);
          strcat(slines[1], " CMD ");
          strcat(slines[1], cmdbuffer);
    
          strcat(slines[2],cCmd[DISP_Cmd[0]].commandTitle);
          if (DISP_Slot == 0 ) strcat(slines[2]," <-");
    
          strcat(slines[3],cCmd[DISP_Cmd[1]].commandTitle);
          if (DISP_Slot == 1 ) strcat(slines[3]," <-");
    
          if (parmCount > 0) {
            strcat(slines[4],pv1.val);
          }
          
          if (parmCount > 1) {
            strcat(slines[4],",");
            strcat(slines[4],pv2.val);
          }
          
          if (parmCount > 2) {
            strcat(slines[4],",");
            strcat(slines[4],pv3.val);
          }
         
          strcat(slines[6], "CFG: [100+]");     
             
        } 
        else if (DISP_Mode == SET){
          
          strcat(slines[0], cCmd[DISP_Cmd[DISP_Slot]].commandTitle);
          strcat(slines[6], "C: [100+], S: [200+]");
       
          for (int i=0; i<parmCount; i++)
          {  
            strcat(slines[2+i], cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[i]);
            strcat(slines[2+i], ": ");
           
            if ( DISP_ParmVals_Sign[i][DISP_Slot] ) strcat(slines[2+i],"-");

            if (i==0) strcat(slines[2+i], pv1.val);
            if (i==1) strcat(slines[2+i], pv2.val);
            if (i==2) strcat(slines[2+i], pv3.val);
                                    
            if (DISP_ParmSel == i) strcat(slines[2+i]," <-");
  
          } 
        }
        
    }
    else if (DISP_Screen == SYSEX){
        strcat(slines[0], "-- Sysex Inp --");
        strcat(slines[1], "Not implemented");
        /* HexMode */
    }  
    else if (DISP_Screen == MISC){
        strcat(slines[0], "-- Options --"); 
        strcat(slines[1], "1. 0F - RouteReset");
        strcat(slines[2], "2. 06 - Id request");
        strcat(slines[3], "3. 0A - HW reset");
        strcat(slines[4], "4. 08 - Serialmode");
    }  
    else if (DISP_Screen == MONITOR){
        strcat(slines[0], "-- Monitor --");
        strcat(slines[1], "Not implemented");
        /* HexMode */
    }  
    
    renderScreenP(&display, slines);

}

/* Proces Incoming - */

void processSerialBuffer()
{
    uint8_t RCV_RouteOrFilter = serialMessageBuffer[5];
    uint8_t RCV_CableOrJackSrc = serialMessageBuffer[6];
    uint8_t RCV_Port = serialMessageBuffer[7];
    uint8_t RCV_Target = serialMessageBuffer[8];

    char targetsTxt[20] = "";
    uint8_t targetsTxtPos = 0; 
      
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
   
        if (RCV_Target == CABLE){
          GLB_BMT_Cable = BMT;
          strncpy(DISP_CableTargets, targetsTxt, sizeof(DISP_CableTargets));  
        } else if (RCV_Target == JACK){
          GLB_BMT_Jack = BMT;
          strncpy(DISP_JackTargets, targetsTxt, sizeof(DISP_JackTargets));
        }

    }
    else if (RCV_RouteOrFilter == FILTER) {  
      
      for (int i=0;i<4;i++) {
        filter[i] = RCV_Target & (1 << i) ? 'X' : '-';
      }

      GLB_Filter = RCV_Target;
       
    }  
    else if (RCV_RouteOrFilter == TRANSFORMER){
   
      DISP_Cmd[RCV_Target] = serialMessageBuffer[9];
      DISP_ParmVals[0][RCV_Target] = serialMessageBuffer[10];
      DISP_ParmVals[1][RCV_Target] = serialMessageBuffer[11];
      DISP_ParmVals[2][RCV_Target] = serialMessageBuffer[12];
      DISP_ParmVals[3][RCV_Target] = serialMessageBuffer[13];

    }      
    
    resetSerialBuffer(); 
}

/* Input Handling */

void toggleFilter(uint8_t filterBit)
{  
    GLB_Filter ^= (1 << filterBit);
    uint8_t sysexConfig[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x2, DISP_CableOrJack, DISP_Port, GLB_Filter, 0xF7};

    Serial3.write(sysexConfig,10);  
}

void saveRoute()
{  
    uint8_t src_id = dialBuffer[1] * 10 + dialBuffer[2];
    uint16_t* GLB_BMT = dialBuffer[0] == CABLE ? &GLB_BMT_Cable : &GLB_BMT_Jack;

    Serial.println(*GLB_BMT,BIN);
    Serial.println(src_id,DEC);
    
    *GLB_BMT ^= (1 << src_id);

    Serial.println(*GLB_BMT,BIN);
    
    Serial.println("------------");
    Serial.println("1 << src_id");
    Serial.println(1 << src_id,BIN);
    Serial.println("------------");

    uint8_t numChannels = countSetBits(*GLB_BMT);
    uint8_t sz = numChannels + 10;
    uint8_t numChannel = 9;
    uint8_t sysexConfig[9] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x1, DISP_CableOrJack, DISP_Port, dialBuffer[0]};
    uint8_t sysexEnd[1] = {0xF7};
    uint8_t sysex[sz];

    if (*GLB_BMT){
      memcpy(sysex,sysexConfig,9*sizeof(uint8_t));
      memcpy(sysex+sz-1,sysexEnd,sizeof(uint8_t)); 
      
      for (uint8_t i=0;i<=15;i++){
        if (*GLB_BMT & (1 << i)){
          sysex[numChannel++] = i;
        }
      }
  
      Serial3.write(sysex, sz);
      
    } else {
      
      uint8_t sysexConfig2[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x1, DISP_CableOrJack, DISP_Port, dialBuffer[0], 0xF7};
      Serial3.write(sysexConfig2, 10);

      Serial.println("");
      for (int i=0;i<10;i++) {
        if (sysexConfig2[i]<10) Serial.print("0");
        Serial.print(sysexConfig2[i],HEX);
      }
      Serial.println("");
      
    }
    
}

void saveTransformer()
{
  uint8_t signMask = 0x0 | DISP_ParmVals_Sign[0][DISP_Slot] | DISP_ParmVals_Sign[1][DISP_Slot] << 1 | DISP_ParmVals_Sign[2][DISP_Slot] << 2;

  uint8_t sysex[15] = {
      0xF0, 0x77, 0x77, 0x78, 0x0F, 0x3,
      DISP_CableOrJack, DISP_Port, DISP_Slot, 
      DISP_Cmd[DISP_Slot], 
      DISP_ParmVals[0][DISP_Slot], DISP_ParmVals[1][DISP_Slot], DISP_ParmVals[2][DISP_Slot], 
      signMask, 
      0xF7
  };

  Serial3.write(sysex, 16);
}

/* Main  */

void processIRKeypress(uint8_t inByte)
{   
    uint8_t noUpdate = 0;
      
    if (inByte == PURPLE_EQ) {

      resetRouteDialBuffer(); 
      
      if (DISP_Screen == numScreens-1){
          DISP_Screen = 0;
      }
        else{
         DISP_Screen++;
      }  

      requestPortRouteDump();
    }
    else {

      if (DISP_Screen == MENU){
        if (inByte > 0 && inByte < 4) DISP_Screen = inByte;
      } 
      else if (DISP_Screen == ROUTES){

        switch(inByte){

          case 0: case 1: case 2: case 3: case 4:
          case 5: case 6: case 7: case 8: case 9:
            
            if (dialBufferPos == 1 && inByte > 1) break;
            
            dialBuffer[dialBufferPos++] = inByte;
           
            if(dialBufferPos == 3) {
              saveRoute();
              resetRouteDialBuffer();
            } else {
              noUpdate = 1;
            }
            
          break;

          case GREEN_PLAY:
             requestPortRouteDump();
          break;
            
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
              toggleFilter(0);
          break;    
  
          case BLUE_NEXT:     
              toggleFilter(1);
          break;

          case PURPLE_MINUS: 
             toggleFilter(2);
          break;    

          case PURPLE_PLUS:     
             toggleFilter(3);
          break;

        }

        if (AUTOREFRESH && !noUpdate) requestPortRouteDump();
        
      }
      else if (DISP_Screen == TRANSFORMERS){

        uint8_t parmCount = cCmd[DISP_Cmd[DISP_Slot]].parameterCount;
         
        if (DISP_Mode == VIEW){     
             
          switch(inByte){

            case GREEN_PLAY:
              requestPortTransformerDump();
            break;
          
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
              noUpdate = 1;
            break;
  
            case PURPLE_MINUS: 
              if (DISP_Cmd[DISP_Slot] > 0) DISP_Cmd[DISP_Slot]--; else DISP_Cmd[DISP_Slot] = 15;
              noUpdate = 1;
            break;    
  
            case PURPLE_PLUS:               
              if (DISP_Cmd[DISP_Slot] < 0xD) DISP_Cmd[DISP_Slot]++; else DISP_Cmd[DISP_Slot] = 0;
              noUpdate = 1;
            break;
  
          }
            
        } else if (DISP_Mode == SET){

          noUpdate = 1;
          
          switch(inByte){
                 
              case BLUE_PREV: 
                if (DISP_ParmSel > 0) DISP_ParmSel--; 
              break;    
    
              case BLUE_NEXT:     
                if (DISP_ParmSel < parmCount-1) DISP_ParmSel++; 
              break;
  
              case PURPLE_MINUS: 
                  if (DISP_ParmVals[DISP_ParmSel][DISP_Slot] > 0) DISP_ParmVals[DISP_ParmSel][DISP_Slot]--;
              break;    
    
              case PURPLE_PLUS:      
                if (DISP_ParmVals[DISP_ParmSel][DISP_Slot] < 127) DISP_ParmVals[DISP_ParmSel][DISP_Slot]++;
              break;

              case RED_CH_MINUS: 
                  DISP_ParmVals[DISP_ParmSel][DISP_Slot] = DISP_ParmVals[DISP_ParmSel][DISP_Slot] - 10;
              break;    

              case RED_CH: 
                  DISP_ParmVals_Sign[DISP_ParmSel][DISP_Slot] = !DISP_ParmVals_Sign[DISP_ParmSel][DISP_Slot];
              break;  
              
              case RED_CH_PLUS:               
                  DISP_ParmVals[DISP_ParmSel][DISP_Slot] = DISP_ParmVals[DISP_ParmSel][DISP_Slot] + 10;
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

        if (AUTOREFRESH && !noUpdate) requestPortTransformerDump();
        
      }
      else if (DISP_Screen == SYSEX){
          
      }
      else if (DISP_Screen == MISC){

        uint8_t sysex1[7] = {0xF0,0x77,0x77,0x78,0x0F,0x00,0xF7};// 1. 0F - RouteReset
        uint8_t sysex2[7] = {0xF0,0x77,0x77,0x78,0x06,0x01,0xF7};// 2. 06 - Id request
        uint8_t sysex3[6] = {0xF0,0x77,0x77,0x78,0x0A,0xF7};// 3. 0A - HW reset
        uint8_t sysex4[6] = {0xF0,0x77,0x77,0x78,0x08,0xF7};// 4. 08 - Serialmode
        
        switch(inByte){
          case 1: 
            Serial3.write(sysex1, 7);
            break; 
          case 2: 
            Serial3.write(sysex2, 7);
            break; 
          case 3: 
            Serial3.write(sysex3, 6);
            break; 
          case 4:          
            Serial3.write(sysex4, 6);
            break; 
        }
        
      }

    }

    updateScreen();
      
}

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
   }
}

void loop()
{
  if (irrecv.decode(&results)) {
    uint8_t v = irTranslate(results.value);
    processIRKeypress(v);   
    irrecv.resume();
  }
  
  if (Serial3.available() > 0){
    char inByte = Serial3.read();
    processSerialData(inByte);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial3.begin(31250);
   
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    
  display.clearDisplay();
  display.display();
  
  irrecv.enableIRIn();
  
  updateScreen();
}
