#include <IRremote.h>
#include <Adafruit_SSD1306_STM32.h>
#include <l3m_c.h>

#include "h_ir.h"
#include "h_calc.h"
#include "h_screen.h"
#include "h_sysex.h"

#define RECV_PIN 8
#define SELECTED_PARMVAL DISP_ParmVals[DISP_ParmSel][DISP_Slot]
#define SELECTED_PARMVAL_SIGN DISP_ParmVals_Sign[DISP_ParmSel][DISP_Slot]

Adafruit_SSD1306 display(-1);

IRrecv irrecv(RECV_PIN);
decode_results results;

enum cj { CABLE=0x0, JACK=0x1 };
enum rf { ROUTE=0x1, FILTER=0x2, TRANSFORMER=0x3 };
enum dm { VIEW=0x0, SET=0x1 };
enum sc { MENU=0x0, ROUTES=0x1, TRANSFORMERS=0x2, MISC=0x3, MONITOR=0x4 };

uint8_t numScreens = 5;

char slines[7][20] = {"","","","","","",""};

uint8_t DISP_Cmd[2];
uint8_t DISP_ParmVals[4][1];
uint8_t DISP_ParmVals_Sign[3][2];

uint8_t DISP_Screen = MENU;
uint8_t DISP_CableOrJack = CABLE;
uint8_t DISP_Mode = VIEW;

uint8_t DISP_Port = 0;
uint8_t DISP_Slot = 0;
uint8_t DISP_ParmSel = 0;

char filter[5];
char DISP_CableTargets[20] = "";
char DISP_JackTargets[20] = "";

uint16_t GLB_BMT_Cable;
uint16_t GLB_BMT_Jack;
uint8_t GLB_Filter = 0;

uint8_t dialBufferidx = 0;
uint8_t dialBuffer[3];

uint8_t serialMessageBufferIDX = 0;
uint8_t serialMessageBuffer[32];

uint8_t justSaved = 0;
uint8_t pendingConfigPackets = 0;

struct PARMVAL{ char val[3]; };

uint8_t MIDIKLIK_VERSION = 20;
uint8_t MIDIKLIK_L3M_TRANSFORMER = 0;

/* Dial buffer stuff */

void resetRouteDialBuffer()
{
  dialBufferidx = 0;
  memset(dialBuffer, 0, sizeof(dialBuffer));
}

void resetSerialBuffer()
{
    serialMessageBufferIDX = 0;
    memset(serialMessageBuffer, 0, sizeof(serialMessageBuffer));
}

/* Dump Request */

void requestData()
{
  
  GLB_BMT_Cable = 0;
  GLB_BMT_Jack = 0;
  GLB_Filter = 0;

  memset(DISP_CableTargets,0,sizeof(DISP_CableTargets));
  memset(DISP_JackTargets,0,sizeof(DISP_JackTargets));
  memset(DISP_ParmVals, 0, sizeof(DISP_ParmVals[0][0]) * 3 * 2);
  memset(DISP_ParmVals_Sign, 0, sizeof(DISP_ParmVals_Sign[0][0]) * 3 * 2);
  
  uint8_t sysex20[6] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0xF7 }; 
  uint8_t sysex25[10] = { 0xF0, 0x77, 0x77, 0x78, 0x5, 0x7F, 0x0, 0x0, 0x0, 0xF7 }; 

  pendingConfigPackets = MIDIKLIK_L3M_TRANSFORMER ? 5 : 3;

  if (MIDIKLIK_VERSION == 20){
    Serial3.write(sysex20, 6);delay(100);Serial3.flush();
  } else if (MIDIKLIK_VERSION == 25){
    Serial3.write(sysex25, 10);delay(100);Serial3.flush();
  }
  
}

/* Screen Handling */

void render()
{   
    uint8_t i=0;
    
    char portbuffer[2];
    char slotbuffer[2];
    char cmdbuffer[2];
    char versionbuffer[2];
    char mkl3mbuffer[1];
    char dialvals_buffer[2][2] = {0,0};
    
    PARMVAL pv[3];
    
    memset(slines, 0, sizeof(slines[0][0]) * 20 * 7);
          
    sprintf(portbuffer, "%d", DISP_Port);
    sprintf(slotbuffer, "%d", DISP_Slot);
    sprintf(cmdbuffer,  "%d", DISP_Cmd[DISP_Slot]);
    
    sprintf(dialvals_buffer[0], "%d", dialBuffer[0]);
    sprintf(dialvals_buffer[1], "%d", dialBuffer[1]);

    sprintf(pv[0].val, "%i", DISP_ParmVals[0][DISP_Slot]);
    sprintf(pv[1].val, "%i", DISP_ParmVals[1][DISP_Slot]);
    sprintf(pv[2].val, "%i", DISP_ParmVals[2][DISP_Slot]);

    sprintf(versionbuffer, "%i", MIDIKLIK_VERSION);
    sprintf(mkl3mbuffer, "%i", MIDIKLIK_L3M_TRANSFORMER);
    
    if (DISP_Screen == MENU){
      
      strcat(slines[0], "-- UMK --");
      strcat(slines[1], "> 1: Routes"); 
      strcat(slines[2], "> 2: Transformers"); 
      strcat(slines[3], "> 3: Options"); 
      strcat(slines[4], "> 4: Monitor"); 
      strcat(slines[6], "1-4, EQ >>");  
       
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

      if (dialBufferidx == 0) strcat(slines[6], "[0-9]");
      if (dialBufferidx > 0) strcat(slines[6], dialBuffer[0] == CABLE ? "CABLE " : "JACK ");  
      if (dialBufferidx > 1) strcat(slines[6], dialvals_buffer[1]);  
      if (dialBufferidx > 2) strcat(slines[6], dialvals_buffer[2]);  
      
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

          while ( cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[i]){
            if (i>0) strcat(slines[4]," ");
            strcat(slines[4], pv[i].val);
            i++;
          }

          if (justSaved) strcat(slines[5],"Saved");
          
          strcat(slines[6], "C:[100+], S:[200+]");     
             
        } 
        else if (DISP_Mode == SET){

          strcat(slines[0], cCmd[DISP_Cmd[DISP_Slot]].commandTitle);
          strcat(slines[6], "C: [100+]");
       
          i=0;

          while ( cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[i] )
          {  
            strcat(slines[2+i], cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[i]);
            strcat(slines[2+i], ": ");
           
            if ( DISP_ParmVals_Sign[i][DISP_Slot] ) strcat(slines[2+i],"-");

            if (i==0) strcat(slines[2+i], pv[0].val);
            if (i==1) strcat(slines[2+i], pv[1].val);
            if (i==2) strcat(slines[2+i], pv[2].val);
                                    
            if (DISP_ParmSel == i) strcat(slines[2+i]," <-");
            i++;
          } 
         
        }           
    }
    else if (DISP_Screen == MISC){
        strcat(slines[0], "-- Options --"); 
        strcat(slines[1], "1. 0F - RouteReset");
        strcat(slines[2], "2. 0A - HW reset");
        strcat(slines[3], "3. 08 - Serialmode");
        strcat(slines[4], "4. MK Version : ");
        strcat(slines[4], versionbuffer);
        strcat(slines[5], "4. l3m : ");
        strcat(slines[5], mkl3mbuffer);
        
        
    } 
    else if (DISP_Screen == MONITOR){
        strcat(slines[0], "-- Monitor --");
        strcat(slines[1], "Not implemented");
    }  

    renderScreenP(&display, slines);   
}

/* Proces Incoming - */

void processSerialBuffer()
{
    
    uint8_t RCV_Function = serialMessageBuffer[4];
    uint8_t RCV_SubFunction = serialMessageBuffer[5];
    uint8_t RCV_CableOrJack = serialMessageBuffer[6];
    uint8_t RCV_Port = serialMessageBuffer[7];
    uint8_t RCV_Target = serialMessageBuffer[8];

    char targetsTxt[20] = "";
    uint8_t targetsTxtPos = 0; 

    if (RCV_Function == 0x6){ //ack
      pendingConfigPackets = 0;
      return;
    }
    
    if ( RCV_Function != 0x0F ) return;   
    if ( DISP_Screen == ROUTES && ( RCV_SubFunction != 0x01 && RCV_SubFunction != 0x02) ) return;
    if ( DISP_Screen == TRANSFORMERS && RCV_SubFunction != 0x03 ) return;
        
    if (!(RCV_Port == DISP_Port && DISP_CableOrJack == RCV_CableOrJack)) return;
    
    if (RCV_SubFunction == ROUTE) { 

        pendingConfigPackets--;

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
        } else if (RCV_Target == JACK ){
           GLB_BMT_Jack = BMT;
           strncpy(DISP_JackTargets, targetsTxt, sizeof(DISP_JackTargets));
        }


    }
    else if (RCV_SubFunction == FILTER) {  

      pendingConfigPackets--;
            
      for (int i=0;i<4;i++) {
        filter[i] = RCV_Target & (1 << i) ? 'X' : '-';
      }

      GLB_Filter = RCV_Target;
       
    }  
    else if (RCV_SubFunction == TRANSFORMER){
   
      DISP_Cmd[RCV_Target] = serialMessageBuffer[9];
      DISP_ParmVals[0][RCV_Target] = serialMessageBuffer[10];
      DISP_ParmVals[1][RCV_Target] = serialMessageBuffer[11];
      DISP_ParmVals[2][RCV_Target] = serialMessageBuffer[12];
      DISP_ParmVals[3][RCV_Target] = serialMessageBuffer[13];
    }      

    resetSerialBuffer();     
}

/* Save Functions */

void saveFilter(uint8_t filterBit)
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
    
    *GLB_BMT ^= (1 << src_id);
    
    Serial.println(*GLB_BMT,BIN);
    
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

      for (uint8_t i=0;i<sizeof(sysex);i++){
        if (sysex[i]<0x10) Serial.print("0");
        Serial.print(sysex[i],HEX);Serial.print(" ");
      }
      Serial.println(" ");
      
    } else {
      
      // uint8_t sysexConfigNone[10] = {0xF0, 0x77, 0x77, 0x78, 0x0F, 0x1, DISP_CableOrJack, DISP_Port, dialBuffer[0], 0xF7};
      // Serial3.write(sysexConfigNone, 10);
     
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

/* Input Handling */

void KEYS_Menu(uint8_t inByte)
{
    if (inByte > 0 && inByte < 4) {
      DISP_Screen = inByte;
    }
}

void KEYS_Routes(uint8_t inByte)
{
    uint8_t noRequest = 0;
    
    switch(inByte){

        case 0: case 1: case 2: case 3: case 4:
        case 5: case 6: case 7: case 8: case 9:
          
          if (dialBufferidx < 2 && inByte > 1) break;
          
          dialBuffer[dialBufferidx++] = inByte;
          
          if(dialBufferidx == 3){
            saveRoute();
            resetRouteDialBuffer();  
          } else noRequest = 1;
          
        break;

        case GREEN_PLAY:
        break;
          
        case RED_CH_MINUS:
          if (DISP_Port > 0) DISP_Port--; else DISP_Port = 15;
        break;    
        
        case RED_CH:
          DISP_CableOrJack = !DISP_CableOrJack;
        break;
        
        case RED_CH_PLUS:    
          if (DISP_Port < 15) DISP_Port++; else DISP_Port = 0;
        break;

        case BLUE_PREV: 
          saveFilter(0);
        break;    

        case BLUE_NEXT:     
          saveFilter(1);
        break;

        case PURPLE_MINUS: 
          saveFilter(2);
        break;    

        case PURPLE_PLUS:     
          saveFilter(3); 
        break;

    }
    
    if (noRequest) {
      render();
    } else {
       requestData(); 
    }
    
}

void KEYS_Transformers_View(uint8_t inByte)
{

    uint8_t noRequest = 0;
   
    switch(inByte){

        case GREEN_PLAY:
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

        case PLUS_200:   
          saveTransformer();
          justSaved = 1;                       
        break;

        case PLUS_100:   
          DISP_Mode = !DISP_Mode;
          noRequest = 1;
        break;

        case PURPLE_MINUS: 
          if (DISP_Cmd[DISP_Slot] > 0) DISP_Cmd[DISP_Slot]--; else DISP_Cmd[DISP_Slot] = 15;
          for (int i=0;i<3;i++) DISP_ParmVals[i][DISP_Slot] = 0;
          noRequest = 1;
        break;    

        case PURPLE_PLUS:               
          if (DISP_Cmd[DISP_Slot] < 0xD) DISP_Cmd[DISP_Slot]++; else DISP_Cmd[DISP_Slot] = 0;
          for (int i=0;i<3;i++) DISP_ParmVals[i][DISP_Slot] = 0;
          noRequest = 1;
        break;

    }
    
    if (noRequest) {
      render();
    } else {
       requestData(); 
    }
}

void KEYS_Transformers_Set(uint8_t inByte)
{

    switch(inByte){
           
        case BLUE_PREV: 
           if (DISP_ParmSel > 0) DISP_ParmSel--; 
        break;    

        case BLUE_NEXT:     
           if (cCmd[DISP_Cmd[DISP_Slot]].parameterTitles[DISP_ParmSel+1]) DISP_ParmSel++;
        break;

        case PURPLE_MINUS: 
           if (SELECTED_PARMVAL > 0) SELECTED_PARMVAL--;
        break;    

        case PURPLE_PLUS:      
           if (SELECTED_PARMVAL < 127) SELECTED_PARMVAL++;
        break;

        case RED_CH_MINUS: 
           if (SELECTED_PARMVAL >= 10) SELECTED_PARMVAL = SELECTED_PARMVAL - 10;
        break;    

        case RED_CH: 
           SELECTED_PARMVAL_SIGN = !SELECTED_PARMVAL_SIGN;
        break;  
        
        case RED_CH_PLUS:               
           if (SELECTED_PARMVAL <= 117) SELECTED_PARMVAL = SELECTED_PARMVAL + 10;
        break;
        
        case PLUS_100:   
          DISP_Mode = !DISP_Mode;
        break;

    }

    render();
}

void KEYS_Misc(uint8_t inByte)
{

  switch(inByte){
    case 1: 
      Serial3.write(sysex_route_reset, 7);
      break; 
    case 2: 
      Serial3.write(sysex_hw_reset, 6);
      break; 
    case 3:          
      Serial3.write(sysex_serial_boot, 6);
      break; 
    case 4:          
      if (MIDIKLIK_VERSION == 20) MIDIKLIK_VERSION = 25;
      else MIDIKLIK_VERSION = 20;
      render();
      break; 
    case 5:          
      if (MIDIKLIK_L3M_TRANSFORMER == 0) MIDIKLIK_L3M_TRANSFORMER = 1;
      else MIDIKLIK_L3M_TRANSFORMER = 0;
      render();
      break; 
  }
}

/* Main  */

void processIRKeypress(uint8_t inByte)
{     
  justSaved = 0;
  
  if (inByte == PURPLE_EQ) {  
      
    if (DISP_Screen == numScreens-1){
        DISP_Screen = 0;
    }
      else{
       DISP_Screen++;
    }  
    
    resetRouteDialBuffer(); 
    render();
    requestData();

  }
  else {

    if (DISP_Screen == MENU){
      KEYS_Menu(inByte);
    } 
    else if (DISP_Screen == ROUTES){
      KEYS_Routes(inByte);   
    }
    else if (DISP_Screen == TRANSFORMERS){
      if (DISP_Mode == VIEW){           
          KEYS_Transformers_View(inByte);
      } else if (DISP_Mode == SET){
          KEYS_Transformers_Set(inByte);
      }
    }
    else if (DISP_Screen == MISC){
      KEYS_Misc(inByte);  
    }

  }  
}

void processSerialData(uint8_t dataByte) 
{     
   if (pendingConfigPackets > 0){

     if (dataByte == 0xF7 && serialMessageBufferIDX > 0) {
        
       serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
       processSerialBuffer();
       resetSerialBuffer();

     }
     else if (dataByte == 0xF0) {
       serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
     }
     else if ( serialMessageBufferIDX > 0){
       serialMessageBuffer[serialMessageBufferIDX++] = dataByte;
     }

  }
  else if (pendingConfigPackets == 0){
      render();
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
  
  render();
}
