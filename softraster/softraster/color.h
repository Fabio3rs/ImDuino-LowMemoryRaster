#pragma once
#ifndef SOFTRASTER_COLOR_H
#define SOFTRASTER_COLOR_H

#include "defines.h"
#include <cstdint>
#include <cstddef>
#include <utility>

enum class underlying_storage : uint8_t {
    little_endian,
    big_endian
};

static inline constexpr underlying_storage CURRENT_STORAGE = underlying_storage::big_endian;

static inline uint16_t adjust_endian(uint16_t v) {
    if constexpr (CURRENT_STORAGE == underlying_storage::big_endian) {
        return (v >> 8) | (v << 8);
    }
    return v;
}

#define C16RMASK 0xF800
#define C16GMASK 0x07E0
#define C16BMASK 0x001F

#define C16R(C) ((C) & C16RMASK)
#define C16G(C) ((C) & C16GMASK)
#define C16B(C) ((C) & C16BMASK)

struct alpha8_t;
struct value8_t;
struct color16_t;
struct color16_alpha8_t;
struct color24_t;
struct color32_t;

// color add

inline alpha8_t         operator+(alpha8_t          lhs, const alpha8_t         &rhs);
inline value8_t         operator+(value8_t          lhs, const value8_t         &rhs);
inline color16_t        operator+(color16_t         lhs, const color16_t        &rhs);
inline color16_alpha8_t operator+(color16_alpha8_t  lhs, const color16_alpha8_t &rhs);
inline color24_t        operator+(color24_t         lhs, const color24_t        &rhs);
inline color32_t        operator+(color32_t         lhs, const color32_t        &rhs);

// color scalar multiply

inline alpha8_t         operator*(alpha8_t          lhs, const float rhs);
inline value8_t         operator*(value8_t          lhs, const float rhs);
inline color16_t        operator*(color16_t         lhs, const float rhs);
inline color16_alpha8_t operator*(color16_alpha8_t  lhs, const float rhs);
inline color24_t        operator*(color24_t         lhs, const float rhs);
inline color32_t        operator*(color32_t         lhs, const float rhs);
inline alpha8_t         operator*(const float lhs, alpha8_t          rhs);
inline value8_t         operator*(const float lhs, value8_t          rhs);
inline color16_t        operator*(const float lhs, color16_t         rhs);
inline color16_alpha8_t operator*(const float lhs, color16_alpha8_t  rhs);
inline color24_t        operator*(const float lhs, color24_t         rhs);
inline color32_t        operator*(const float lhs, color32_t         rhs);

// color multiply

inline alpha8_t         operator*(      alpha8_t   lhs, const alpha8_t  &rhs);
inline color32_t        operator*(const alpha8_t  &lhs, const value8_t  &rhs);
inline color16_alpha8_t operator*(const alpha8_t  &lhs, const color16_t &rhs);
inline color32_t        operator*(const alpha8_t  &lhs, const color24_t &rhs);
inline color32_t        operator*(const alpha8_t  &lhs,       color32_t  rhs);

inline color32_t        operator*(const value8_t  &lhs, const alpha8_t  &rhs);
inline value8_t         operator*(      value8_t   lhs, const value8_t  &rhs);
inline color16_t        operator*(const value8_t  &lhs,       color16_t  rhs);
inline color24_t        operator*(const value8_t  &lhs,       color24_t  rhs);
inline color32_t        operator*(const value8_t  &lhs,       color32_t  rhs);

inline color16_alpha8_t operator*(const color16_t &lhs, const alpha8_t  &rhs);
inline color16_t        operator*(      color16_t  lhs, const value8_t  &rhs);
inline color16_t        operator*(      color16_t  lhs, const color16_t &rhs);
inline color24_t        operator*(const color16_t &lhs,       color24_t  rhs);
inline color32_t        operator*(const color16_t &lhs,       color32_t  rhs);

inline color32_t        operator*(const color24_t &lhs, const alpha8_t  &rhs);
inline color24_t        operator*(      color24_t  lhs, const value8_t  &rhs);
inline color24_t        operator*(      color24_t  lhs, const color16_t &rhs);
inline color24_t        operator*(      color24_t  lhs, const color24_t &rhs);
inline color32_t        operator*(const color24_t &lhs,       color32_t  rhs);

inline color32_t        operator*(      color32_t  lhs, const alpha8_t  &rhs);
inline color32_t        operator*(      color32_t  lhs, const value8_t  &rhs);
inline color32_t        operator*(      color32_t  lhs, const color16_t &rhs);
inline color32_t        operator*(      color32_t  lhs, const color24_t &rhs);
inline color32_t        operator*(      color32_t  lhs, const color32_t &rhs);

inline color16_alpha8_t operator*(color16_alpha8_t lhs, const color16_alpha8_t &rhs);
inline color16_alpha8_t operator*(color16_alpha8_t lhs, const alpha8_t  &rhs);
inline color16_alpha8_t operator*(color16_alpha8_t lhs, const value8_t  &rhs);
inline color16_alpha8_t operator*(color16_alpha8_t lhs, const color16_t &rhs);
inline color32_t        operator*(const color16_alpha8_t &lhs, color24_t rhs);
inline color32_t        operator*(const color16_alpha8_t &lhs, color32_t rhs);
inline color16_alpha8_t operator*(const alpha8_t  &lhs, color16_alpha8_t rhs);
inline color16_alpha8_t operator*(const value8_t  &lhs, color16_alpha8_t rhs);
inline color16_alpha8_t operator*(const color16_t &lhs, color16_alpha8_t rhs);
inline color32_t        operator*(      color24_t  lhs, const color16_alpha8_t &rhs);
inline color32_t        operator*(      color32_t  lhs, const color16_alpha8_t &rhs);

// alpha blend

//
// rhsA == 1 -> outRGBA = rhsRGBA
//
// lhsA == 1 -> outRGB = lhsRGB (1 - rhsA) + rhsRGB rhsA
//
// outA = lhsA (1 - rhsA) + rhsA
// outA == 0 -> outRGB = 0
// outA != 0 -> outRGB = (lhsRGB lhsA (1 - rhsA) + rhsRGB rhsA) / outA
//

//
// a + ((f * (b - a)) / 0xFF)
// a + (((f * b) - (f * a)) / 0xFF)
// a - ((f * a) / 0xFF) + ((f * b) / 0xFF)
// ((a * (0xFF - f)) / 0xFF) + ((f * b) / 0xFF)
// ((b * f) / 0xFF) + ((a * (0xFF - f)) / 0xFF)
//

inline alpha8_t         operator%(const alpha8_t  &lhs,       alpha8_t   rhs);
inline value8_t         operator%(const alpha8_t  &lhs,       value8_t   rhs);
inline color16_t        operator%(const alpha8_t  &lhs,       color16_t  rhs);
inline color24_t        operator%(const alpha8_t  &lhs,       color24_t  rhs);
inline color32_t        operator%(const alpha8_t  &lhs,       color32_t  rhs);

inline value8_t         operator%(      value8_t   lhs, const alpha8_t  &rhs);
inline value8_t         operator%(const value8_t  &lhs,       value8_t   rhs);
inline color16_t        operator%(const value8_t  &lhs,       color16_t  rhs);
inline color24_t        operator%(const value8_t  &lhs,       color24_t  rhs);
inline color32_t        operator%(const value8_t  &lhs,       color32_t  rhs);

inline color16_t        operator%(      color16_t  lhs, const alpha8_t  &rhs);
inline value8_t         operator%(const color16_t &lhs,       value8_t   rhs);
inline color16_t        operator%(const color16_t &lhs,       color16_t  rhs);
inline color24_t        operator%(const color16_t &lhs,       color24_t  rhs);
inline color32_t        operator%(const color16_t &lhs,       color32_t  rhs);

inline color24_t        operator%(      color24_t  lhs, const alpha8_t  &rhs);
inline value8_t         operator%(const color24_t &lhs,       value8_t   rhs);
inline color16_t        operator%(const color24_t &lhs,       color16_t  rhs);
inline color24_t        operator%(const color24_t &lhs,       color24_t  rhs);
inline color32_t        operator%(const color24_t &lhs,       color32_t  rhs);

inline color32_t        operator%(      color32_t  lhs, const alpha8_t  &rhs);
inline value8_t         operator%(const color32_t &lhs,       value8_t   rhs);
inline color16_t        operator%(const color32_t &lhs,       color16_t  rhs);
inline color24_t        operator%(const color32_t &lhs,       color24_t  rhs);
inline color32_t        operator%(      color32_t  lhs, const color32_t &rhs);

inline color16_alpha8_t operator%(color16_alpha8_t lhs, const color16_alpha8_t &rhs);
inline color16_alpha8_t operator%(color16_alpha8_t lhs, const alpha8_t  &rhs);
inline value8_t         operator%(const color16_alpha8_t &lhs, value8_t  rhs);
inline color16_t        operator%(const color16_alpha8_t &lhs, color16_t rhs);
inline color24_t        operator%(const color16_alpha8_t &lhs, color24_t rhs);
inline color32_t        operator%(const color16_alpha8_t &lhs, color32_t rhs);
inline color16_alpha8_t operator%(const alpha8_t  &lhs, color16_alpha8_t rhs);
inline color16_t        operator%(const value8_t  &lhs, color16_alpha8_t rhs);
inline color16_t        operator%(      color16_t  lhs, const color16_alpha8_t &rhs);
inline color24_t        operator%(      color24_t  lhs, const color16_alpha8_t &rhs);
inline color32_t        operator%(      color32_t  lhs, const color16_alpha8_t &rhs);

struct alpha8_t
{
    uint8_t a;

    alpha8_t() = default;
    alpha8_t(const uint8_t alpha) : a(alpha) {}
    alpha8_t(const signed int alpha) : a(alpha) {}
    alpha8_t(const unsigned int alpha) : a(alpha) {}
    alpha8_t(const alpha8_t &rhs) : a(rhs.a) {}

    template<typename COLOR>
    explicit inline alpha8_t(const COLOR &rhs) : a(rhs.A()) { }

    template<typename COLOR> inline alpha8_t &operator=(const COLOR &rhs) { a = rhs.A(); return *this; }
    template<typename T> inline alpha8_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline alpha8_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline alpha8_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const alpha8_t &rhs) const { return a == rhs.a; }
    inline bool operator!=(const alpha8_t &rhs) const { return a != rhs.a; }

    inline uint8_t R() const { return 0xFF; }
    inline uint8_t G() const { return 0xFF; }
    inline uint8_t B() const { return 0xFF; }
    inline uint8_t A() const { return a; }
    inline uint8_t V() const { return a; }
    inline uint16_t RGB16() const { return (((a * 0x1F) / 0xFF) << 0xB) | (((a * 0x3FU) / 0xFF) << 0x5) | ((a * 0x1F) / 0xFF); }
    inline uint32_t RGBA32() const { return 0xFFFFFF00 | a; }
};

struct value8_t
{
    uint8_t v;

    value8_t() = default;
    value8_t(const uint8_t value) : v(value) {}
    value8_t(const signed int value) : v(value) {}
    value8_t(const unsigned int value) : v(value) {}
    value8_t(const value8_t &rhs) : v(rhs.v) {}

    template<typename COLOR>
    explicit inline value8_t(const COLOR &rhs) : v(rhs.V()) { }

    template<typename COLOR> inline value8_t &operator=(const COLOR &rhs) { v = rhs.V(); return *this; }
    template<typename T> inline value8_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline value8_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline value8_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const value8_t &rhs) const { return v == rhs.v; }
    inline bool operator!=(const value8_t &rhs) const { return v != rhs.v; }

    inline uint8_t R() const { return v; }
    inline uint8_t G() const { return v; }
    inline uint8_t B() const { return v; }
    inline uint8_t A() const { return 0xFF; }
    inline uint8_t V() const { return v; }
    inline uint16_t RGB16() const { return (((v * 0x1F) / 0xFF) << 0xB) | (((v * 0x3F) / 0xFF) << 0x5) | ((v * 0x1F) / 0xFF); }
    inline uint32_t RGBA32() const { return (v << 24) | (v << 16) | (v << 8) | v;}
};

void WRITE_COLOR16FN(size_t ix, size_t iy, const color16_t *ref);
void WRITE_COLOR16FN(size_t ix, size_t iy, const color24_t *ref);
void WRITE_COLOR16FN(size_t ix, size_t iy, const color32_t *ref);

struct color16_t
{
    color16_t() = default;
    color16_t(const uint16_t val) : rgb(adjust_endian(val)) {}
    color16_t(const signed int val) : rgb(adjust_endian(val)) {}
    color16_t(const unsigned int val) : rgb(adjust_endian(val)) {}
    color16_t(const color16_t &rhs) : rgb(rhs.rgb) {}

    template<typename COLOR>
    explicit inline color16_t(const COLOR &rhs) : rgb(adjust_endian(rhs.RGB16())) { }

    template<typename COLOR> inline color16_t &operator=(const COLOR &rhs) { rgb = adjust_endian(rhs.RGB16());
   // WRITE_COLOR16FN(0, 0, this);
     return *this; }
    template<typename T> inline color16_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline color16_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline color16_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const color16_t &rhs) const { return rgb == rhs.rgb; }
    inline bool operator!=(const color16_t &rhs) const { return rgb != rhs.rgb; }

    inline uint8_t R() const { return ((C16R(value()) >> 0xB) * 0xFF) / 0x1F; }
    inline uint8_t G() const { return ((C16G(value()) >> 0x5) * 0xFF) / 0x3F; }
    inline uint8_t B() const { return  (C16B(value())         * 0xFF) / 0x1F; }
    inline uint8_t A() const { return 0xFF; }
    inline uint8_t V() const { return (R() + G() + B()) / 3; }
    inline uint16_t RGB16() const { return adjust_endian(rgb); }
    inline uint32_t RGBA32() const { return (R() << 24) | (G() << 16) | (B() << 8) | 0xFF; }

    inline uint16_t value() const { return adjust_endian(rgb); }
    inline void value(uint16_t val) { rgb = adjust_endian(val); }

protected:
    uint16_t rgb;
};


static_assert(sizeof(color16_t) == 2, "color16_t must be 2 bytes");


template<typename NCOLOR>
struct COLOR_WRAPPER : public NCOLOR {
    size_t x, y;
    NCOLOR ref;

    template<typename COLOR> inline NCOLOR &operator=(const COLOR &rhs) { NCOLOR::rgb = ref.rgb = rhs.RGB16();
        WRITE_COLOR16FN(x, y, reinterpret_cast<const NCOLOR*>(this));
     return *this; }
    template<typename T> inline NCOLOR &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline NCOLOR &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline NCOLOR &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const NCOLOR &rhs) const { return ref.rgb == rhs.rgb; }
    inline bool operator!=(const NCOLOR &rhs) const { return ref.rgb != rhs.rgb; }

    inline uint8_t R() const { return ((C16R(ref.rgb) >> 0xB) * 0xFF) / 0x1F; }
    inline uint8_t G() const { return ((C16G(ref.rgb) >> 0x5) * 0xFF) / 0x3F; }
    inline uint8_t B() const { return  (C16B(ref.rgb)         * 0xFF) / 0x1F; }
    inline uint8_t A() const { return 0xFF; }
    inline uint8_t V() const { return (R() + G() + B()) / 3; }
    inline uint16_t RGB16() const { return ref.rgb; }
    inline uint32_t RGBA32() const { return (R() << 24) | (G() << 16) | (B() << 8) | 0xFF; }

    inline COLOR_WRAPPER() = default;
    inline COLOR_WRAPPER(size_t ix, size_t iy, const NCOLOR &colordata) : NCOLOR(colordata), x(ix), y(iy), ref(colordata) {}
};


struct color16_alpha8_t
{
    color16_t color;
    alpha8_t alpha;

    color16_alpha8_t() = default;
    color16_alpha8_t(const uint16_t val, const uint8_t alpha) : color(val), alpha(alpha) {}
    color16_alpha8_t(const signed int val, const signed int alpha) : color(val), alpha(alpha) {}
    color16_alpha8_t(const unsigned int val, const unsigned int alpha) : color(val), alpha(alpha) {}
    color16_alpha8_t(const color16_t &val, const alpha8_t &alpha) : color(val), alpha(alpha) {}
    color16_alpha8_t(const color16_alpha8_t &rhs) : color(rhs.color), alpha(rhs.alpha) {}

    template<typename COLOR>
    explicit inline color16_alpha8_t(const COLOR &rhs) : color(rhs.RGB16()), alpha(rhs.A()) { }

    template<typename T> inline color16_alpha8_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline color16_alpha8_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline color16_alpha8_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const color16_alpha8_t &rhs) const { return color == rhs.color && alpha == rhs.alpha; }
    inline bool operator!=(const color16_alpha8_t &rhs) const { return color != rhs.color || alpha != rhs.alpha; }

    inline uint8_t R() const { return ((C16R(color.value()) >> 0xB) * 0xFF) / 0x1F; }
    inline uint8_t G() const { return ((C16G(color.value()) >> 0x5) * 0xFF) / 0x3F; }
    inline uint8_t B() const { return  (C16B(color.value())         * 0xFF) / 0x1F; }
    inline uint8_t A() const { return alpha.a; }
    inline uint8_t V() const { return (R() + G() + B()) / 3; }
    inline uint16_t RGB16() const { return color.value(); }
    inline uint32_t RGBA32() const { return (R() << 24) | (G() << 16) | (B() << 8) | alpha.a; }
};

struct color24_t
{
    uint8_t r, g, b;

    color24_t() = default;
    color24_t(const uint8_t valR, const uint8_t valG, const uint8_t valB)
        : r(valR), g(valG), b(valB) {}
    color24_t(const color24_t &rhs) : r(rhs.r), g(rhs.g), b(rhs.b) {}

    template<typename COLOR>
    explicit inline color24_t(const COLOR &rhs) : r(rhs.R()), g(rhs.G()), b(rhs.B()) { }

    template<typename COLOR>
    inline color24_t &operator=(const COLOR &rhs) { r = rhs.R(); g = rhs.G(); b = rhs.B(); return *this;}
    template<typename T> inline color24_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline color24_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline color24_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const color24_t &rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b; }
    inline bool operator!=(const color24_t &rhs) const { return r != rhs.r && g != rhs.g && b != rhs.b; }

    inline uint8_t R() const { return r; }
    inline uint8_t G() const { return g; }
    inline uint8_t B() const { return b; }
    inline uint8_t A() const { return 0xFF; }
    inline uint8_t V() const { return (R() + G() + B()) / 3; }
    inline uint16_t RGB16() const { return (((r * 0x1F) / 0xFF) << 0xB) | (((g * 0x3F) / 0xFF) << 0x5) | ((b * 0x1F) / 0xFF); }
    inline uint32_t RGBA32() const { return (r << 24) | (g << 16) | (b << 8) | 0xFF; }
};

struct color32_t
{
    uint8_t r, g, b, a;

    color32_t() = default;
    color32_t(const uint8_t valR, const uint8_t valG, const uint8_t valB, const uint8_t valA)
        : r(valR), g(valG), b(valB), a(valA) {}
    color32_t(const color32_t &rhs) : r(rhs.r), g(rhs.g), b(rhs.b), a(rhs.a) {}

    template<typename COLOR>
    explicit inline color32_t(const COLOR &rhs) : r(rhs.R()), g(rhs.G()), b(rhs.B()), a(rhs.A()) { }

    template<typename COLOR>
    inline color32_t &operator=(const COLOR &rhs) { r = rhs.R(); g = rhs.G(); b = rhs.B(); a = rhs.A(); return *this; }
    template<typename T> inline color32_t &operator+=(const T &rhs) { return *this = *this + rhs; }
    template<typename T> inline color32_t &operator*=(const T &rhs) { return *this = *this * rhs; }
    template<typename T> inline color32_t &operator%=(const T &rhs) { return *this = *this % rhs; }

    inline bool operator==(const color32_t &rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
    inline bool operator!=(const color32_t &rhs) const { return r != rhs.r && g != rhs.g && b != rhs.b && a != rhs.a; }

    inline uint8_t R() const { return r; }
    inline uint8_t G() const { return g; }
    inline uint8_t B() const { return b; }
    inline uint8_t A() const { return a; }
    inline uint8_t V() const { return (R() + G() + B()) / 3; }
    inline uint16_t RGB16() const { return (((r * 0x1FU) / 0xFFU) << 0x0BU) | (((g * 0x3FU) / 0xFFU) << 0x05U) | ((b * 0x1FU) / 0xFFU); }
    inline uint32_t RGBA32() const { return (r << 24U) | (g << 16U) | (b << 8U) | a; }
};

// color add

inline alpha8_t         operator+(alpha8_t          lhs, const alpha8_t         &rhs)
{
    lhs.a += rhs.a;
    return lhs;
}

inline value8_t         operator+(value8_t          lhs, const value8_t         &rhs)
{
    lhs.v += rhs.v;
    return lhs;
}

inline color16_t        operator+(color16_t         lhs, const color16_t        &rhs)
{
    lhs.value(C16R(C16R(lhs.value()) + C16R(rhs.value())) |
               C16G(C16G(lhs.value()) + C16G(rhs.value())) |
               C16B(C16B(lhs.value()) + C16B(rhs.value())));
    return lhs;
}

inline color16_alpha8_t operator+(color16_alpha8_t  lhs, const color16_alpha8_t &rhs)
{
    lhs.color.value(C16R(C16R(lhs.color.value()) + C16R(rhs.color.value())) |
                    C16G(C16G(lhs.color.value()) + C16G(rhs.color.value())) |
                    C16B(C16B(lhs.color.value()) + C16B(rhs.color.value())));
    lhs.alpha.a += rhs.alpha.a;
    return lhs;
}

inline color24_t        operator+(color24_t         lhs, const color24_t        &rhs)
{
    lhs.r += rhs.r;
    lhs.g += rhs.g;
    lhs.b += rhs.b;
    return lhs;
}

inline color32_t        operator+(color32_t         lhs, const color32_t        &rhs)
{
    lhs.r += rhs.r;
    lhs.g += rhs.g;
    lhs.b += rhs.b;
    lhs.a += rhs.a;
    return lhs;
}


// color scalar multiply

inline alpha8_t         operator*(alpha8_t          lhs, const float rhs)
{
    lhs.a *= rhs;
    return lhs;
}

inline value8_t         operator*(value8_t          lhs, const float rhs)
{
    lhs.v *= rhs;
    return lhs;
}

inline color16_t        operator*(color16_t         lhs, const float rhs)
{
    lhs.value(C16R(static_cast<uint8_t>(C16R(lhs.value()) * rhs)) |
               C16G(static_cast<uint8_t>(C16G(lhs.value()) * rhs)) |
               C16B(static_cast<uint8_t>(C16B(lhs.value()) * rhs)));
    return lhs;
}

inline color16_alpha8_t operator*(color16_alpha8_t  lhs, const float rhs)
{
    lhs.color.value(C16R(static_cast<uint8_t>(C16R(lhs.color.value()) * rhs)) |
                    C16G(static_cast<uint8_t>(C16G(lhs.color.value()) * rhs)) |
                    C16B(static_cast<uint8_t>(C16B(lhs.color.value()) * rhs)));
    lhs.alpha.a *= rhs;
    return lhs;
}

inline color24_t        operator*(color24_t         lhs, const float rhs)
{
    lhs.r *= rhs;
    lhs.g *= rhs;
    lhs.b *= rhs;
    return lhs;
}

inline color32_t        operator*(color32_t         lhs, const float rhs)
{
    lhs.r *= rhs;
    lhs.g *= rhs;
    lhs.b *= rhs;
    lhs.a *= rhs;
    return lhs;
}

inline alpha8_t         operator*(const float lhs, alpha8_t          rhs)
{
    rhs.a *= lhs;
    return rhs;
}

inline value8_t         operator*(const float lhs, value8_t          rhs)
{
    rhs.v *= lhs;
    return rhs;
}

inline color16_t        operator*(const float lhs, color16_t         rhs)
{
    rhs.value(C16R(static_cast<uint8_t>(C16R(rhs.value()) * lhs)) |
              C16G(static_cast<uint8_t>(C16G(rhs.value()) * lhs)) |
              C16B(static_cast<uint8_t>(C16B(rhs.value()) * lhs)));
    return rhs;
}

inline color16_alpha8_t operator*(const float lhs, color16_alpha8_t  rhs)
{
    rhs.color.value(C16R(static_cast<uint8_t>(C16R(rhs.color.value()) * lhs)) |
                    C16G(static_cast<uint8_t>(C16G(rhs.color.value()) * lhs)) |
                    C16B(static_cast<uint8_t>(C16B(rhs.color.value()) * lhs)));
    rhs.alpha.a *= lhs;
    return rhs;
}

inline color24_t        operator*(const float lhs, color24_t         rhs)
{
    rhs.r *= lhs;
    rhs.g *= lhs;
    rhs.b *= lhs;
    return rhs;
}

inline color32_t        operator*(const float lhs, color32_t         rhs)
{
    rhs.r *= lhs;
    rhs.g *= lhs;
    rhs.b *= lhs;
    rhs.a *= lhs;
    return rhs;
}

// color multiply

// alpha8_t

inline alpha8_t         operator*(      alpha8_t   lhs, const alpha8_t  &rhs)
{
    lhs.a = (lhs.a * rhs.a) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(const alpha8_t  &lhs, const value8_t  &rhs)
{
    return {rhs.v, rhs.v, rhs.v, lhs.a};
}

inline color16_alpha8_t operator*(const alpha8_t  &lhs, const color16_t &rhs)
{
    return {rhs, lhs};
}

inline color32_t        operator*(const alpha8_t  &lhs, const color24_t &rhs)
{
    return {rhs.r, rhs.g, rhs.b, lhs.a};
}

inline color32_t        operator*(const alpha8_t  &lhs,       color32_t  rhs)
{
    rhs.a = (rhs.a * lhs.a) / 0xFFU;
    return rhs;
}

// value8_t

inline color32_t        operator*(const value8_t  &lhs, const alpha8_t  &rhs)
{
    return {lhs.v, lhs.v, lhs.v, rhs.a};
}

inline value8_t         operator*(      value8_t   lhs, const value8_t  &rhs)
{
    lhs.v = (lhs.v * rhs.v) / 0xFFU;
    return lhs;
}

inline color16_t        operator*(const value8_t  &lhs,       color16_t  rhs)
{
    const uint16_t v = lhs.RGB16();
    rhs.value(C16R((C16R(rhs.value()) * (C16R(v) >> 0xB)) / 0x1FU) |
                C16G((C16G(rhs.value()) * (C16G(v) >> 0x5)) / 0x3FU) |
                C16B((C16B(rhs.value()) * (C16B(v) >> 0x0)) / 0x1FU));
    return rhs;
}

inline color24_t        operator*(const value8_t  &lhs,       color24_t  rhs)
{
    rhs.r = (rhs.r * lhs.v) / 0xFFU;
    rhs.g = (rhs.g * lhs.v) / 0xFFU;
    rhs.b = (rhs.b * lhs.v) / 0xFFU;
    return rhs;
}

inline color32_t        operator*(const value8_t  &lhs,       color32_t  rhs)
{
    rhs.r = (rhs.r * lhs.v) / 0xFFU;
    rhs.g = (rhs.g * lhs.v) / 0xFFU;
    rhs.b = (rhs.b * lhs.v) / 0xFFU;
    return rhs;
}

// color16_t

inline color16_alpha8_t operator*(const color16_t &lhs, const alpha8_t  &rhs)
{
    return {lhs, rhs};
}

inline color16_t        operator*(      color16_t  lhs, const value8_t  &rhs)
{
    const uint16_t v = rhs.RGB16();
    lhs.value(C16R((C16R(lhs.value()) * (C16R(v) >> 0xB)) / 0x1FU) |
                C16G((C16G(lhs.value()) * (C16G(v) >> 0x5)) / 0x3FU) |
                C16B((C16B(lhs.value()) * (C16B(v) >> 0x0)) / 0x1FU));
    return lhs;
}

inline color16_t        operator*(      color16_t  lhs, const color16_t &rhs)
{
    lhs.value(C16R((C16R(lhs.value()) * (C16R(rhs.value()) >> 0xB)) / 0x1FU) |
                C16G((C16G(lhs.value()) * (C16G(rhs.value()) >> 0x5)) / 0x3FU) |
                C16B((C16B(lhs.value()) * (C16B(rhs.value()) >> 0x0)) / 0x1FU));
    return lhs;
}

inline color24_t        operator*(const color16_t &lhs,       color24_t  rhs)
{
    rhs.r = (rhs.r * lhs.R()) / 0xFFU;
    rhs.g = (rhs.g * lhs.G()) / 0xFFU;
    rhs.b = (rhs.b * lhs.B()) / 0xFFU;
    return rhs;
}

inline color32_t        operator*(const color16_t &lhs,       color32_t  rhs)
{
    rhs.r = (rhs.r * lhs.R()) / 0xFFU;
    rhs.g = (rhs.g * lhs.G()) / 0xFFU;
    rhs.b = (rhs.b * lhs.B()) / 0xFFU;
    return rhs;
}

// color24_t

inline color32_t        operator*(const color24_t &lhs, const alpha8_t  &rhs)
{
    return {lhs.r, lhs.g, lhs.b, rhs.a};
}

inline color24_t        operator*(      color24_t  lhs, const value8_t  &rhs)
{
    lhs.r = (lhs.r * rhs.v) / 0xFFU;
    lhs.g = (lhs.g * rhs.v) / 0xFFU;
    lhs.b = (lhs.b * rhs.v) / 0xFFU;
    return lhs;
}

inline color24_t        operator*(      color24_t  lhs, const color16_t &rhs)
{
    lhs.r = (lhs.r * rhs.R()) / 0xFFU;
    lhs.g = (lhs.g * rhs.G()) / 0xFFU;
    lhs.b = (lhs.b * rhs.B()) / 0xFFU;
    return lhs;
}

inline color24_t        operator*(      color24_t  lhs, const color24_t &rhs)
{
    lhs.r = (lhs.r * rhs.r) / 0xFFU;
    lhs.g = (lhs.g * rhs.g) / 0xFFU;
    lhs.b = (lhs.b * rhs.b) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(const color24_t &lhs,       color32_t  rhs)
{
    rhs.r = (rhs.r * lhs.r) / 0xFFU;
    rhs.g = (rhs.g * lhs.g) / 0xFFU;
    rhs.b = (rhs.b * lhs.b) / 0xFFU;
    return rhs;
}

// color32_t

inline color32_t        operator*(      color32_t  lhs, const alpha8_t  &rhs)
{
    lhs.a = (lhs.a * rhs.a) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(      color32_t  lhs, const value8_t  &rhs)
{
    lhs.r = (lhs.r * rhs.v) / 0xFFU;
    lhs.g = (lhs.g * rhs.v) / 0xFFU;
    lhs.b = (lhs.b * rhs.v) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(      color32_t  lhs, const color16_t &rhs)
{
    lhs.r = (lhs.r * rhs.R()) / 0xFFU;
    lhs.g = (lhs.g * rhs.G()) / 0xFFU;
    lhs.b = (lhs.b * rhs.B()) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(      color32_t  lhs, const color24_t &rhs)
{
    lhs.r = (lhs.r * rhs.r) / 0xFFU;
    lhs.g = (lhs.g * rhs.g) / 0xFFU;
    lhs.b = (lhs.b * rhs.b) / 0xFFU;
    return lhs;
}

inline color32_t        operator*(      color32_t  lhs, const color32_t &rhs)
{
    lhs.r = (lhs.r * rhs.r) / 0xFFU;
    lhs.g = (lhs.g * rhs.g) / 0xFFU;
    lhs.b = (lhs.b * rhs.b) / 0xFFU;
    lhs.a = (lhs.a * rhs.a) / 0xFFU;
    return lhs;
}

// color16_alpha8_t

inline color16_alpha8_t operator*(color16_alpha8_t lhs, const color16_alpha8_t &rhs)
{
    lhs.color = lhs.color * rhs.color;
    lhs.alpha = lhs.alpha * rhs.alpha;
    return lhs;
}

inline color16_alpha8_t operator*(color16_alpha8_t lhs, const alpha8_t  &rhs)
{
    lhs.alpha = lhs.alpha * rhs;
    return lhs;
}

inline color16_alpha8_t operator*(color16_alpha8_t lhs, const value8_t  &rhs)
{
    lhs.color = lhs.color * rhs;
    return lhs;
}

inline color16_alpha8_t operator*(color16_alpha8_t lhs, const color16_t &rhs)
{
    lhs.color = lhs.color * rhs;
    return lhs;
}

inline color32_t        operator*(const color16_alpha8_t &lhs, color24_t rhs)
{
    rhs = lhs.color * rhs;
    return {rhs.r, rhs.g, rhs.b, lhs.alpha.a};
}

inline color32_t        operator*(const color16_alpha8_t &lhs, color32_t rhs)
{
    rhs = lhs.color * (lhs.alpha * rhs);
    return rhs;
}

inline color16_alpha8_t operator*(const alpha8_t  &lhs, color16_alpha8_t rhs)
{
    rhs.alpha = lhs * rhs.alpha;
    return rhs;
}

inline color16_alpha8_t operator*(const value8_t  &lhs, color16_alpha8_t rhs)
{
    rhs.color = lhs * rhs.color;
    return rhs;
}

inline color16_alpha8_t operator*(const color16_t &lhs, color16_alpha8_t rhs)
{
    rhs.color = lhs * rhs.color;
    return rhs;
}

inline color32_t        operator*(      color24_t  lhs, const color16_alpha8_t &rhs)
{
    lhs = lhs * rhs.color;
    return {lhs.r, lhs.g, lhs.b, rhs.alpha.a};
}

inline color32_t        operator*(      color32_t  lhs, const color16_alpha8_t &rhs)
{
    lhs = (lhs * rhs.color) * rhs.alpha;
    return lhs;
}

// alpha blend

//
// rhsA == 1 -> outRGBA = rhsRGBA
//
// lhsA == 1 -> outRGB = lhsRGB (1 - rhsA) + rhsRGB rhsA
//
// outA = lhsA (1 - rhsA) + rhsA
// outA == 0 -> outRGB = 0
// outA != 0 -> outRGB = (lhsRGB lhsA (1 - rhsA) + rhsRGB rhsA) / outA
//

//
// a + ((f * (b - a)) / 0xFF)
// a + (((f * b) - (f * a)) / 0xFF)
// a - ((f * a) / 0xFF) + ((f * b) / 0xFF)
// ((a * (0xFF - f)) / 0xFF) + ((f * b) / 0xFF)
// ((b * f) / 0xFF) + ((a * (0xFF - f)) / 0xFF)
//

// alpha8_t

inline alpha8_t         operator%(const alpha8_t  &lhs,       alpha8_t   rhs)
{
    if (lhs.a == 0xFFU || rhs.a == 0xFFU)
    {
        return alpha8_t{0xFFU};
    }
    else
    {
        rhs.a += (lhs.a * (0xFFU - rhs.a)) / 0xFFU;
    }
    return lhs;
}

inline value8_t         operator%(const alpha8_t  &lhs,       value8_t   rhs)
{
    return rhs;
}

inline color16_t        operator%(const alpha8_t  &lhs,       color16_t  rhs)
{
    return rhs;
}

inline color24_t        operator%(const alpha8_t  &lhs,       color24_t  rhs)
{
    return rhs;
}

inline color32_t        operator%(const alpha8_t  &lhs,       color32_t  rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (lhs.a == 0xFFU)
    {
        rhs.r = (0xFFU - rhs.a) + ((rhs.r * rhs.a) / 0xFF);
        rhs.g = (0xFFU - rhs.a) + ((rhs.g * rhs.a) / 0xFF);
        rhs.b = (0xFFU - rhs.a) + ((rhs.b * rhs.a) / 0xFF);
    }
    else if (rhs.a == 0x00U)
    {
        return {0xFFU, 0xFFU, 0xFFU, lhs.a};
    }
    else
    {
        const uint8_t outA = ((lhs.a * (0xFFU - rhs.a)) / 0xFFU) + rhs.a;
        if (outA)
        {
            const uint8_t srcRGB = lhs.a;
            rhs.r = ((srcRGB * 0xFF) + (rhs.a * (rhs.r - srcRGB))) / outA;
            rhs.g = ((srcRGB * 0xFF) + (rhs.a * (rhs.g - srcRGB))) / outA;
            rhs.b = ((srcRGB * 0xFF) + (rhs.a * (rhs.b - srcRGB))) / outA;
        }
        rhs.a = outA;
    }
    return rhs;
}

// value8_t

inline value8_t         operator%(      value8_t   lhs, const alpha8_t  &rhs)
{
    lhs.v += rhs.a - ((lhs.v * rhs.a) / 0xFFU);
    return lhs;
}

inline value8_t         operator%(const value8_t  &lhs,       value8_t   rhs)
{
    return rhs;
}

inline color16_t        operator%(const value8_t  &lhs,       color16_t  rhs)
{
    return rhs;
}

inline color24_t        operator%(const value8_t  &lhs,       color24_t  rhs)
{
    return rhs;
}

inline color32_t        operator%(const value8_t  &lhs,       color32_t  rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.a == 0x00U)
    {
        return {lhs.v, lhs.v, lhs.v, 0xFFU};
    }
    else
    {
        const uint8_t v = ((0xFFU - rhs.a) * lhs.v) / 0xFFU;
        rhs.r = v + ((rhs.r * rhs.a) / 0xFFU);
        rhs.g = v + ((rhs.g * rhs.a) / 0xFFU);
        rhs.b = v + ((rhs.b * rhs.a) / 0xFFU);
    }
    return rhs;
}

// color16_t

inline color16_t        operator%(      color16_t  lhs, const alpha8_t  &rhs)
{
    lhs.value(C16R(C16R(lhs.value()) + ((rhs.a * (C16RMASK - C16R(lhs.value()))) / 0xFFU)) |
                C16G(C16G(lhs.value()) + ((rhs.a * (C16GMASK - C16G(lhs.value()))) / 0xFFU)) |
                C16B(C16B(lhs.value()) + ((rhs.a * (C16BMASK - C16B(lhs.value()))) / 0xFFU)));
    return lhs;
}

inline value8_t         operator%(const color16_t &lhs,       value8_t   rhs)
{
    return rhs;
}

inline color16_t        operator%(const color16_t &lhs,       color16_t  rhs)
{
    return rhs;
}

inline color24_t        operator%(const color16_t &lhs,       color24_t  rhs)
{
    return rhs;
}

inline color32_t        operator%(const color16_t &lhs,       color32_t  rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.a == 0x00U)
    {
        return {lhs.R(), lhs.G(), lhs.B(), 0xFFU};
    }
    else
    {
        rhs.r = (((0xFFU - rhs.a) * lhs.R()) / 0xFFU) + ((rhs.r * rhs.a) / 0xFFU);
        rhs.g = (((0xFFU - rhs.a) * lhs.G()) / 0xFFU) + ((rhs.g * rhs.a) / 0xFFU);
        rhs.b = (((0xFFU - rhs.a) * lhs.B()) / 0xFFU) + ((rhs.b * rhs.a) / 0xFFU);
    }
    return rhs;
}

// color24_t

inline color24_t        operator%(      color24_t  lhs, const alpha8_t  &rhs)
{
    lhs.r += (rhs.a * (0xFFU - lhs.r)) / 0xFFU;
    lhs.g += (rhs.a * (0xFFU - lhs.g)) / 0xFFU;
    lhs.b += (rhs.a * (0xFFU - lhs.b)) / 0xFFU;
    return lhs;
}

inline value8_t         operator%(const color24_t &lhs,       value8_t   rhs)
{
    return rhs;
}

inline color16_t        operator%(const color24_t &lhs,       color16_t  rhs)
{
    return rhs;
}

inline color24_t        operator%(const color24_t &lhs,       color24_t  rhs)
{
    return rhs;
}

inline color32_t        operator%(const color24_t &lhs,       color32_t  rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.a == 0x00U)
    {
        return {lhs.r, lhs.g, lhs.b, 0xFFU};
    }
    else
    {
        rhs.r = (((0xFFU - rhs.a) * lhs.r) / 0xFFU) + ((rhs.r * rhs.a) / 0xFFU);
        rhs.g = (((0xFFU - rhs.a) * lhs.g) / 0xFFU) + ((rhs.g * rhs.a) / 0xFFU);
        rhs.b = (((0xFFU - rhs.a) * lhs.b) / 0xFFU) + ((rhs.b * rhs.a) / 0xFFU);
    }
    return rhs;
}


// color32_t

inline color32_t        operator%(      color32_t  lhs, const alpha8_t  &rhs)
{
    if (rhs.a == 0xFFU)
    {
        return {0xFFU, 0xFFU, 0xFFU, rhs.a};
    }
    else if (rhs.a == 0x00U)
    {
        return lhs;
    }
    else if (lhs.a == 0xFFU)
    {
        lhs.r += rhs.a - ((lhs.r * rhs.a) / 0xFFU);
        lhs.g += rhs.a - ((lhs.g * rhs.a) / 0xFFU);
        lhs.b += rhs.a - ((lhs.b * rhs.a) / 0xFFU);
    }
    else
    {
        const uint8_t outA = ((lhs.a * (0xFFU - rhs.a)) / 0xFFU) + rhs.a;
        if (outA)
        {
            lhs.r = (((((lhs.r * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (0xFFU * rhs.a)) / outA;
            lhs.g = (((((lhs.g * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (0xFFU * rhs.a)) / outA;
            lhs.b = (((((lhs.b * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (0xFFU * rhs.a)) / outA;
        }
        lhs.a = outA;
    }
    return lhs;
}

inline value8_t         operator%(const color32_t &lhs,       value8_t   rhs)
{
    return rhs;
}

inline color16_t        operator%(const color32_t &lhs,       color16_t  rhs)
{
    return rhs;
}

inline color24_t        operator%(const color32_t &lhs,       color24_t  rhs)
{
    return rhs;
}

inline color32_t        operator%(      color32_t  lhs, const color32_t &rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.a == 0x00U)
    {
        return lhs;
    }
    else if (lhs.a == 0xFFU)
    {
        lhs.r += ((rhs.a * (rhs.r - lhs.r)) / 0xFFU);
        lhs.g += ((rhs.a * (rhs.g - lhs.g)) / 0xFFU);
        lhs.b += ((rhs.a * (rhs.b - lhs.b)) / 0xFFU);
    }
    else
    {
        const uint8_t outA = ((lhs.a * (0xFFU - rhs.a)) / 0xFFU) + rhs.a;
        if (outA)
        {
            lhs.r = (((((lhs.r * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.r * rhs.a)) / outA;
            lhs.g = (((((lhs.g * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.g * rhs.a)) / outA;
            lhs.b = (((((lhs.b * lhs.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.b * rhs.a)) / outA;
        }
        lhs.a = outA;
    }
    return lhs;
}


// color16_alpha8_t

inline color16_alpha8_t operator%(color16_alpha8_t lhs, const color16_alpha8_t &rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return lhs;
    }
    else if (lhs.alpha.a == 0xFFU)
    {
        lhs.color.value(C16R(((C16R(lhs.color.value()) * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16G(((C16G(lhs.color.value()) * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16B(((C16B(lhs.color.value()) * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / 0xFFU));
        lhs.alpha = lhs.alpha % rhs.alpha;
    }
    else
    {
        const uint8_t outA = (lhs.alpha % rhs.alpha).a;
        if (outA)
        {
            lhs.color.value(C16R(((((C16R(lhs.color.value()) * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / outA) |
                            C16G(((((C16G(lhs.color.value()) * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / outA) |
                            C16B(((((C16B(lhs.color.value()) * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / outA));
        }
        lhs.alpha.a = outA;
    }
    return lhs;
}

inline color16_alpha8_t operator%(color16_alpha8_t lhs, const alpha8_t  &rhs)
{
    lhs.color = lhs.color % rhs;
    lhs.alpha = lhs.alpha % rhs;
    return lhs;
}

inline value8_t         operator%(const color16_alpha8_t &lhs, value8_t  rhs)
{
    return rhs;
}

inline color16_t        operator%(const color16_alpha8_t &lhs, color16_t rhs)
{
    return rhs;
}

inline color24_t        operator%(const color16_alpha8_t &lhs, color24_t rhs)
{
    return rhs;
}

inline color32_t        operator%(const color16_alpha8_t &lhs, color32_t rhs)
{
    if (rhs.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.a == 0x00U)
    {
        return {lhs.color.R(), lhs.color.G(), lhs.color.B(), lhs.alpha.a};
    }
    else if (lhs.alpha.a == 0xFFU)
    {
        rhs.r = ((lhs.color.R() * (0xFFU - rhs.a)) / 0xFFU) + ((rhs.r * rhs.a) / 0xFFU);
        rhs.g = ((lhs.color.G() * (0xFFU - rhs.a)) / 0xFFU) + ((rhs.g * rhs.a) / 0xFFU);
        rhs.b = ((lhs.color.B() * (0xFFU - rhs.a)) / 0xFFU) + ((rhs.b * rhs.a) / 0xFFU);
    }
    else
    {
        const uint8_t outA = ((lhs.alpha.a * (0xFFU - rhs.a)) / 0xFFU) + rhs.a;
        if (outA)
        {
            rhs.r = (((((lhs.color.R() * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.r * rhs.a)) / outA;
            rhs.g = (((((lhs.color.G() * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.g * rhs.a)) / outA;
            rhs.b = (((((lhs.color.B() * lhs.alpha.a) / 0xFFU) * (0xFFU - rhs.a))) + (rhs.b * rhs.a)) / outA;
        }
        rhs.a = outA;
    }
    return rhs;
}

inline color16_alpha8_t operator%(const alpha8_t  &lhs, color16_alpha8_t rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return rhs;
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return {0xFFFFU, lhs};
    }
    else if (lhs.a == 0xFFU)
    {
        rhs.color.value(C16R(((C16RMASK * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16G(((C16GMASK * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16B(((C16BMASK * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / 0xFFU));
        rhs.alpha = lhs % rhs.alpha;
    }
    else
    {
        const uint8_t outA = (lhs % rhs.alpha).a;
        if (outA)
        {
            rhs.color.value(C16R(((((C16RMASK * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / outA) |
                            C16G(((((C16GMASK * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / outA) |
                            C16B(((((C16BMASK * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / outA));
        }
        rhs.alpha.a = outA;
    }
    return rhs;
}

inline color16_t        operator%(const value8_t  &lhs, color16_alpha8_t rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return rhs.color;
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return {lhs.RGB16()};
    }
    else
    {
        rhs.color.value(C16R(((((C16RMASK * lhs.v) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16G(((((C16GMASK * lhs.v) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                        C16B(((((C16BMASK * lhs.v) / 0xFFU) * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / 0xFFU));
    }
    return rhs.color;
}

inline color16_t        operator%(      color16_t  lhs, const color16_alpha8_t &rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return rhs.color;
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return lhs;
    }
    else
    {
        lhs.value(C16R(((C16R(lhs.value()) * (0xFFU - rhs.alpha.a)) + (C16R(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                    C16G(((C16G(lhs.value()) * (0xFFU - rhs.alpha.a)) + (C16G(rhs.color.value()) * rhs.alpha.a)) / 0xFFU) |
                    C16B(((C16B(lhs.value()) * (0xFFU - rhs.alpha.a)) + (C16B(rhs.color.value()) * rhs.alpha.a)) / 0xFFU));
    }
    return lhs;
}

inline color24_t        operator%(      color24_t  lhs, const color16_alpha8_t &rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return {rhs.color.R(), rhs.color.G(), rhs.color.B()};
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return lhs;
    }
    else
    {
        lhs.r += ((rhs.alpha.a * (rhs.color.R() - lhs.r)) / 0xFFU);
        lhs.g += ((rhs.alpha.a * (rhs.color.G() - lhs.g)) / 0xFFU);
        lhs.b += ((rhs.alpha.a * (rhs.color.B() - lhs.b)) / 0xFFU);
    }
    return lhs;
}

inline color32_t        operator%(      color32_t  lhs, const color16_alpha8_t &rhs)
{
    if (rhs.alpha.a == 0xFFU)
    {
        return {rhs.color.R(), rhs.color.G(), rhs.color.B(), rhs.alpha.a};
    }
    else if (rhs.alpha.a == 0x00U)
    {
        return lhs;
    }
    else if (lhs.a == 0xFFU)
    {
        lhs.r += ((rhs.alpha.a * (rhs.color.R() - lhs.r)) / 0xFFU);
        lhs.g += ((rhs.alpha.a * (rhs.color.G() - lhs.g)) / 0xFFU);
        lhs.b += ((rhs.alpha.a * (rhs.color.B() - lhs.b)) / 0xFFU);
    }
    else
    {
        const uint8_t outA = ((lhs.a * (0xFFU - rhs.alpha.a)) / 0xFFU) + rhs.alpha.a;
        if (outA)
        {
            lhs.r = (((((lhs.r * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a))) + (rhs.color.R() * rhs.alpha.a)) / outA;
            lhs.g = (((((lhs.g * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a))) + (rhs.color.G() * rhs.alpha.a)) / outA;
            lhs.b = (((((lhs.b * lhs.a) / 0xFFU) * (0xFFU - rhs.alpha.a))) + (rhs.color.B() * rhs.alpha.a)) / outA;
        }
        lhs.a = outA;
    }
    return lhs;
}

#undef C16R
#undef C16G
#undef C16B

#undef C16RMASK
#undef C16GMASK
#undef C16BMASK

#endif // SOFTRASTER_COLOR_H
