#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#ifndef SOFTRASTER_H
#define SOFTRASTER_H

/*
 * @brief original from imgui/misc/softraster/softraster.h changed to raster by
 * line
 */

#include "../../imgui/imgui.h"
#include "barycentric.h"
#include "color.h"
#include "defines.h"
#include "texture.h"
#include "utils.h"
#include <cstring>
#include <utility>
// #include <iostream>

namespace {
#include <stddef.h>
#include <stdint.h>

static inline uint32_t rotl32(uint32_t x, int r) {
    return (x << r) | (x >> (32 - r));
}

// MurmurHash3 x86_32 constants
static constexpr uint32_t C1 = 0xcc9e2d51u;
static constexpr uint32_t C2 = 0x1b873593u;

static inline uint32_t murmur_scramble32(uint32_t k) {
    k *= C1;
    k = rotl32(k, 15);
    k *= C2;
    return k;
}

// fmix32 finalizer constants
static inline uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

struct StripeHash32 {
    uint32_t h = 0;        // estado
    uint32_t lenBytes = 0; // total alimentado (para mix final)

    void reset(uint32_t seed = 0) {
        h = seed;
        lenBytes = 0;
    }

    // Atualiza com pixels RGB565 (uint16_t). Processa 2 pixels -> 1x uint32_t.
    void update565(const uint16_t *px, size_t pixelCount) {
        lenBytes += (uint32_t)(pixelCount * sizeof(uint16_t));

        // Se alinhado em 4 bytes e número par, dá para ler como uint32_t direto
        if ((((uintptr_t)px & 3u) == 0u) && ((pixelCount & 1u) == 0u)) {
            const uint32_t *p32 = (const uint32_t *)px;
            const size_t n32 = pixelCount / 2;
            for (size_t i = 0; i < n32; ++i) {
                uint32_t k = p32[i]; // 2 pixels por load
                h ^= murmur_scramble32(k);
                h = rotl32(h, 13);
                h = h * 5u + 0xe6546b64u; // constante do Murmur3_32
            }
            return;
        }

        // Caminho seguro: empacota 2x uint16_t em uint32_t
        size_t i = 0;
        for (; i + 1 < pixelCount; i += 2) {
            uint32_t k = (uint32_t)px[i] | ((uint32_t)px[i + 1] << 16);
            h ^= murmur_scramble32(k);
            h = rotl32(h, 13);
            h = h * 5u + 0xe6546b64u;
        }
        // Pixel sobrando (tail de 16 bits)
        if (i < pixelCount) {
            uint32_t k = (uint32_t)px[i];
            h ^= murmur_scramble32(k);
        }
    }

    uint32_t finish() const {
        uint32_t out = h ^ lenBytes; // inclui len no mix final
        return fmix32(out);
    }
};

} // namespace

template <typename POS_T, class SCREEN> struct SoftRaster {
    texture_t<SCREEN> *pscreen;
    SCREEN *pLine;

    SoftRaster() = default;
    SoftRaster(texture_t<SCREEN> &usingScreen, POS_T /*pos*/)
        : pscreen(&usingScreen) {}

    template <typename POS, typename TEXTURE, typename COLOR>
    struct QuadCfgBlit {
        float startu{};
        float startv{};
        range_t<POS> ry;
        range_t<POS> rx;
        COLOR color;
        const texture_t<TEXTURE> *ptex{};
        bool alphaBlend{};

        void render(SCREEN *screen, const POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            const auto &tex = *ptex;
            const POS u = startu - rx.min;
            const POS v = startv - ry.min;
            const auto yV = y + v;
            if (alphaBlend) {
                // for (POS y = ry.min; y < ry.max; ++y)
                for (POS x = rx.min; x < rx.max; ++x) {
                    screen[x] %= color * tex.at(x + u, yV);
                }
            } else {
                // for (POS y = ry.min; y < ry.max; ++y)
                for (POS x = rx.min; x < rx.max; ++x) {
                    screen[x] = color * tex.at(x + u, yV);
                }
            }
        }
    };

    template <typename POS, typename TEXTURE, typename COLOR> struct QuadCfg {
        float startu{};
        float startv{};
        float duDx{};
        float dvDy{};
        range_t<POS> ry;
        range_t<POS> rx;
        COLOR color;
        const texture_t<TEXTURE> *ptex{};
        bool alphaBlend{};

        void render(SCREEN *screen, const POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            const auto &tex = *ptex;
            const float v = startv + dvDy * y;
            float u = startu;
            POS x = rx.min;
            if (alphaBlend) {
                while (x < rx.max) {
                    screen[x] %= color * tex.at(u, v);
                    ++x;
                    u += duDx;
                }
            } else {
                while (x < rx.max) {
                    screen[x] = color * tex.at(u, v);
                    ++x;
                    u += duDx;
                }
            }
        }
    };

    template <typename POS, typename COLOR> struct QuadCore {
        clip_t<POS> clip;
        range_t<POS> rx;
        range_t<POS> ry;
        COLOR color;

        void render(SCREEN *screen, const POS y) const {}

        void renderAlphaBlend(SCREEN *screen, POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            for (POS x = rx.min; x < rx.max; ++x) {
                screen[x] %= color;
            }
        }

        void renderNotBlend(SCREEN *screen, POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            for (POS x = rx.min; x < rx.max; ++x) {
                screen[x] = color;
            }
        }
    };

    template <typename POS, typename TEXTURE, typename COLOR> struct TriCore {
        const texture_t<TEXTURE> *ptex;
        bary_t<POS, COLOR> bary;
        clip_t<POS> clip;
        range_t<POS> rY;
        range_t<POS> ry;
        range_t<POS> rX1;
        range_t<POS> rX2;
        float yDist;

        void render(SCREEN *screen, const POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            const auto &tex = *ptex;
            const float f = (y - rY.min) / yDist;
            const range_t<POS> rx = inl_min(
                {lerp(rX1.min, rX2.min, f), lerp(rX1.max, rX2.max, f)}, clip.x);
            pixel_t<POS, COLOR> p;

            for (POS x = rx.min; x < rx.max; ++x) {
                p.x = x;
                p.y = y;
                barycentricUV(p, bary);

                POS u = (POS)((p.u * tex.w) + 0.5f) % tex.w;
                POS v = (POS)((p.v * tex.h) + 0.5f) % tex.h;

                screen[x] = bary.a.c * tex.at(u, v);
            }
        }

        void renderAlpha(SCREEN *screen, const POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            const auto &tex = *ptex;
            const float f = (y - rY.min) / yDist;
            const range_t<POS> rx = inl_min(
                {lerp(rX1.min, rX2.min, f), lerp(rX1.max, rX2.max, f)}, clip.x);
            pixel_t<POS, COLOR> p;
            for (POS x = rx.min; x < rx.max; ++x) {
                p.x = x;
                p.y = y;
                barycentricUV(p, bary);

                POS u = (POS)((p.u * tex.w) + 0.5f) % tex.w;
                POS v = (POS)((p.v * tex.h) + 0.5f) % tex.h;

                screen[x] %= bary.a.c * tex.at(u, v);
            }
        }
    };

    template <typename POS, typename COLOR> struct TriCoreColor {
        bary_t<POS, COLOR> bary;
        clip_t<POS> clip;
        range_t<POS> rY;
        range_t<POS> ry;
        range_t<POS> rX1;
        range_t<POS> rX2;
        float yDist;
        bool uvBlend, alphaBlend;

        void render(SCREEN *screen, const POS y) const {
            if (y < ry.min || y >= ry.max) {
                return;
            }

            const float f = (y - rY.min) / yDist;
            const range_t<POS> rx = inl_min(
                {lerp(rX1.min, rX2.min, f), lerp(rX1.max, rX2.max, f)}, clip.x);

            if (uvBlend) {
                pixel_t<POS, COLOR> p;
                if (alphaBlend) {
                    for (POS x = rx.min; x < rx.max; ++x) {
                        p.x = x;
                        p.y = y;
                        barycentricCol(p, bary);
                        screen[x] %= p.c;
                    }
                } else {
                    for (POS x = rx.min; x < rx.max; ++x) {
                        p.x = x;
                        p.y = y;
                        barycentricCol(p, bary);
                        screen[x] = p.c;
                    }
                }
            } else {
                if (alphaBlend) {
                    for (POS x = rx.min; x < rx.max; ++x) {
                        screen[x] %= bary.a.c;
                    }
                } else {
                    for (POS x = rx.min; x < rx.max; ++x) {
                        screen[x] = bary.a.c;
                    }
                }
            }
        }
    };

    union rendererMem_t {
        TriCoreColor<uint32_t, color32_t> triColor;
        TriCore<int32_t, color32_t, color32_t> triText;
        QuadCore<int32_t, color32_t> quadColor;
        QuadCfgBlit<int32_t, color32_t, color32_t> quadTextb;
        QuadCfg<int32_t, color32_t, color32_t> quadText;
    };

    template <typename POS> struct raiiRender {
        void (*render)(void *, SCREEN *screen, POS y){};
        void (*cleanup)(void *){};
        void (*moveContent)(void *to, void *from){};
        uint8_t memory[sizeof(rendererMem_t)];

        void renderThis(SCREEN *screen, POS y) { render(memory, screen, y); }

        raiiRender() = default;

        raiiRender &operator=(raiiRender &&other) noexcept {
            if (this == std::addressof(other)) {
                return *this;
            }

            if (other.moveContent) {
                other.moveContent(memory, other.memory);
            } else {
                memmove(memory, other.memory, sizeof(memory));
            }

            cleanup = other.cleanup;
            render = other.render;

            other.moveContent = nullptr;
            other.cleanup = nullptr;
            other.render = nullptr;

            return *this;
        }

        raiiRender(raiiRender &&other) noexcept { *this = std::move(other); }

        raiiRender(const raiiRender &) = delete;
        raiiRender &operator=(const raiiRender &) = delete;

        void reset() {
            if (cleanup != nullptr) {
                cleanup(memory);
                moveContent = nullptr;
                cleanup = nullptr;
            }
        }

        template <class T>
        T &create(void (*crender)(void *, SCREEN *screen, POS y) = nullptr) {
            static_assert(sizeof(T) <= sizeof(memory), "doesn't have memory");
            reset();

            auto *concretePtr = reinterpret_cast<T *>(memory);

            new (concretePtr) T();

            cleanup = [](void *objPtr) {
                auto &realPtr = *reinterpret_cast<T *>(objPtr);
                realPtr.~T();
            };

            if (crender) {
                render = crender;
            } else {
                render = [](void *objPtr, SCREEN *screen, POS y) {
                    auto *realPtr = reinterpret_cast<T *>(objPtr);

                    realPtr->render(screen, y);
                };
            }

            moveContent = [](void *to, void *from) {
                auto *realTo = reinterpret_cast<T *>(to);
                auto *realFrom = reinterpret_cast<T *>(from);

                *realTo = std::move(*realFrom);
            };

            return *concretePtr;
        }

        ~raiiRender() {
            if (cleanup != nullptr) {
                cleanup(memory);
                moveContent = nullptr;
                cleanup = nullptr;
            }
        }
    };

    std::vector<raiiRender<POS_T>> renderer;

    template <typename POS, typename TEXTURE, typename COLOR>
    void renderQuadCore(const texture_t<TEXTURE> &tex, const clip_t<POS> &clip,
                        const rectangle_t<POS, COLOR> &quad,
                        const bool alphaBlend) {
        if ((quad.p2.x < clip.x.min) || (quad.p2.y < clip.y.min) ||
            (quad.p1.x >= clip.x.max) || (quad.p1.y >= clip.y.max))
            return;

        const range_t<POS> qy = {quad.p1.y, quad.p2.y};

        const range_t<POS> ry = inl_min(qy, clip.y);

        const range_t<float> qv = {
            inl_max((float)quad.p1.v * tex.h, 0.0f),
            inl_min((float)quad.p2.v * tex.h, (float)tex.h)};

        const range_t<POS> qx = {quad.p1.x, quad.p2.x};
        const range_t<POS> rx = inl_min(qx, clip.x);

        const range_t<float> qu = {
            inl_max((float)quad.p1.u * tex.w, 0.0f),
            inl_min((float)quad.p2.u * tex.w, (float)tex.w)};

        const float duDx = (qu.max - qu.min) / (qx.max - qx.min);
        const float dvDy = (qv.max - qv.min) / (qy.max - qy.min);

        const float xoffset = (float)rx.min - (float)qx.min;
        const float yoffset = (float)ry.min - (float)qy.min;

        const float startu = qu.min + (xoffset > 0 ? duDx * xoffset : 0);
        const float startv = qv.min + (yoffset > 0 ? dvDy * yoffset : 0);

        bool blit = ((duDx == 1.0f) && (dvDy == 1.0f));

        renderer.emplace_back();
        raiiRender<POS> &raii = renderer.back();

        if (blit) {
            auto &quadObj =
                raii.template create<QuadCfgBlit<POS, TEXTURE, COLOR>>();

            quadObj.rx = rx;
            quadObj.ry = ry;
            quadObj.startu = startu;
            quadObj.startv = startv;
            quadObj.alphaBlend = alphaBlend;
            quadObj.ptex = &tex;
            quadObj.color = quad.p1.c;
        } else {
            auto &quadObj =
                raii.template create<QuadCfg<POS, TEXTURE, COLOR>>();

            quadObj.duDx = duDx;
            quadObj.dvDy = dvDy;
            quadObj.rx = rx;
            quadObj.ry = ry;
            quadObj.startu = startu;
            quadObj.startv = startv;
            quadObj.alphaBlend = alphaBlend;
            quadObj.ptex = &tex;
            quadObj.color = quad.p1.c;
        }
    }

    template <typename POS, typename COLOR>
    void renderQuadCore(const clip_t<POS> &clip,
                        const rectangle_t<POS, COLOR> &quad,
                        const bool alphaBlend) {
        if ((quad.p2.x < clip.x.min) || (quad.p2.y < clip.y.min) ||
            (quad.p1.x >= clip.x.max) || (quad.p1.y >= clip.y.max))
            return;
        const range_t<POS> ry = inl_min({quad.p1.y, quad.p2.y}, clip.y);

        const range_t<POS> rx = inl_min({quad.p1.x, quad.p2.x}, clip.x);

        renderer.emplace_back();
        raiiRender<POS> &raii = renderer.back();

        auto notBlend = [](void *objPtr, SCREEN *screen, POS y) {
            auto *realPtr = reinterpret_cast<QuadCore<POS, COLOR> *>(objPtr);

            realPtr->renderNotBlend(screen, y);
        };

        auto blend = [](void *objPtr, SCREEN *screen, POS y) {
            auto *realPtr = reinterpret_cast<QuadCore<POS, COLOR> *>(objPtr);

            realPtr->renderAlphaBlend(screen, y);
        };

        auto &quadData = raii.template create<QuadCore<POS, COLOR>>(
            alphaBlend ? blend : notBlend);

        quadData.clip = clip;
        quadData.rx = rx;
        quadData.ry = inl_min({quad.p1.y, quad.p2.y}, clip.y);
        quadData.color = quad.p1.c;
    }

    template <typename POS, typename COLOR>
    void renderQuad(SCREEN *screen, const texture_base_t *tex,
                    const clip_t<POS> &clip,
                    const rectangle_t<POS, COLOR> &quad,
                    const bool alphaBlend) {
        switch (tex == nullptr ? texture_type_t::NONE : tex->type) {
        case texture_type_t::ALPHA8:
            renderQuadCore(*reinterpret_cast<const texture_alpha8_t *>(tex),
                           clip, quad, alphaBlend);
            break;

        case texture_type_t::VALUE8:
            renderQuadCore(*reinterpret_cast<const texture_value8_t *>(tex),
                           clip, quad, alphaBlend);
            break;

        case texture_type_t::COLOR16:
            renderQuadCore(*reinterpret_cast<const texture_color16_t *>(tex),
                           clip, quad, alphaBlend);
            break;

        case texture_type_t::COLOR24:
            renderQuadCore(*reinterpret_cast<const texture_color24_t *>(tex),
                           clip, quad, alphaBlend);
            break;

        case texture_type_t::COLOR32:
            renderQuadCore(*reinterpret_cast<const texture_color32_t *>(tex),
                           clip, quad, alphaBlend);
            break;

        default:
            renderQuadCore(clip, quad, alphaBlend);
            break;
        }
    }

    template <typename POS, typename TEXTURE, typename COLOR>
    void renderTriCore(const texture_t<TEXTURE> &tex, const clip_t<POS> &clip,
                       const range_t<POS> &rY, const range_t<POS> &rX1,
                       const range_t<POS> &rX2, const bary_t<POS, COLOR> &bary,
                       const bool alphaBlend) {
        renderer.emplace_back();
        raiiRender<POS> &raii = renderer.back();

        using triangle_t = TriCore<POS, TEXTURE, COLOR>;

        auto alphaRender = [](void *objPtr, SCREEN *screen, POS y) {
            auto *realPtr = reinterpret_cast<triangle_t *>(objPtr);

            realPtr->renderAlpha(screen, y);
        };

        auto Render = [](void *objPtr, SCREEN *screen, POS y) {
            auto *realPtr = reinterpret_cast<triangle_t *>(objPtr);

            realPtr->render(screen, y);
        };

        auto &core =
            raii.template create<triangle_t>(alphaBlend ? alphaRender : Render);

        core.ptex = &tex;
        core.rY = rY;
        core.rX1 = rX1;
        core.rX2 = rX2;
        core.bary = bary;
        core.clip = clip;
        core.ry = inl_min(rY, clip.y);
        core.yDist = (float)(rY.max - rY.min);
    }

    template <typename POS, typename COLOR>
    void renderTriCore(const clip_t<POS> &clip, const range_t<POS> &rY,
                       const range_t<POS> &rX1, const range_t<POS> &rX2,
                       const bary_t<POS, COLOR> &bary, const bool uvBlend,
                       const bool alphaBlend) {
        renderer.emplace_back();
        raiiRender<POS> &raii = renderer.back();
        auto &core = raii.template create<TriCoreColor<POS, COLOR>>();

        core.alphaBlend = alphaBlend;
        core.uvBlend = uvBlend;
        core.bary = bary;
        core.clip = clip;
        core.rX1 = rX1;
        core.rX2 = rX2;
        core.rY = rY;
        core.ry = inl_min(rY, clip.y);
        core.yDist = (float)(rY.max - rY.min);
    }

    template <typename POS, typename COLOR>
    void renderTri(SCREEN *screen, const texture_base_t *tex,
                   const clip_t<POS> &clip, const range_t<POS> &rY,
                   const range_t<POS> &rX1, const range_t<POS> &rX2,
                   const bary_t<POS, COLOR> &bary, const bool uvBlend,
                   const bool alphaBlend) {
        switch (tex == nullptr ? texture_type_t::NONE : tex->type) {
        case texture_type_t::ALPHA8:
            renderTriCore(*reinterpret_cast<const texture_alpha8_t *>(tex),
                          clip, rY, rX1, rX2, bary, alphaBlend);
            break;

        case texture_type_t::VALUE8:
            renderTriCore(*reinterpret_cast<const texture_value8_t *>(tex),
                          clip, rY, rX1, rX2, bary, alphaBlend);
            break;

        case texture_type_t::COLOR16:
            renderTriCore(*reinterpret_cast<const texture_color16_t *>(tex),
                          clip, rY, rX1, rX2, bary, alphaBlend);
            break;

        case texture_type_t::COLOR24:
            renderTriCore(*reinterpret_cast<const texture_color24_t *>(tex),
                          clip, rY, rX1, rX2, bary, alphaBlend);
            break;

        case texture_type_t::COLOR32:
            renderTriCore(*reinterpret_cast<const texture_color32_t *>(tex),
                          clip, rY, rX1, rX2, bary, alphaBlend);
            break;

        default:
            renderTriCore(clip, rY, rX1, rX2, bary, uvBlend, alphaBlend);
            break;
        }
    }

    template <typename POS, typename COLOR>
    void renderTri(SCREEN *screen, const texture_base_t *tex,
                   const clip_t<POS> &clip, triangle_t<POS, COLOR> &tri,
                   const bool uvBlend, const bool alphaBlend) {
        if (tri.p1.y > tri.p2.y)
            swap(&(tri.p1), &(tri.p2));
        if (tri.p1.y > tri.p3.y)
            swap(&(tri.p1), &(tri.p3));
        if (tri.p2.y > tri.p3.y)
            swap(&(tri.p2), &(tri.p3));

        if ((tri.p3.x < clip.x.min) || (tri.p1.x >= clip.x.max))
            return;

        if (tri.p2.y == tri.p3.y) // Flat bottom triangle
        {
            if (tri.p1.y == tri.p2.y) // Flat line
            {
                if (tri.p1.x > tri.p2.x)
                    swap(&(tri.p1), &(tri.p2));
                if (tri.p1.x > tri.p3.x)
                    swap(&(tri.p1), &(tri.p3));
                if (tri.p2.x > tri.p3.x)
                    swap(&(tri.p2), &(tri.p3));

                if (tri.p1.y < clip.x.min || tri.p1.y > clip.x.max)
                    return;

                renderTri(screen, tex, clip, {tri.p1.y, tri.p1.y + 1},
                          {tri.p1.x, tri.p3.x}, {tri.p1.x, tri.p3.x},
                          baryPre(tri.p1, tri.p2, tri.p3), uvBlend, alphaBlend);
            } else // Flat bottom triangle
            {
                if (tri.p2.x > tri.p3.x)
                    swap(&(tri.p2), &(tri.p3));

                renderTri(screen, tex, clip, {tri.p1.y, tri.p3.y},
                          {tri.p1.x, tri.p1.x}, {tri.p2.x, tri.p3.x},
                          baryPre(tri.p1, tri.p2, tri.p3), uvBlend, alphaBlend);
            }
        } else if (tri.p1.y == tri.p2.y) // Flat top triangle
        {
            if (tri.p1.x > tri.p2.x)
                swap(&(tri.p1), &(tri.p2));

            renderTri(screen, tex, clip, {tri.p1.y, tri.p3.y},
                      {tri.p1.x, tri.p2.x}, {tri.p3.x, tri.p3.x},
                      baryPre(tri.p1, tri.p2, tri.p3), uvBlend, alphaBlend);
        } else {
            // Find 4th point to split the tri into flat top and flat bottom
            // triangles
            pixel_t<POS, COLOR> p4;
            p4.x = lerp(tri.p1.x, tri.p3.x,
                        (tri.p2.y - tri.p1.y) / (float)(tri.p3.y - tri.p1.y));
            p4.y = tri.p2.y;
            p4.c = tri.p1.c;

            if (tri.p2.x > p4.x)
                swap(&(tri.p2), &p4);

            renderTri(screen, tex, clip, {tri.p1.y, tri.p2.y},
                      {tri.p1.x, tri.p1.x}, {tri.p3.x, p4.x},
                      baryPre(tri.p1, tri.p2, p4), uvBlend, alphaBlend);

            renderTri(screen, tex, clip, {tri.p2.y, tri.p3.y}, {tri.p2.x, p4.x},
                      {tri.p3.x, tri.p3.x}, baryPre(tri.p2, p4, tri.p3),
                      uvBlend, alphaBlend);
        }
    }

    template <typename POS>
    void renderCommand(SCREEN *Line, const texture_base_t *texture,
                       const ImDrawVert *vtx_buffer,
                       const ImDrawIdx *idx_buffer, const ImDrawCmd &pcmd) {
        auto &screen = *pscreen;

        const clip_t<POS> clip = {
            {inl_max((POS)pcmd.ClipRect.x, (POS)0),
             inl_max((POS)pcmd.ClipRect.z, (POS)screen.w)},
            {inl_min((POS)pcmd.ClipRect.y, (POS)0),
             inl_min((POS)pcmd.ClipRect.w, (POS)screen.h)}};

        for (unsigned int i = 0; i < pcmd.ElemCount; i += 3) {
            const ImDrawVert *verts[] = {&vtx_buffer[idx_buffer[i]],
                                         &vtx_buffer[idx_buffer[i + 1]],
                                         &vtx_buffer[idx_buffer[i + 2]]};

            if (i < pcmd.ElemCount - 3) {
                ImVec2 tlpos = verts[0]->pos;
                ImVec2 brpos = verts[0]->pos;
                ImVec2 tluv = verts[0]->uv;
                ImVec2 bruv = verts[0]->uv;
                for (int v = 1; v < 3; v++) {
                    if (verts[v]->pos.x < tlpos.x) {
                        tlpos.x = verts[v]->pos.x;
                        tluv.x = verts[v]->uv.x;
                    } else if (verts[v]->pos.x > brpos.x) {
                        brpos.x = verts[v]->pos.x;
                        bruv.x = verts[v]->uv.x;
                    }
                    if (verts[v]->pos.y < tlpos.y) {
                        tlpos.y = verts[v]->pos.y;
                        tluv.y = verts[v]->uv.y;
                    } else if (verts[v]->pos.y > brpos.y) {
                        brpos.y = verts[v]->pos.y;
                        bruv.y = verts[v]->uv.y;
                    }
                }

                const ImDrawVert *nextVerts[] = {
                    &vtx_buffer[idx_buffer[i + 3]],
                    &vtx_buffer[idx_buffer[i + 4]],
                    &vtx_buffer[idx_buffer[i + 5]]};

                bool isRect = true;
                for (int v = 0; v < 3; v++) {
                    if (((nextVerts[v]->pos.x != tlpos.x) &&
                         (nextVerts[v]->pos.x != brpos.x)) ||
                        ((nextVerts[v]->pos.y != tlpos.y) &&
                         (nextVerts[v]->pos.y != brpos.y)) ||
                        ((nextVerts[v]->uv.x != tluv.x) &&
                         (nextVerts[v]->uv.x != bruv.x)) ||
                        ((nextVerts[v]->uv.y != tluv.y) &&
                         (nextVerts[v]->uv.y != bruv.y))) {
                        isRect = false;
                        break;
                    }
                }

                if (isRect) {
                    rectangle_t<POS, SCREEN> quad;
                    quad.p1.x = tlpos.x;
                    quad.p1.y = tlpos.y;
                    quad.p2.x = brpos.x;
                    quad.p2.y = brpos.y;
                    quad.p1.u = tluv.x;
                    quad.p1.v = tluv.y;
                    quad.p2.u = bruv.x;
                    quad.p2.v = bruv.y;
                    quad.p1.c = color32_t(verts[0]->col >> IM_COL32_R_SHIFT,
                                          verts[0]->col >> IM_COL32_G_SHIFT,
                                          verts[0]->col >> IM_COL32_B_SHIFT,
                                          verts[0]->col >> IM_COL32_A_SHIFT);
                    quad.p2.c = quad.p1.c;

                    const bool noUV =
                        (quad.p1.u == quad.p2.u) && (quad.p1.v == quad.p2.v);
                    const bool alphaBlend = true;

                    renderQuad(Line, noUV ? nullptr : texture, clip, quad,
                               alphaBlend);

                    i += 3;
                    continue;
                }
            }

            triangle_t<POS, SCREEN> tri;
            // triangle_t<POS, color32_t> tri;
            tri.p1.x = verts[0]->pos.x;
            tri.p1.y = verts[0]->pos.y;
            tri.p1.u = verts[0]->uv.x;
            tri.p1.v = verts[0]->uv.y;
            tri.p1.c = color32_t(verts[0]->col >> IM_COL32_R_SHIFT,
                                 verts[0]->col >> IM_COL32_G_SHIFT,
                                 verts[0]->col >> IM_COL32_B_SHIFT,
                                 verts[0]->col >> IM_COL32_A_SHIFT);

            tri.p2.x = verts[1]->pos.x;
            tri.p2.y = verts[1]->pos.y;
            tri.p2.u = verts[1]->uv.x;
            tri.p2.v = verts[1]->uv.y;
            tri.p2.c = color32_t(verts[1]->col >> IM_COL32_R_SHIFT,
                                 verts[1]->col >> IM_COL32_G_SHIFT,
                                 verts[1]->col >> IM_COL32_B_SHIFT,
                                 verts[1]->col >> IM_COL32_A_SHIFT);

            tri.p3.x = verts[2]->pos.x;
            tri.p3.y = verts[2]->pos.y;
            tri.p3.u = verts[2]->uv.x;
            tri.p3.v = verts[2]->uv.y;
            tri.p3.c = color32_t(verts[2]->col >> IM_COL32_R_SHIFT,
                                 verts[2]->col >> IM_COL32_G_SHIFT,
                                 verts[2]->col >> IM_COL32_B_SHIFT,
                                 verts[2]->col >> IM_COL32_A_SHIFT);

            const bool noUV = (tri.p1.u == tri.p2.u) &&
                              (tri.p1.u == tri.p3.u) &&
                              (tri.p1.v == tri.p2.v) && (tri.p1.v == tri.p3.v);
            const bool flatCol =
                noUV || ((tri.p1.c == tri.p2.c) && (tri.p1.c == tri.p3.c));
            const bool alphaBlend = true;

            renderTri(Line, noUV ? nullptr : texture, clip, tri, !flatCol,
                      alphaBlend);
        }
    }

    template <typename POS>
    void renderDrawLists(ImDrawData *drawData, SCREEN *get_buffer(size_t *size), bool useStripe,
                         uint32_t *stripesHashes = nullptr) {
        ImGuiIO &io = ImGui::GetIO();
        int fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        int fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        if (fbWidth == 0 || fbHeight == 0)
            return;
        drawData->ScaleClipRects(io.DisplayFramebufferScale);

        size_t lineElements = 0;
        auto *Line = get_buffer(&lineElements);
        pLine = Line;

        auto &screen = *pscreen;
        int stripeSize = useStripe ? lineElements / screen.w : 1;

        renderer.clear();

        {
            size_t objcnt = 0;
            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList *cmdList = drawData->CmdLists[n];
                objcnt += cmdList->CmdBuffer.Size;
            }

            renderer.reserve(objcnt);
        }

        {
            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList *cmdList = drawData->CmdLists[n];
                const ImDrawVert *vtx_buffer = cmdList->VtxBuffer.Data;
                const ImDrawIdx *idx_buffer = cmdList->IdxBuffer.Data;

                for (int cmdi = 0; cmdi < cmdList->CmdBuffer.Size; cmdi++) {
                    const ImDrawCmd &pcmd = cmdList->CmdBuffer[cmdi];
                    if (pcmd.UserCallback) {
                        pcmd.UserCallback(cmdList, &pcmd);
                    } else {
                        renderCommand<POS>(
                            Line,
                            reinterpret_cast<const texture_base_t *>(
                                pcmd.TextureId),
                            vtx_buffer, idx_buffer, pcmd);
                    }
                    idx_buffer += pcmd.ElemCount;
                }
            }
        }

        int currentLine = 0;
        int yStripe = 0;
        int currentStripe = 0;
        StripeHash32 stripeHash;

        for (POS y = 0; y < screen.h; y++) {
            if (currentLine == 0) {
                memset(Line, 0, lineElements * sizeof(Line[0]));
                stripeHash.reset();
            }

            auto *cLine = &Line[currentLine * screen.w];

            for (auto &obj : renderer) {
                obj.renderThis(cLine, y);
            }

            stripeHash.update565(reinterpret_cast<const uint16_t *>(cLine),
                                 screen.w);

            if (screen.lineWritedCb != nullptr) {
                if (currentLine == stripeSize - 1) {
                    currentLine = 0;
                    int tmpStripe = currentStripe++;

                    if (stripesHashes != nullptr) {
                        uint32_t hash = stripeHash.finish();
                        if (hash == stripesHashes[tmpStripe]) {
                            yStripe = y + 1;
                            continue; // skip unchanged stripe
                        }

                        stripesHashes[tmpStripe] = hash;
                    }

                    screen.lineWritedCb(screen, yStripe, Line, stripeSize);

                    Line = get_buffer(&lineElements);
                    pLine = Line;

                    yStripe = y + 1;
                } else {
                    currentLine++;
                }
            } else {
                auto *ptr = reinterpret_cast<SCREEN *>(screen.pixels);
                memcpy(&ptr[y * screen.w], Line, sizeof(Line[0]) * screen.w);
            }
        }

        if (currentLine != 0 && screen.lineWritedCb != nullptr) {
            uint32_t hash = stripeHash.finish();

            if (stripesHashes != nullptr) {
                int tmpStripe = currentStripe;

                if (hash == stripesHashes[tmpStripe]) {
                    return; // skip unchanged stripe
                }

                stripesHashes[tmpStripe] = hash;
            }
            screen.lineWritedCb(screen, yStripe, Line, currentLine);
        }
    }
};

#endif // SOFTRASTER_H
