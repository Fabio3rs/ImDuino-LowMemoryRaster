#ifdef __x86_64
/*
 * @brief this file is for test the code on the PC
 */

#include "imgui.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <vector>

texture_alpha8_t fontAtlas;

namespace {
#define FRAME_X 210
#define FRAME_Y 240

#define SCREENX FRAME_X
#define SCREENY FRAME_Y

std::array<uint16_t, FRAME_X * FRAME_Y> pixels;
texture_color16_t screen;
void drawLineCallback(texture_color16_t &screen, int y, color16_t *Line) {
    const auto *lineData = reinterpret_cast<const uint16_t *>(Line);
    std::copy(lineData, lineData + FRAME_X, &(pixels[y * FRAME_X]));
}

ImplSoftRaster implRaster(screen);

void screen_init() {
    screen.lineWritedCb = drawLineCallback;
    screen.init(SCREENX, SCREENY, nullptr);
}

} // namespace

unsigned long drawTime;
unsigned long renderTime;
unsigned long rasterTime;

void setup() {
    auto context = ImGui::CreateContext();

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

    f = 0.0f;

    ImGui::Text("Hardware write time %d ms", 10000);
    ImGui::Text("Render time %d ms", 10000);
    ImGui::Text("Raster time %d ms", 10000);
    ImGui::Text("Remaining time %d ms", 10000);
    ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);

    ImGui::Render();

    //  tft.startWrite();
    auto now = std::chrono::high_resolution_clock::now();
    implRaster.ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData());
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
                     (end - now))
                     .count()
              << std::endl;
    // tft.endWrite();
}

int main() {
    setup();

    loop();

    std::ifstream comparison(std::filesystem::path(MAINSRCDIR) / "tests" /
                                 "compare.data",
                             std::ios::binary | std::ios::in);
    comparison >> std::noskipws;
    assert(comparison.is_open());
    std::istreambuf_iterator itIn(comparison);

    std::vector<uint16_t> data;
    {
        std::vector<char> datatmp(itIn, {});
        data.assign(reinterpret_cast<uint16_t *>(datatmp.data()),
                    reinterpret_cast<uint16_t *>(datatmp.data()) +
                        datatmp.size() / sizeof(uint16_t));
    }
    // std::copy(itIn, {}, outtest);

    std::cout << "data size " << data.size() << " pixels size " << pixels.size()
              << std::endl;

    std::fstream dump("dump.data",
                      std::ios::out | std::ios::trunc | std::ios::binary);
    dump.write(reinterpret_cast<const char *>(pixels.data()),
               screen.size * screen.h * screen.w);

    assert(data.size() == pixels.size());
    assert(std::equal(data.begin(), data.end(), pixels.begin()));
}

#endif
