/*
 * Parts of this © 2023-5 Bob Gamble

NEOPIXEL BEST PRACTICES for most reliable operation:
- Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
- MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
- NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
- AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
  connect GROUND (-) first, then +, then data.
- When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
  a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
(Skipping these may work OK on your workbench but can fail in the field)

NeoPixel_RP2040_PioX4 - demonstrates the use of the PIO method allowing upto 4 instances
Per one of the two PIO channels, only works on the RP2040

The key part of the method name is Rp2040Pio1X4, 
   Rp2040 (platform specific method),
   PIO Channel 1 (most commonly available), 
   X4 (4 instances allowed)

*/

#include "NPlights.h"
#include "BoardSettings.h"
#include <NeoPixelBusLg.h>
#include "config_mem_helper.h"

//// Debugging -- uncomment to activate debugging statements:
    // dP(x) prints x, 
    // dPH(x) prints x in hex, 
    // dPS(string,x) prints string and x
#define DEBUG Serial
#include "mdebugging.h"           // debugging

#define MAX_GROUPS 6
#define MAX_STRINGS 4
#define MAX_LIGHTS 20
#define MAX_LEADS 3

#define NUM_LUM_EVENTS  2
#define NUM_GROUP_EVENTS MAX_GROUPS * 2
#define NUM_GLOBAL_EVENTS MAX_STRINGS * 7
#define NUM_EVENT NUM_LUM_EVENTS + NUM_GROUP_EVENTS + NUM_GLOBAL_EVENTS 

// Variables 

extern config_mem_t ConfigMemHelper_config_data;

typedef struct {           // strings
  bool ON;
  bool EffectsON;
  bool DimON;
  struct {
    RgbColor color;
    // int effect;
    // bool on;
    struct {  // Neo-Pixels (replicated 20 times)
      bool state;
      uint8_t count;
    } npLead[3];
  } npHead[20];
} npStrings;

bool _changed = false;
uint32_t read_started_ms = millis();
uint32_t _last_change = millis();

RgbColor red(brightnesss, 0, 0);
RgbColor green(0, brightnesss, 0);
RgbColor blue(0, 0, brightnesss);
RgbColor white(brightnesss,brightnesss,brightnesss);
RgbColor black(0,0,0);

unsigned long pixelPrevious = 0;        // Previous Pixel Millis
int           pixelQueue = 0;           // Pattern Pixel Queue
int           pixelCycle = 0;           // Pattern Pixel Cycle
uint16_t      pixelCurrent = 0;         // Pattern Current Pixel Number

bool GroupState[MAX_GROUPS];

npStrings _Strings[MAX_STRINGS];

uint8_t PixelPinA = NeoPixel_PinA;  // pin for the data line, ignored for Esp8266
uint8_t PixelPinB = NeoPixel_PinB;  // pin for the data line, ignored for Esp8266
uint8_t PixelPinC = NeoPixel_PinC;  // pin for the data line, ignored for Esp8266
uint8_t PixelPinD = NeoPixel_PinD;  // pin for the data line, ignored for Esp8266

// NeoPixelBusLg strips using the RP2040 PIO hardware for WS2811 LEDs.
// To use WS2812 LEDs instead, replace Rp2040x4Pio1Ws2811Method with Rp2040x4Pio1Ws2812xMethod.
NeoPixelBusLg<NeoGrbFeature, Rp2040x4Pio1Ws2811Method, NeoGammaNullMethod> stripA(MAX_LIGHTS, PixelPinA);
NeoPixelBusLg<NeoGrbFeature, Rp2040x4Pio1Ws2811Method, NeoGammaNullMethod> stripB(MAX_LIGHTS, PixelPinB);
NeoPixelBusLg<NeoGrbFeature, Rp2040x4Pio1Ws2811Method, NeoGammaNullMethod> stripC(MAX_LIGHTS, PixelPinC);
NeoPixelBusLg<NeoGrbFeature, Rp2040x4Pio1Ws2811Method, NeoGammaNullMethod> stripD(MAX_LIGHTS, PixelPinD);

bool pixelsOn = false;

// ---------------------------------------------------------------------------
//  Safe Show() helpers — skip the PIO send when the pin was never initialised.
//  NeoPixelBus Show() blocks forever in while(!CanShow()){} if Begin() was
//  not called (e.g. NeoPixel_Pin* = UNUSED_PIN on boards without NeoPixels).
// ---------------------------------------------------------------------------
static inline void _showA() { if (PixelPinA != UNUSED_PIN) _showA(); }
static inline void _showB() { if (PixelPinB != UNUSED_PIN) _showB(); }
static inline void _showC() { if (PixelPinC != UNUSED_PIN) _showC(); }
static inline void _showD() { if (PixelPinD != UNUSED_PIN) _showD(); }

/* NeoPixel parameters
*/
// ===== Process Consumer-eventIDs =====
void PixelCallback(uint16_t callin) {  
  uint16_t index;
  index = callin;
// NUM_LUM_EVENTS
  if (index < NUM_LUM_EVENTS){
    if(index==0)  DimmerHigh();      // go to high luminosity
    if(index==1)  DimmerLow();       // go to low luminosity 
    }
  else {
    index = index - NUM_LUM_EVENTS;
    if (index < NUM_GROUP_EVENTS){
      bool pastState = GroupState[index / 2];
      GroupState[index / 2] = (index+1) % 2;
      dP(" Group ="); dP((int) index / 2);
      dP(" flag ="); dP((int) (index+1) % 2); dP("\n");
      if(!pastState == GroupState[index / 2]) {
        // process a change in group state
        GroupStateChange(index / 2);
      }
    }
    else {
      int sindex = index - NUM_GROUP_EVENTS;
      int i = sindex / 7;
      dP(" String ="); dP((int)i); dP("\n");
      switch (sindex % 7) {
        case 0:  // CEID(strings[s].eidAllOn),
        _Strings[i].ON = true;
        TurnOnString(i);
        break;
        case 1:  // CEID(strings[s].eidAllOff),            
        TurnOffString(i);
        _Strings[i].ON = false;
        break;
        case 2:  // CEID(strings[s].eidAllToggle),          
        ToggleString(i);
        break;
        case 3:  // CEID(strings[s].eidEffectsOn),         
        _Strings[i].EffectsON = true;
        break;
        case 4:  // CEID(strings[s].eidEffectsOff),      
        _Strings[i].EffectsON = false;
        break;
        case 5:  // CEID(strings[s].eidEffectsToggle),     
        if (_Strings[i].EffectsON)
          _Strings[i].EffectsON = false;
        else
          _Strings[i].EffectsON = true;
        break;
        case 6:  // CEID(strings[s].eidDimmerToggle)   
        ToggleDimmer(i);   
        break;
        default:
          // do nothing
        break;
      }   
    }
  }
}

void TestPixels()
{                  //  testing code
  RgbColor tempColor;
  uint8_t tempR, tempG, tempB;
  uint16_t pixelCount;
  uint8_t Luminance;
  // _showA();
  tempColor = stripA.GetPixelColor(1);
  pixelCount = stripA.PixelCount();
  Luminance = stripA.GetLuminance();
  tempR = tempColor[ColorIndexR];  
  tempG = tempColor[ColorIndexG];  
  tempB = tempColor[ColorIndexB];
  Serial.println((uint16_t) 1);
  Serial.println((uint16_t) pixelCount);
  Serial.println((uint16_t) Luminance);
  Serial.print((uint8_t) tempR);
  Serial.print((uint8_t) tempG);
  Serial.println((uint8_t) tempB);
  _showA();
  tempColor = stripA.GetPixelColor(1);
  pixelCount = stripA.PixelCount();
  Luminance = stripA.GetLuminance();
  tempR = tempColor[ColorIndexR];  
  tempG = tempColor[ColorIndexG];  
  tempB = tempColor[ColorIndexB];
  Serial.println((uint16_t) 1);
  Serial.println((uint16_t) pixelCount);
  Serial.println((uint16_t) Luminance);
  Serial.print((uint8_t) tempR);
  Serial.print((uint8_t) tempG);
  Serial.println((uint8_t) tempB);
}

void TestPixels2()
{                  //  testing code
  RgbColor tempColor;
  uint8_t tempR, tempG, tempB;
  uint16_t pixelCount;
  uint8_t Luminance;
  _showA();
  tempColor = getColor(1, 1);
  tempR = tempColor[ColorIndexR];
  tempG = tempColor[ColorIndexG];
  tempB = tempColor[ColorIndexB];
  Serial.println((uint16_t) 1);
  Serial.print((uint8_t) tempR);
  Serial.print((uint8_t) tempG);
  Serial.println((uint8_t) tempB);
  tempColor = getColor(1, 1);
  tempR = tempColor[ColorIndexR];  
  tempG = tempColor[ColorIndexG];  
  tempB = tempColor[ColorIndexB];
  Serial.println((uint16_t) 1);
  Serial.print((uint8_t) tempR);
  Serial.print((uint8_t) tempG);
  Serial.println((uint8_t) tempB);
}


void SetupPixels()
{
  // Begin() programs the RP2040 PIO state machine for each strip.
  // Calling it with UNUSED_PIN (127) would configure a non-existent GPIO
  // and corrupt the PIO, causing all subsequent Show() calls to hang.
  if (PixelPinA != UNUSED_PIN) { stripA.Begin(); stripA.SetLuminance(MAX_LUMINANCE); }
  if (PixelPinB != UNUSED_PIN) { stripB.Begin(); stripB.SetLuminance(MAX_LUMINANCE); }
  if (PixelPinC != UNUSED_PIN) { stripC.Begin(); stripC.SetLuminance(MAX_LUMINANCE); }
  if (PixelPinD != UNUSED_PIN) { stripD.Begin(); stripD.SetLuminance(MAX_LUMINANCE); }
}

void ResetStrings()
{
  // This will force the internal state to think that Show() has happened and there is nothing new to send.
  stripA.ResetDirty();
  stripB.ResetDirty();
  stripC.ResetDirty();
  stripD.ResetDirty();
}

void initStringFlags()
{
  // dP("\n  Initialize String and Head flags "); dP("\n");

  for(uint8_t i = 0; i < MAX_STRINGS; i++) {
    _Strings[i].ON = true;
    _Strings[i].EffectsON = true;
    _Strings[i].DimON = false;
   for(uint8_t j = 0; j < MAX_LIGHTS; j++) {   
      for(uint8_t k = 0; k < MAX_LEADS; k++) {
        _Strings[i].npHead[j].npLead[k].state = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].starting_cycle;
        _Strings[i].npHead[j].npLead[k].count = 0;
      }
    } 
  }
  for(uint8_t i = 0; i < MAX_GROUPS; i++) {
    GroupState[i] = true;
  }
}
void InitialzePixels()
{	  
  stripA.ClearTo(black); // Set all pixel colors to 'off'
  // if(stripA.IsDirty() || stripA.CanShow()) 
  _showA();
  stripB.ClearTo(black); // Set all pixel colors to 'off'
  // if(stripB.IsDirty() || stripB.CanShow()) 
  _showB();
  stripC.ClearTo(black); // Set all pixel colors to 'off'
  // if(stripC.IsDirty() || stripC.CanShow()) 
  _showC();
  stripD.ClearTo(black); // Set all pixel colors to 'off'
  // if(stripD.IsDirty() || stripD.CanShow()) 
  _showD();
  TurnOnPixels();
    Serial.println();
    Serial.println("Running...");
} 
RgbColor getColor(int i, uint16_t j)
{
  RgbColor tempColor;
  switch (i) {
    case 0:
      _showA();
      tempColor = stripA.GetPixelColor(j);
    break;
    case 1:
      tempColor = stripB.GetPixelColor(j); 
    break;
    case 2:
      tempColor = stripC.GetPixelColor(j); 
    break;
    case 3:
      tempColor = stripD.GetPixelColor(j);  
    break;
    default:
      tempColor = RgbColor(ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].intensity);
    break;
  } 
  return tempColor;
}
void ProcessPixels()
{
  unsigned long currentMillis = millis();                     //  Update current time;
  if(currentMillis - pixelPrevious >= ConfigMemHelper_config_data.attributes.effect_cycle_frequency) {        //  Check for expired time to process a cycle
    pixelPrevious = currentMillis;
    for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount
    // process a string if effects are on
      if (_Strings[i].ON) { 
        if (_Strings[i].EffectsON) { 
          ProcessEffects (i);
        }
        else {
          ProcessHeads (i);
        }
        switch (i) {
          case 0:
          _showA();
          break;
          case 1:
          _showB();
          break;
          case 2:
          _showC();
          break;
          case 3:
          _showD();
          break;
          default:
            // do nothing
          break;
        }   
      }
      else
      {        // clear to black        
        switch (i) {
          case 0:
          stripA.ClearTo(black); // Set all pixel colors to 'off'
          _showA();
          break;
          case 1:  
          stripB.ClearTo(black); // Set all pixel colors to 'off'
          _showB();
          break;
          case 2: 
          stripC.ClearTo(black); // Set all pixel colors to 'off'
          _showC();
          break;
          case 3:
          stripD.ClearTo(black); // Set all pixel colors to 'off'
          _showD();
          break;
          default:
          // do nothing
          break;
        }  
      }
    }
  }    
}

void ProcessEffects (int i)
{// process effect // effect (0=off; 1=constant; 2= blink; 3=flicker)
  RgbColor tempColor;
  uint8_t tempR, tempG, tempB;
// process a string head by head
  for (uint16_t j = 0; j < ConfigMemHelper_config_data.strings.string[i].number_of_heads; j++)
  {
    tempColor = RgbColor(ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].intensity);
    tempR = tempColor[ColorIndexR];
    tempG = tempColor[ColorIndexG];
    tempB = tempColor[ColorIndexB];
    
    for(uint8_t k = 0; k < MAX_LEADS; k++) {
    if (GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].group])    //if the group is set ON
    switch (ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].effect) { 
      case 0:// effect 0=off
        // 
        if(k == 0) tempR = 0;
        if(k == 1) tempG = 0;
        if(k == 2) tempB = 0;
      break;
      case 1:// effect  1=constant
        //  
      break;
      case 2:// effect  2= blink      
            // Serial.print( (uint8_t)_Strings[i]._Head[j].Rcount);        
        if(_Strings[i].npHead[j].npLead[k].count <= 0)   {           
          if(_Strings[i].npHead[j].npLead[k].state){
            _Strings[i].npHead[j].npLead[k].state = false;
            _Strings[i].npHead[j].npLead[k].count = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].cycles_off;
          }
          else {
            _Strings[i].npHead[j].npLead[k].state = true;
            _Strings[i].npHead[j].npLead[k].count = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].cycles_on;
          }       
        }    
        if (!_Strings[i].npHead[j].npLead[k].state) {
          if(k == 0) tempR = 0;
          if(k == 1) tempG = 0;
          if(k == 2) tempB = 0;};
      break;
      case 3:// effect  3=flicker      
        if(_Strings[i].npHead[j].npLead[k].count <= 0)   {           
          if(_Strings[i].npHead[j].npLead[k].state){
            _Strings[i].npHead[j].npLead[k].state = false;
            _Strings[i].npHead[j].npLead[k].count = random(1,ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].cycles_off);

          }
          else {
            _Strings[i].npHead[j].npLead[k].state = true;
            _Strings[i].npHead[j].npLead[k].count = random(1,ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[k].cycles_on);
          }       
        }    
        if (!_Strings[i].npHead[j].npLead[k].state) {
          if(k == 0) tempR = 0;
          if(k == 1) tempG = 0;
          if(k == 2) tempB = 0;};
        
      break;
      default:
        // do nothing
      break;
        }
        else  // group is off
        {
          if(k == 0) tempR = 0;
          if(k == 1) tempG = 0;
          if(k == 2) tempB = 0;;
        } 
    --_Strings[i].npHead[j].npLead[k].count;
    }
    // set new head
    tempColor = RgbColor(tempR, tempG, tempB);
      switch (i) {
        case 0:
          stripA.SetPixelColor(j, tempColor); 
        break;
        case 1:
          stripB.SetPixelColor(j, tempColor); 
        break;
        case 2:
          stripC.SetPixelColor(j, tempColor); 
        break;
        case 3:
          stripD.SetPixelColor(j, tempColor); 
        break;
        default:
          // do nothing
        break;
      }    
  }
}

void ProcessHeads (int i)
{ // process a string head by head
  RgbColor tempColor;
  uint8_t tempR, tempG, tempB;
  for (uint16_t j = 0; j < ConfigMemHelper_config_data.strings.string[i].number_of_heads; j++)
  {
    tempColor = RgbColor(ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].intensity, ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].intensity);
    tempR = tempColor[ColorIndexR];  
    tempG = tempColor[ColorIndexG];  
    tempB = tempColor[ColorIndexB];
    if (!GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].group]) // group is off
    {      tempR = 0;    }
    if (!GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].group]) // group is off
    {      tempG = 0;    }
    if (!GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].group]) // group is off
    {      tempB = 0;    }
    // set new head
    tempColor = RgbColor(tempR, tempG, tempB);
      switch (i) {
        case 0:
          stripA.SetPixelColor(j, tempColor); 
        break;
        case 1:
          stripB.SetPixelColor(j, tempColor); 
        break;
        case 2:
          stripC.SetPixelColor(j, tempColor); 
        break;
        case 3:
          stripD.SetPixelColor(j, tempColor); 
        break;
        default:
          // do nothing
        break;
      }    
  }
}

void MakeDirty()
{  
  for (int i = 0; i <= ConfigMemHelper_config_data.attributes.number_of_strings; i++) 
  { // _StringCount
    switch (i) {
      case 0: 
      stripA.Dirty();
      break;
      case 1:
      stripB.Dirty();
      break;
      case 2: 
      stripC.Dirty();
      break;
      case 3:
      stripD.Dirty();
      break;
      default:
        // do nothing
      break;
    }
  }
}

void GroupStateChange(int g)   // toggle a group in all strings
{    
  for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount
    ToggleStringGroup(g, i); // toggle all the pixels in a group
  }
  Serial.print("Change Group ");
  Serial.println((int) g);
}

void ToggleStringGroup(int g, int i){  
  RgbColor tempColor;
  uint16_t tempR, tempG, tempB;
  for (int j = 0; j < ConfigMemHelper_config_data.strings.string[i].number_of_heads; j++) // set color
  { //Serial.print((int)j);
      if (GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].group])    //if the group is set ON
      {        tempR = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[0].intensity;      }
      else
      {        tempR = 0;      }
      if (GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].group])    //if the group is set ON
      {        tempG = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[1].intensity;      }
      else
      {        tempG = 0;      }
      if (GroupState[ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].group])    //if the group is set ON
      {        tempB = ConfigMemHelper_config_data.strings.string[i].neo_pixels[j].head_lead[2].intensity;      }
      else
      {        tempB = 0;      }          
      // set new head
    tempColor = RgbColor(tempR, tempG, tempB);
    switch (i) {
      case 0:
        stripA.SetPixelColor(j, tempColor); 
      break;
      case 1:
        stripB.SetPixelColor(j, tempColor); 
      break;
      case 2:
        stripC.SetPixelColor(j, tempColor); 
      break;
      case 3:
        stripD.SetPixelColor(j, tempColor); 
      break;
      default:
        // do nothing
      break;
    }          
  } // end for
  if (_Strings[i].ON) {
    switch (i) {
      case 0:
        _showA();
        Serial.print("A Group set ... ");
      break;
      case 1:
        _showB();
        Serial.print("B Group set ... ");
      break;
      case 2:
        _showC();
        Serial.print("C Group set ... ");
      break;
      case 3:
        _showD();
        Serial.print("D Group set ... ");
      break;
      default:
        // do nothing
      break;
    }  
        Serial.println((int) g);
  }
}

void TurnOnPixels()   // turn on the pixels
{    
  for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount
    _Strings[i].ON = false;
    TurnOnString(i); // turn on all the pixels
  }
  pixelsOn = true;
  Serial.println("All On ...");
}

void TurnOnString(int i)   // turn on the pixels
{Serial.print((int)i);
 // turn on all the pixels
  if (!_Strings[i].ON) {
    for (int j = 0; j < ConfigMemHelper_config_data.strings.string[i].number_of_heads; j++) // set color
    { //Serial.print((int)j);
      switch (i) {
        case 0:
          stripA.SetPixelColor(j, _Strings[i].npHead[j].color); 
        break;
        case 1:
          stripB.SetPixelColor(j, _Strings[i].npHead[j].color);   
        break;
        case 2:
          stripC.SetPixelColor(j, _Strings[i].npHead[j].color);   
        break;
        case 3:
          stripD.SetPixelColor(j, _Strings[i].npHead[j].color);   
        break;
        default:
          // do nothing
        break;
      }  
    }
      switch (i) {
        case 0:
          _showA();
          Serial.println("A On ...");
        break;
        case 1:
          _showB();
          Serial.println("B On ...");
        break;
        case 2:
          _showC();
          Serial.println("C On ...");
        break;
        case 3:
          _showD();
          Serial.println("D On ...");
        break;
        default:
          // do nothing
        break;
      }  
    _Strings[i].ON = true; 
  }
}
void TurnOffPixels()      // turn off the pixels
{
  for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount
    TurnOffString(i); // turn off all the pixels
  }
  pixelsOn = false;
  Serial.println("All Off ...");
}
void TurnOffString(int i)      // turn off the pixels
{  
  switch (i) {
    case 0:
      stripA.ClearTo(black); // Set all pixel colors to 'off'
      _showA();
      Serial.println("A Off ...");
    break;
    case 1:
      stripB.ClearTo(black); // Set all pixel colors to 'off'
      _showB();
      Serial.println("B Off ...");
    break;
    case 2:
      stripC.ClearTo(black); // Set all pixel colors to 'off'
      _showC();
      Serial.println("C Off ...");
    break;
    case 3:
      stripD.ClearTo(black); // Set all pixel colors to 'off'
      _showD();
      Serial.println("D Off ...");
    break;
    default:
      // do nothing
    break;
  }  
_Strings[i].ON = false; 
}

void TogglePixels()   // toggle the pixels
{
  if (pixelsOn) { 
    pixelsOn = false;
    TurnOffPixels();
    Serial.println("Off ...");
    }
  else {
    pixelsOn = true;
    TurnOnPixels();
    Serial.println("On ...");
    }
}
void ToggleString(int i)   // toggle the pixels
{
   if (_Strings[i].ON) {
    TurnOffString(i);
    }
  else {
    TurnOnString(i);
    }
}
void DimmerHigh()      // turn off the pixels
{
uint8_t luminance;
  for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount    
    if(_Strings[i].ON) _Strings[i].ON = false;  // this is to ensure to execute the turnonstring routine to reload pixel values at new luminance
    {
      _Strings[i].DimON = false;
      luminance = ConfigMemHelper_config_data.attributes.maximum_luminosity; // _HighLuminosity;
    }    
    switch (i) {
      case 0:
        stripA.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showA();
      break;
      case 1:
        stripB.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showB();
      break;
      case 2:
        stripC.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showC();
      break;
      case 3:
        stripD.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showD();
      break;
      default:
        // do nothing
      break;
    }  
  }
  Serial.println("All Bright ...");
}
void DimmerLow()      // turn off the pixels
{
uint8_t luminance;
  for (int i = 0; i < ConfigMemHelper_config_data.attributes.number_of_strings; i++)  { // _StringCount    
    if(_Strings[i].ON) _Strings[i].ON = false;  // this is to ensure to execute the turnonstring routine to reload pixel values at new luminance
    {
      _Strings[i].DimON = true;
      luminance = ConfigMemHelper_config_data.attributes.minimum_luminosity; // _LowLuminosity;
    }   
    switch (i) {
      case 0:
        stripA.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showA();
      break;
      case 1:
        stripB.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showB();
      break;
      case 2:
        stripC.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showC();
      break;
      case 3:
        stripD.SetLuminance(luminance); // Set all pixel colors to 'off'
        TurnOnString(i);
        _showD();
      break;
      default:
        // do nothing
      break;
    }  
  }
  Serial.println("All Dim ...");
}
void ToggleDimmer(int i)   // toggle the pixel brighness
{// strip.SetLuminance(luminance);
uint8_t luminance;
  
  if(_Strings[i].ON) _Strings[i].ON = false;  // this is to ensure to execute the turnonstring routine to reload pixel values at new luminance
  if (_Strings[i].DimON){
    _Strings[i].DimON = false;
    luminance = ConfigMemHelper_config_data.attributes.maximum_luminosity;
  }
  else{
    _Strings[i].DimON = true;
    luminance = ConfigMemHelper_config_data.attributes.minimum_luminosity;
  }
    
  switch (i) {
    case 0:
      stripA.SetLuminance(luminance); // Set all pixel colors to 'off'
      TurnOnString(i);
      _showA();
      Serial.print("A ");
    break;
    case 1:
      stripB.SetLuminance(luminance); // Set all pixel colors to 'off'
      TurnOnString(i);
      _showB();
      Serial.print("B ");
    break;
    case 2:
      stripC.SetLuminance(luminance); // Set all pixel colors to 'off'
      TurnOnString(i);
      _showC();
      Serial.print("C ");
    break;
    case 3:
      stripD.SetLuminance(luminance); // Set all pixel colors to 'off'
      TurnOnString(i);
      _showD();
      Serial.print("D ");
    break;
    default:
      // do nothing
    break;
  }  
      Serial.print(" dimmer ...");
      Serial.println((uint8_t ) luminance);
}
