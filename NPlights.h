/*
 * Parts of this ©  2023-5 Bob Gamble
 *
*/

#ifndef IOFUNCTIONS_H
#define IOFUNCTIONS_H

#include <Arduino.h>
#include <Wire.h>
#include <NeoPixelBusLg.h>

void PixelCallback(uint16_t callin);

void TestPixels();
void TestPixels2();
RgbColor getColor(int i, uint16_t j);

void ResetStrings();
void SetupPixels();
// void ConfigurePixels();
void InitialzePixels();
void initStringFlags();
void ProcessPixels();
void ProcessEffects (int i);
void ProcessHeads (int i);
void MakeDirty();
void TurnOnPixels();
void TurnOnString(int i);
void TurnOffPixels();
void TurnOffString(int i);
void TogglePixels();
void ToggleString(int i);
void ToggleDimmer(int i);   // toggle the pixel brighness
void GroupStateChange(int g);
void DimmerHigh();      // go to high luminosity
void DimmerLow();       // go to low luminosity
void ToggleStringGroup(int g, int i);

#endif
