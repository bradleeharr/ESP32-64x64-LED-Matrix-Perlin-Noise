# ESP32-64x64-LED-Matrix-Perlin-Noise
Perlin noise on a 64x64 LED matrix using an ESP32 microcontroller to generate a natural-looking terrain map

<p align="center">
  <img src="https://github.com/bradleeharr/Perlin-Noise-LED-Matrix/blob/main/20231121_183643.gif?raw=true"/>
</p>
# ESP32-64x64-LED-Matrix-Perlin-Noise

The Perlin noise is used to generate a pseudo-random, natural-looking, continuous noise pattern that can be used for various applications such as terrain generation, texture generation, and more.

## Installation and Running the Project

To run this project, you need to have the ESP32 microcontroller a 64x64 LED matrix display The libraries required are `ESP32-HUB75-MatrixPanel-I2S-DMA.h` and `FastLED.h`.

1. Connect your ESP32 microcontroller to your computer.
2. Open the project in your Arduino IDE.
3. Select the correct board and port in the Arduino IDE.
4. Upload the project to your ESP32 microcontroller.


## Credits

This project is adapted from libraries from two sources:

1. [Aurora](https://github.com/pixelmatix/aurora) by Jason Coon
2. [LedEffects Plasma](https://bitbucket.org/ratkins/ledeffects/src/26ed3c51912af6fac5f1304629c7b4ab7ac8ca4b/Plasma.cpp?at=default) by Robert Atkins
