#include "imgui.h"
#include "softraster/softraster/color.h"
#include <cstdint>
texture_alpha8_t fontAtlas;

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <SPI.h>
#include <Wire.h>

// Reconfigured for some personal tests

namespace {
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#define TFT_CS 5
#define TFT_DC 4
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

boolean RecordOn = false;

#define FRAME_X 320
#define FRAME_Y 240

#define SCREENX FRAME_X
#define SCREENY FRAME_Y

texture_color16_t screen;
ImplSoftRaster<color16_t> implRaster(screen);

void drawRGBBitmap(int16_t x, int16_t y, const unsigned char *bitmap, int16_t w,
                   int16_t h) {
    tft.startWrite();
    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            int32_t pos = (j * w + i);
            tft.writePixel(x + i, y, ((const unsigned uint16_t *)bitmap)[pos]);
        }
    }
    tft.endWrite();
}

/*
 * @brief Draws one line
 */
void drawLineCallback(texture_color16_t &screen, int y, color16_t *Line) {
    for (int16_t i = 0; i < screen.w; i++) {
        tft.writePixel(i, y, ((const unsigned uint16_t *)Line)[i]);
    }
}

void screen_init() {
    digitalWrite(22, LOW);
    delay(500);
    digitalWrite(22, HIGH);

    tft.begin();
    tft.fillScreen(ILI9341_BLUE);
    // tft.setFont(Terminal6x8);
    tft.setRotation(3);

    /*
     * @brief callback for draw one line
     */
    screen.lineWritedCb = drawLineCallback;

    screen.init(SCREENX, SCREENY, nullptr); // Sets the raster buffer as nullptr
}

void screen_draw() {
    tft.startWrite();
    implRaster.ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData());
    tft.endWrite();
}

unsigned long drawTime;
unsigned long renderTime;
unsigned long rasterTime;

ImGuiContext *context;
} // namespace

void setup() {
    Serial.begin(115200);

    context = ImGui::CreateContext();

    implRaster.ImGui_ImplSoftraster_Init(&screen);

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
}

float f = 0.0f;
unsigned long t = 0;

void loop() {
    ImGuiIO &io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;

    // io.MousePos = mouse_pos;
    // io.MouseDown[0] = mouse_button_0;
    // io.MouseDown[1] = mouse_button_1;

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

    deltaTime -= (drawTime + renderTime + rasterTime);

    ImGui::Text("Hardware write time %d ms", drawTime);
    ImGui::Text("Render time %d ms", renderTime);
    ImGui::Text("Raster time %d ms", rasterTime);
    ImGui::Text("Remaining time %d ms", deltaTime);
    ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);

    renderTime = millis();
    ImGui::Render();
    renderTime = millis() - renderTime;

    //  tft.startWrite();
    rasterTime = millis();
    drawTime = millis();
    screen_draw();
    drawTime = millis() - drawTime;
    rasterTime = millis() - rasterTime;
}
