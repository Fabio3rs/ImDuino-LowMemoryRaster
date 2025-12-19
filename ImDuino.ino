#include "imgui.h"
#include "softraster/softraster/color.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
texture_alpha8_t fontAtlas;

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>

// Reconfigured for some personal tests

namespace {
#define STMPE_CS 8

// ===== TFT ILI9341 (SPI) =====
#define TFT_CS   27
#define TFT_DC   26
#define TFT_RST  33      // ou -1 se não estiver ligado
#define TFT_BL   32      // opcional (PWM), ou direto em 3V3

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19

// Rotação (0..3)
#define TFT_ROTATION 1   // 1 ou 3 = landscape (320x240)

#define TFT_NATIVE_W 240
#define TFT_NATIVE_H 320

// Se você quer trabalhar “fixo” em landscape:
#define SCREEN_W 320
#define SCREEN_H 240

// ===== Touch XPT2046 (SPI compartilhado) =====
#define TOUCH_CS   25
#define TOUCH_IRQ  34    // opcional (se usar IRQ)

// (opcional) frequência SPI para o ILI9341 no ESP32
#define TFT_SPI_FREQ 40000000  // 40 MHz (padrão típico no ESP32)

//  Adafruit_ILI9341(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
//                   int8_t _RST = -1, int8_t _MISO = -1);
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
//XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

boolean RecordOn = false;

#define FRAME_X 320
#define FRAME_Y 240

#define SCREENX FRAME_X
#define SCREENY FRAME_Y

texture_color16_t screen;
ImplSoftRaster<color16_t> implRaster(screen);

unsigned long rasterTime = 0;
unsigned long lastRasterTime = 0;

/*
 * @brief Draws one line
 */
void drawLineCallback(texture_color16_t &screen, int y, const color16_t *Line, int stripeSize) {
    static int nextY = 0;
    if (y == 0) {
        nextY = 0;
    }

    if (y != nextY) {
        // Serial.printf("Skipped line %d (expected %d)\n", y, nextY);
        nextY = y + stripeSize;
    }

    tft.setAddrWindow(0, y, screen.w, stripeSize);

    rasterTime += micros() - lastRasterTime;
    tft.writePixels((uint16_t*)Line, screen.w * stripeSize, true, false);
    lastRasterTime = micros();
}

void screen_init() {
    //digitalWrite(22, LOW);
    //delay(500);
    //digitalWrite(22, HIGH);
    Serial.begin(115200);
    Serial.println("ILI9341 TFT + XPT2046 Touchscreen Test");

    tft.begin(); // You can pass in a specific SPI frequency

    //tft.setAddrWindow(0, 0, SCREEN_W, SCREEN_H);
    tft.fillScreen(ILI9341_BLUE);
    // tft.setFont(Terminal6x8);
    tft.setRotation(3);

    /*
     * @brief callback for draw one line
     */
    screen.lineWritedCb = drawLineCallback;

    screen.init(SCREENX, SCREENY, nullptr); // Sets the raster buffer as nullptr
}

template<class Arr, size_t N>
size_t arrayElements(const Arr (&)[N]) {
    return N;
}

size_t stripeLines = 16;
size_t lineElements = SCREEN_W * stripeLines;
color16_t *Line = new color16_t[lineElements];
uint32_t *stripesHashes = new uint32_t[SCREEN_W / stripeLines + 1];

void screen_draw() {
    rasterTime = 0;
    tft.startWrite();
    implRaster.ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData(), Line, lineElements, true);
    tft.endWrite();
}

unsigned long drawTime;
unsigned long renderTime;
unsigned long startRenderTime;

ImGuiContext *context;
} // namespace

void setup() {
    Serial.begin(115200);

    context = ImGui::CreateContext();

    implRaster.ImGui_ImplSoftraster_Init(&screen);
    implRaster.stripesHashes = stripesHashes;

    ImGuiStyle &style = ImGui::GetStyle();
    style.AntiAliasedLines = false;
    style.AntiAliasedFill = false;
    style.WindowRounding = 0.0f;

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Flags |=
        ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

    uint8_t *pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    fontAtlas.init(width, height, (alpha8_t *)pixels);
    io.Fonts->TexID = &fontAtlas;

    screen_init();
    lastRasterTime = micros();
    startRenderTime = millis();
}

float f = 0.0f;
unsigned long t = 0;

// FPS Calc
unsigned long frameCount = 0;
unsigned long lastFPS = 0;

void loop() {
    startRenderTime = millis();
    ImGuiIO &io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;

    // io.MousePos = mouse_pos;
    // io.MouseDown[0] = mouse_button_0;
    // io.MouseDown[1] = mouse_button_1;

    frameCount++;
    if (millis() - lastFPS >= 1000) {
        Serial.printf("FPS: %lu\n", frameCount);
        lastFPS = millis();
        frameCount = 0;
    }

    /* [0.0f - 1.0f] */
    io.NavInputs[ImGuiNavInput_Activate] =
        0.0f; // activate / open / toggle / tweak value       // e.g. Circle
              // (PS4), A (Xbox), B (Switch), Space (Keyboard)
    io.NavInputs[ImGuiNavInput_Cancel] =
        0.0f; // cancel / close / exit                        // e.g. Cross
              // (PS4), B (Xbox), A (Switch), Escape (Keyboard)
    io.NavInputs[ImGuiNavInput_Input] =
        0.0f; // text input / on-screen keyboard              // e.g.
              // Triang.(PS4), Y (Xbox), X (Switch), Return (Keyboard)
    io.NavInputs[ImGuiNavInput_Menu] =
        0.0f; // tap: toggle menu / hold: focus, move, resize // e.g. Square
              // (PS4), X (Xbox), Y (Switch), Alt (Keyboard)
    io.NavInputs[ImGuiNavInput_DpadLeft] =
        0.0f; // move / tweak / resize window (w/ PadMenu)    // e.g. D-pad
              // Left/Right/Up/Down (Gamepads), Arrow keys (Keyboard)
    io.NavInputs[ImGuiNavInput_DpadRight] = 0.0f;
    io.NavInputs[ImGuiNavInput_DpadUp] = 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown] = 0.0f;
    io.NavInputs[ImGuiNavInput_TweakSlow] =
        0.0f; // slower tweaks                                // e.g. L1 or L2
              // (PS4), LB or LT (Xbox), L or ZL (Switch)
    io.NavInputs[ImGuiNavInput_TweakFast] =
        0.0f; // faster tweaks                                // e.g. R1 or R2
              // (PS4), RB or RT (Xbox), R or ZL (Switch)

    implRaster.ImGui_ImplSoftraster_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowPos(ImVec2(0.0, 0.0));
    ImGui::SetWindowSize(ImVec2(SCREENX, SCREENY));

    f += 0.05;
    if (f > 1.0f)
        f = 0.0f;

    unsigned int deltaTime = millis() - t;
    t += deltaTime;

    unsigned long rasterTimeMs = rasterTime / 1000;

    deltaTime -= (drawTime + renderTime + rasterTimeMs);

    ImGui::Text("Hardware write time %d ms", drawTime);
    ImGui::Text("Render time %d ms", renderTime);
    ImGui::Text("Raster time %d ms", rasterTimeMs);
    ImGui::Text("Remaining time %d ms", deltaTime);
    ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);

    ImGui::Render();
    renderTime = millis() - startRenderTime;

    //  tft.startWrite();
    drawTime = millis();
    screen_draw();
    drawTime = millis() - drawTime;
}
