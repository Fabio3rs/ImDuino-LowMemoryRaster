#pragma once
#ifndef IMGUI_IMPL_SOFTRASTER_H
#define IMGUI_IMPL_SOFTRASTER_H

#include "../imgui/imgui.h"
#include "softraster/softraster.h"
#include "softraster/texture.h"

template <class T> struct ImplSoftRaster {
    T *Screen{};

    bool ImGui_ImplSoftraster_Init(T *screen) {
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
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<T *>(Screen));
    }

    explicit constexpr ImplSoftRaster(T &screen) : Screen(&screen) {}
    ImplSoftRaster() = default;
};

#endif // IMGUI_IMPL_SOFTRASTER_H
