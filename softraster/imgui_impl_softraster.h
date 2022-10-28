#pragma once
#include <cstdint>
#ifndef IMGUI_IMPL_SOFTRASTER_H
#define IMGUI_IMPL_SOFTRASTER_H

#include "../imgui/imgui.h"
#include "softraster/softraster.h"
#include "softraster/texture.h"

template <class T> struct ImplSoftRaster {
    texture_t<T> *Screen{};
    SoftRaster<int32_t, T> raster;

    bool ImGui_ImplSoftraster_Init(texture_t<T> *screen) {
        if (screen != nullptr) {
            Screen = screen;
            return true;
        }
        return false;
    }

    void ImGui_ImplSoftraster_Shutdown() { Screen = nullptr; }

    void ImGui_ImplSoftraster_NewFrame() {
        if (Screen == nullptr) {
            return;
        }

        auto &io = ImGui::GetIO();
        io.DisplaySize.x = Screen->w;
        io.DisplaySize.y = Screen->h;
    }

    void ImGui_ImplSoftraster_RenderDrawData(ImDrawData *draw_data) {
        if (Screen == nullptr) {
            return;
        }

        Screen->clear();
        raster.pscreen = Screen;
        raster.template renderDrawLists<int32_t>(draw_data);
    }

    explicit constexpr ImplSoftRaster(texture_t<T> &screen) : Screen(&screen) {}
    ImplSoftRaster() = default;
};

#endif // IMGUI_IMPL_SOFTRASTER_H
