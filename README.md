# ImDuino
ImGui on Arduino example

### This is a fork for do some personal tests and for code changes backup, use at your own risk

## Softraster changed to render by line
### Motivation
My ESP32 can not allocate all the memory needed to render a 320x240 colored TFT screen, rendering by line allows using a smaller buffer size because every line it is sent to the screen and the buffer reused for the next line

![example](ESP32_TFT_TESTS.jpg)
