/*
 * Portions of this code are adapted from Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from LedEffects Plasma by Robert Atkins: https://bitbucket.org/ratkins/ledeffects/src/26ed3c51912af6fac5f1304629c7b4ab7ac8ca4b/Plasma.cpp?at=default
 * Copyright (c) 2013 Robert Atkins
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 

// HUB75E pinout
// R1 | G1
// B1 | GND
// R2 | G2
// B2 | E
//  A | B
//  C | D
// CLK| LAT
// OE | GND

/*  Default library pin configuration for the reference
  you can redefine only ones you need later on object creation

#define R1 25
#define G1 26
#define BL1 27
#define R2 14
#define G2 12
#define BL2 13
#define CH_A 23
#define CH_B 19
#define CH_C 5
#define CH_D 17
#define CH_E -1 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
#define CLK 16
#define LAT 4
#define OE 15

*/


#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <FastLED.h>

// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64  	// Panel height of 64 will required PIN_E to be defined.
#define PANELS_NUMBER 2 	// Number of chained panels, if just a single panel, obviously set to 1
#define PIN_E 32
#define NOISE_WIDTH 128  // Width of the Perlin noise array
#define NOISE_HEIGHT 64  // Height of the Perlin noise array

#define PANE_WIDTH PANEL_WIDTH * PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT


// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;


uint16_t time_counter = 0, cycles = 0, fps = 0;
unsigned long fps_timer;

void setup() {
  
  Serial.begin(115200);
  
  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32-HUB75-MatrixPanel-I2S-DMA DEMO       *"));
  Serial.println(F("*****************************************************"));

  /*
    The configuration for MatrixPanel_I2S_DMA object is held in HUB75_I2S_CFG structure,
    pls refer to the lib header file for full details.
    All options has it's predefined default values. So we can create a new structure and redefine only the options we need 

    // those are the defaults
    mxconfig.mx_width = 64;                   // physical width of a single matrix panel module (in pixels, usually it is always 64 ;) )
    mxconfig.mx_height = 32;                  // physical height of a single matrix panel module (in pixels, usually almost always it is either 32 or 64)
    mxconfig.chain_length = 1;                // number of chained panels regardless of the topology, default 1 - a single matrix module
    mxconfig.gpio.r1 = R1;                    // pin mappings
    mxconfig.gpio.g1 = G1;
    mxconfig.gpio.b1 = B1;                    // etc
    mxconfig.driver = HUB75_I2S_CFG::SHIFT;   // shift reg driver, default is plain shift register
    mxconfig.double_buff = false;             // use double buffer (twice amount of RAM required)
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;// I2S clock speed, better leave as-is unless you want to experiment
  */

  /*
    For example we have two 64x64 panels chained, so we need to customize our setup like this

  */
  HUB75_I2S_CFG mxconfig;
  mxconfig.mx_height = PANEL_HEIGHT;      // we have 64 pix heigh panels
  mxconfig.chain_length = PANELS_NUMBER;  // we have 2 panels chained
  mxconfig.gpio.e = PIN_E;                // we MUST assign pin e to some free pin on a board to drive 64 pix height panels with 1/32 scan
  mxconfig.driver = HUB75_I2S_CFG::FM6124;     // in case that we use panels based on FM6126A chip, we can change that

  // OK, now we can create our matrix object
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  // let's adjust default brightness to about 75%
  dma_display->setBrightness8(92);    // range is 0-255, 0 - 0%, 255 - 100%

  // Allocate memory and start DMA display
  if( not dma_display->begin() )
      Serial.println("****** !KABOOM! I2S memory allocation failed ***********");
 
  
  Serial.println("Fill screen: Neutral White");
  dma_display->fillScreenRGB888(128, 128, 128);
  delay(1000);
  // Set current FastLED palette
  Serial.println("Starting image display...");
  fps_timer = millis();
}

// Global variables for scrolling
int scrollPosition = 0;  // Starting position for scrolling
const int scrollSpeed = 1;  // Number of rows to scroll per frame

uint16_t x_noise = 0;   // X offset in the noise array
uint16_t y_noise = 0;  // Fixed Y offset in the noise array
uint16_t y_noise_speed = 5; // Speed of scrolling
uint16_t x_noise_speed = 5;

uint8_t terrain_noise_array[NOISE_HEIGHT][NOISE_WIDTH];
uint8_t feature_array[NOISE_HEIGHT][NOISE_WIDTH];

uint8_t combinedNoise(int x_offset, int y_offset) {
    // Adjust these scale factors for more variety
    uint8_t zeroNoise = inoise16(x_offset * 450, y_offset * 450) >> 8;
    uint8_t primaryNoise = inoise16(x_offset * 650, y_offset * 650) >> 8;
    uint8_t fineNoise = inoise16(x_offset * 1050, y_offset * 1050) >> 8;
    uint8_t finerNoise = inoise16(x_offset * 2048, y_offset * 2048) >> 8;
    // Combine noises using a different method for more variety
    return ((zeroNoise/4) + (primaryNoise / 4) + (fineNoise / 4) + (finerNoise / 4));
}
enum FeatureType {
    NONE,
    TREE,
    FLOWER
};

void fill_noise16() {
    for (int y = 0; y < NOISE_HEIGHT; y++) {
        int y_offset = y_noise + y;
        for (int x = 0; x < NOISE_WIDTH; x++) {
            int x_offset = x_noise + x;
            uint8_t noise = combinedNoise(x_offset, y_offset);

            terrain_noise_array[y][x] = noise;
            uint8_t randomVal = random8();

            // Determine feature based on noise
            if (noise > 150 && noise < 152 || noise > 155 && noise < 158) { // Adjust this for different thresholds
                feature_array[y][x] = TREE;
            } 
            else {
                feature_array[y][x] = NONE;
            }
        }
    }
    y_noise += y_noise_speed;  // Increment for vertical scrolling
    x_noise += x_noise_speed;
}
const uint8_t oceanThreshold = 130;
const uint8_t desertThreshold = 135;
const uint8_t landThreshold = 150;
const uint8_t mountainThreshold = 195;
const uint8_t highMountainThreshold = 210;
CRGB getTerrainColor(uint8_t noiseValue, int x, int y) {
    uint8_t feature = feature_array[y][x];
    if (feature == TREE) {
        return CRGB::DarkGreen;
    } 
    else if (feature == FLOWER) {
        return CRGB::Red;
    }
    else if (noiseValue < oceanThreshold) {
        uint8_t randomVal = random8();
        if (randomVal < 10) { // Adjust for more or less rifts
            return CRGB::DarkBlue; // Darker blue for deeper areas
        }
        return CRGB::Blue;
    } 
    else if (noiseValue < desertThreshold) {
        return CRGB::Yellow;
    }
    else if (noiseValue < landThreshold) {
        return CRGB::Green;
    } 
    else if (noiseValue < mountainThreshold) {
        return CRGB::Grey;
    } 
    else if (noiseValue < highMountainThreshold) {
        return CRGB::Yellow;
    } 
    else {
        return CRGB::White; // Snow-capped peaks
    }
}

uint16_t counter = 0;
void loop() {
    fill_noise16();
    if (counter % 100 == 0) {
      counter = 0;
      x_noise_speed++;
      y_noise_speed++;
      if (x_noise_speed > 2 || x_noise_speed < -2)
        x_noise_speed *= -1;
      if (y_noise_speed > 2 || y_noise_speed < -2)
        y_noise_speed *= -1;
      if (random(100) < 10) {
        y_noise_speed += 1;
      }  
      if (random(100 < 10)) { 
        x_noise_speed += 1;
      }
    }

    for (int x = 0; x < PANE_WIDTH; x++) {
        for (int y = 0; y < PANE_HEIGHT; y++) {
            CRGB color = getTerrainColor(terrain_noise_array[y][x], x, y);
            dma_display->drawPixelRGB888(x, y, color.r, color.g, color.b);
        }
    }
    counter++;
    delay(10);
}
