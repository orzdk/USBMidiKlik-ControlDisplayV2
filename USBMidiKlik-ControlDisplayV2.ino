#include <IRremote.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>
#include <libMMM_c.h>
#include "cd_ir.h"
#include "cd_calc.h"
#include "cd_screen.h"

#define RECV_PIN 8
#define TRANSFORMERS_PR_CHANNEL 2

Adafruit_SSD1306 display(-1);

IRrecv irrecv(RECV_PIN);
decode_results results;

enum cj { CABLE=0x0, JACK=0x1 };
enum rf { ROUTE=0x1, FILTER=0x2, TRANSFORMER=0x3 };
enum sc { MENU=0x0, ROUTES=0x1, TRANSFORMERS=0x2, SYSEX=0x3, MISC=0x4 };
enum dm { VIEW=0x0, SET=0x1 };

uint8_t numScreens = 5;

uint8_t dialBuffer[2];
uint8_t dialBufferPos = 0;
uint8_t dialCableOrJack = 0;

char slines[7][20] = {"","","","","","",""};
uint8_t DISP_Cmd[2];
uint8_t DISP_ParmVals[3][2];
uint8_t DISP_ParmVals_Sign[3][2];

char filter[4];

uint8_t DISP_Screen = 0;
uint8_t DISP_CableOrJack = 0;
uint8_t DISP_Port = 0;
uint8_t DISP_Slot = 0;
uint8_t DISP_Mode = 0;
uint8_t DISP_ParmSel = 0;

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

/* Utils */

void printSerialBuffer()
{

    for (int i=0;i<32;i++){
      if (serialMessageBuffer[i] < 0x10) {
        Serial.print("0");
      }
      Serial.print(serialMessageBuffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
}

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
  uint8_t sysex_transformers_slot0[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 0x0, 0xF7 }; 
  uint8_t sysex_transformers_slot1[11] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x1, 0x3, DISP_CableOrJack, DISP_Port, 0x1, 0xF7 }; 

  pendingConfigPackets = 2;
   
  Serial3.write(sysex_transformers_slot0, 11);delay(100);Serial3.flush();  
  Serial3.write(sysex_transformers_slot1, 11);delay(100);Serial3.flush();    
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
    Serial.print("updateScreen() DISP_SCREEN = ");Serial.println(DISP_Screen);
    
    memset(slines, 0, sizeof(slines[0][0]) * 20 * 7);
    memset(DISP_ParmVals, 0, sizeof(DISP_ParmVals[0][0]) * 3 * 2);
    memset(DISP_ParmVals_Sign, 0, sizeof(DISP_ParmVals_Sign[0][0]) * 3 * 2);
    
    char portbuffer[2];
    char slotbuffer[2];
    char cmdbuffer[2];
    
    char parmvals_buffer[3][2] = {0,0,0};
    char dialvals_buffer[2][2] = {0,0};
      
    sprintf(portbuffer, "%d", DISP_Port);
    sprintf(slotbuffer,"%d", DISP_Slot);
    sprintf(cmdbuffer, "%d", DISP_Cmd[DISP_Slot]);
    
    sprintf(dialvals_buffer[0], "%d", dialBuffer[0]);
    sprintf(dialvals_buffer[1], "%d", dialBuffer[1]);

    sprintf(parmvals_buffer[0], "%d", DISP_ParmVals[0][DISP_Slot]);
    sprintf(parmvals_buffer[1], "%d", DISP_ParmVals[1][DISP_Slot]);
    sprintf(parmvals_buffer[2], "%d", DISP_ParmVals[2][DISP_Slot]);

    if (DISP_Screen == MENU){
      strcat(slines[0], "-- Menu --");
      strcat(slines[1], "> 1: Routes"); 
      strcat(slines[2], "> 2: Transformers"); 
      strcat(slines[3], "> 3: Sysex"); 
      strcat(slines[4], "> 4: Options"); 
      strcat(slines[5], ""); 
      strcat(slines[6], "1-4,EQ");   
  
    } 
    else if (DISP_Screen == ROUTES){
      strcat(slines[0], "-- Routes --");
      strcat(slines[1], DISP_CableOrJack == CABLE ? "CABLE " : "JACK "); 
      strcat(slines[1], portbuffer);
//      strcat(slines[1], "        ");
//      strcat(slines[1], filter); 
  
      strcat(slines[2], "C: ");
      strcat(slines[2], cableTargetsTxt);
      
      strcat(slines[3], "J: ");
      strcat(slines[3], jackTargetsTxt);

      strcat(slines[6], dialCableOrJack == CABLE ? "CBL " : "JCK ");  
      strcat(slines[6], dialvals_buffer[0]);  
      strcat(slines[6], dialvals_buffer[1]);  
      strcat(slines[6], "<-[+200]");
  
    } 
    else if (DISP_Screen == TRANSFORMERS){
      
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
    
          strcat(slines[4],parmvals_buffer[0]);
          strcat(slines[4],",");
          strcat(slines[4],parmvals_buffer[1]);
          strcat(slines[4],",");
          strcat(slines[4],parmvals_buffer[2]);
          
          strcat(slines[6], "CFG: [100+]");        
        } 
        else if (DISP_Mode == SET){
          
          strcat(slines[0], cCmd[DISP_Cmd[DISP_Slot]].commandTitle);
          strcat(slines[6], "C: [100+], S: [200+]");
       
          for (int i=0; i < cCmd[DISP_Cmd[DISP_Slot]].parameterCount; i++)
          {  
            strcat(slines[2+i], cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[0]);
            strcat(slines[2+i], ": ");
  
            if ( DISP_ParmVals_Sign[i][DISP_Slot] ) strcat(slines[2+i],"-");
            strcat(slines[2+i], parmvals_buffer[i]);
            
            if (DISP_ParmSel == 0) strcat(slines[2+i]," <-");
  
          } 
        }
        
    }
    else if (DISP_Screen == SYSEX){
        strcat(slines[0], "-- Sysex --");
    }  
    else if (DISP_Screen == MISC){
        strcat(slines[0], "-- Misc Options --");
    }  

    renderScreenP(&display, slines);
}

/* Input Handling */

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

void toggleFilter(uint8_t filterBit)
{
  
    GLB_Filter ^= (1 << filterBit);
    uint8_t sysexConfig[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x2, DISP_CableOrJack, DISP_Port, GLB_Filter, 0xF7};

    delay(250);
  
    resetRouteDialBuffer();
    requestPortRouteDump();
   
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

        targetsTxtPos = 0;
        GLB_BMT_Cable = 0;
        GLB_BMT_Jack = 0;
        memset(targetsTxt,0,sizeof(targetsTxt));
        memset(cableTargetsTxt,0,sizeof(cableTargetsTxt));
        memset(jackTargetsTxt,0,sizeof(jackTargetsTxt));
      
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
          strncpy(cableTargetsTxt, targetsTxt, sizeof(cableTargetsTxt));  
        } else if (RCV_Target == JACK){
          GLB_BMT_Jack = BMT;
          strncpy(jackTargetsTxt, targetsTxt, sizeof(jackTargetsTxt));
        }
 
    }
    else if (RCV_RouteOrFilter == FILTER) {  
      
      for (int i=0;i<4;i++) {
        filter[i] = RCV_Target & (1 << i) ? 'X' : '-';
      }
  
    }  
    else if (RCV_RouteOrFilter == TRANSFORMER){

      DISP_Cmd[RCV_Target] = serialMessageBuffer[10];
      DISP_ParmVals[0][RCV_Target] = serialMessageBuffer[11];
      DISP_ParmVals[1][RCV_Target] = serialMessageBuffer[12];
      DISP_ParmVals[2][RCV_Target] = serialMessageBuffer[13];
      DISP_ParmVals[3][RCV_Target] = serialMessageBuffer[14];
    
    }      
    
    resetSerialBuffer(); 
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
   
    Serial3.write(sysexComplete, sz);
    delay(250);
    
    resetRouteDialBuffer();
    requestPortRouteDump();
}

/* Main  */

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
            if (dialBufferPos == 2) dialBufferPos = 0;
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

          case PURPLE_MINUS: 
            dialCableOrJack = !dialCableOrJack;
          break;    

          case PURPLE_PLUS:     
            dialCableOrJack = !dialCableOrJack;
          break;

          case PLUS_200:   
            processRouteDialBuffer();
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
              if (DISP_Cmd[DISP_Slot] > 0) DISP_Cmd[DISP_Slot]--; else DISP_Cmd[DISP_Slot] = 15;
            break;    
  
            case PURPLE_PLUS:               
              if (DISP_Cmd[DISP_Slot] < 0xD) DISP_Cmd[DISP_Slot]++; else DISP_Cmd[DISP_Slot] = 0;
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
                  DISP_ParmVals[DISP_ParmSel][DISP_Slot]--;
              break;    
    
              case PURPLE_PLUS:               
                DISP_ParmVals[DISP_ParmSel][DISP_Slot]++;
              break;

              case RED_CH_MINUS: 
                  DISP_ParmVals_Sign[DISP_ParmSel][DISP_Slot] = 1;
              break;    
    
              case RED_CH_PLUS:               
                  DISP_ParmVals_Sign[DISP_ParmSel][DISP_Slot] = 0;
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
