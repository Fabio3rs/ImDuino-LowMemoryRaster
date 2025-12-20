#include "imgui.h"
#include "softraster/softraster/color.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
texture_alpha8_t fontAtlas;

#include "ILI9341_DMA.hpp"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
// #include <XPT2046_Touchscreen.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

struct TS_Point {
    uint16_t x;
    uint16_t y;
    uint16_t z;
};

inline void ILI9341_DMA::preCbThunk_(spi_transaction_t *t) {
    uintptr_t u = (uintptr_t)t->user;
    int dc_level = (int)(u & 1u);
    gpio_num_t dc_pin = (gpio_num_t)(u >> 1);
    gpio_set_level(dc_pin, dc_level);
}

namespace {
#define STMPE_CS 8

// ===== TFT ILI9341 (SPI) =====
#define TFT_CS 27
#define TFT_DC 26
#define TFT_RST 33 // ou -1 se não estiver ligado
#define TFT_BL 32  // opcional (PWM), ou direto em 3V3

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19

// Rotação (0..3)
#define TFT_ROTATION 3 // 1 ou 3 = landscape (320x240)

#define TFT_NATIVE_W 240
#define TFT_NATIVE_H 320

// Se você quer trabalhar “fixo” em landscape:
#define SCREEN_W 320
#define SCREEN_H 240

// ===== Touch XPT2046 (SPI compartilhado) =====
#define TOUCH_CS 25
#define TOUCH_IRQ 34 // opcional (se usar IRQ)

// (opcional) frequência SPI para o ILI9341 no ESP32
#define TFT_SPI_FREQ (40 * 1000 * 1000) // 40 MHz (padrão típico no ESP32)

//  Adafruit_ILI9341(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
//                   int8_t _RST = -1, int8_t _MISO = -1);
// Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
ILI9341_DMA lcd;
// XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

boolean RecordOn = false;

#define FRAME_X 320
#define FRAME_Y 240

#define SCREENX FRAME_X
#define SCREENY FRAME_Y

#define SCREEN_W 320
#define SCREEN_H 240

// Ajuste com seus valores medidos:
#define TS_MINX 250
#define TS_MAXX 3800
#define TS_MINY 250
#define TS_MAXY 3800

#define Z_MIN 200 // ajuste

static spi_device_handle_t touchDev = nullptr;

static inline uint16_t xpt2046_read12(uint8_t cmd) {
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0, 0, 0};

    spi_transaction_t t = {};
    t.length = 24;
    t.rxlength = 24; // explícito (boa prática no IDF)
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    ESP_ERROR_CHECK(spi_device_polling_transmit(touchDev, &t));

    uint16_t v = ((uint16_t)rx[1] << 8) | rx[2];
    return v >>
           3; // 12-bit alinhado (padrão) :contentReference[oaicite:2]{index=2}
}

static inline bool touch_raw(uint16_t &x, uint16_t &y, uint16_t &z) {
    if (!touchDev)
        return false;

    // PENIRQ: normalmente HIGH = não tocado, LOW = tocado
    if (TOUCH_IRQ >= 0 && gpio_get_level((gpio_num_t)TOUCH_IRQ) == 1) {
        return false;
    }

    uint16_t yraw = xpt2046_read12(0x90);
    uint16_t xraw = xpt2046_read12(0xD0);
    uint16_t z1 = xpt2046_read12(0xB0);
    uint16_t z2 = xpt2046_read12(0xC0);

    uint16_t zraw =
        z1 + 4095 - z2; // fórmula comum :contentReference[oaicite:3]{index=3}

    x = xraw;
    y = yraw;
    z = zraw;

    return zraw >= Z_MIN;
}

static inline int32_t map_clamped(int32_t v, int32_t in_min, int32_t in_max,
                                  int32_t out_min, int32_t out_max) {
    // Permite TS_MIN > TS_MAX (inverte automaticamente)
    if (in_min == in_max)
        return out_min;

    bool inv = false;
    if (in_min > in_max) {
        int32_t tmp = in_min;
        in_min = in_max;
        in_max = tmp;
        inv = true;
    }

    // map() do Arduino é inclusivo, então use out_max = (W-1)/(H-1).
    // :contentReference[oaicite:1]{index=1}
    int32_t m = map(v, in_min, in_max, out_min, out_max);

    // Se os TS_* vieram invertidos, espelha no intervalo de saída
    if (inv)
        m = out_max - (m - out_min);

    if (m < out_min)
        m = out_min;
    else if (m > out_max)
        m = out_max;
    return m;
}

static inline void apply_rotation(int32_t nx, int32_t ny, int16_t &sx,
                                  int16_t &sy, uint8_t rot) {
    rot &= 3;

    // nx,ny já estão no “espaço” SCREEN_W x SCREEN_H (paisagem 320x240 no seu
    // caso).
    switch (rot) {
    default:
    case 0:
        sx = (int16_t)nx;
        sy = (int16_t)ny;
        break;
    case 1:
        sx = (int16_t)ny;
        sy = (int16_t)(SCREEN_W - 1 - nx);
        break;
    case 2:
        sx = (int16_t)(SCREEN_W - 1 - nx);
        sy = (int16_t)(SCREEN_H - 1 - ny);
        break;
    case 3:
        sx = (int16_t)(SCREEN_H - 1 - ny);
        sy = (int16_t)nx;
        break;
    }
}

static inline bool readTouchScreen(int16_t &sx, int16_t &sy, int16_t &z) {
    uint16_t xr, yr, zr;

    // IMPORTANTÍSSIMO: só usa os valores se touch_raw() disser que é toque.
    // (Sem toque, resistivo pode saturar em 0/4095 e variar.)
    if (!touch_raw(xr, yr, zr))
        return false;

    z = (int16_t)zr;
    if (z < Z_MIN)
        return false;

    // 1) Normaliza para o espaço da tela (0..W-1, 0..H-1)
    int32_t nx = map_clamped((int32_t)xr, TS_MINX, TS_MAXX, 0, SCREEN_W - 1);
    int32_t ny = map_clamped((int32_t)yr, TS_MINY, TS_MAXY, 0, SCREEN_H - 1);

    // 2) Aplica rotação (0..3). Em geral isso substitui os “(W-1)-...”
    apply_rotation(nx, ny, sx, sy, TFT_ROTATION);

    return true;
}

void feedTouchToImGui(float dt) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)SCREEN_W, (float)SCREEN_H);
    io.DeltaTime = dt;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    int16_t x, y, z;
    bool down = readTouchScreen(x, y, z);

    if (down) {
        io.MousePos = ImVec2((float)x, (float)y);
        io.MouseDown[0] = true;
    } else {
        io.MouseDown[0] = false;
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    }
}

texture_color16_t screen;
ImplSoftRaster<color16_t> implRaster(screen);

unsigned long rasterTime = 0;
unsigned long lastRasterTime = 0;

/*
 * @brief Draws one line
 */
void drawLineCallback(texture_color16_t &screen, int y, const color16_t *Line,
                      int stripeSize) {
    rasterTime += micros() - lastRasterTime;
    lcd.waitAll();
    lcd.setAddrWindow(0, y, screen.w, stripeSize);
    lcd.pushPixelsAsync(
        (const uint16_t *)Line, (size_t)screen.w * (size_t)stripeSize,
        /*swap*/ CURRENT_STORAGE == underlying_storage::little_endian);
    lastRasterTime = micros();
}

template <class Arr, size_t N> size_t arrayElements(const Arr (&)[N]) {
    return N;
}

size_t stripeLines = 16;
size_t lineElements = SCREEN_W * stripeLines;
//color16_t *Line = new color16_t[lineElements];
uint32_t *stripesHashes = new uint32_t[SCREEN_W / stripeLines + 1];

static void touch_idf_init(spi_host_device_t host) {
    spi_device_interface_config_t devcfg = {};
    devcfg.mode = 0;
    devcfg.spics_io_num = TOUCH_CS;
    devcfg.queue_size = 1;

    // Touch NÃO precisa alta frequência. Use 1–2 MHz para estabilidade.
    devcfg.clock_speed_hz = 2 * 1000 * 1000;

    // Não use pre_cb aqui (isso era específico do DC do TFT).
    devcfg.pre_cb = nullptr;

    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &touchDev));

    // IRQ opcional (LOW quando tocado, na maioria dos módulos)
    if (TOUCH_IRQ >= 0) {
        gpio_set_direction((gpio_num_t)TOUCH_IRQ, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)TOUCH_IRQ, GPIO_PULLUP_ONLY);
    }
}

void screen_init() {
    // digitalWrite(22, LOW);
    // delay(500);
    // digitalWrite(22, HIGH);
    Serial.begin(115200);
    Serial.println("ILI9341 TFT + XPT2046 Touchscreen Test");

    // tft.begin(TFT_SPI_FREQ); // You can pass in a specific SPI frequency

    // tft.setAddrWindow(0, 0, SCREEN_W, SCREEN_H);
    // tft.setFont(Terminal6x8);

    ILI9341_DMA::Pins p;
    p.mosi = 23;
    p.miso = 19;
    p.sclk = 18;
    p.cs = TFT_CS;
    p.dc = TFT_DC;
    p.rst = TFT_RST; // ajuste conforme seu wiring

    lcd.setMaxTransferBytes(SCREEN_W * stripeLines * 2);
    // dmaBufWords_ no header está como 320*16; ajuste se seu W/stripe variam.
    lcd.begin(p, SCREEN_W, SCREEN_H, SPI2_HOST, TFT_SPI_FREQ,
              /*queueDepth*/ 2, /*NO_DUMMY*/ true);
    lcd.setRotation(TFT_ROTATION);

    touch_idf_init(SPI2_HOST);

    //     tft.fillScreen(ILI9341_BLUE);

    /*
     * @brief callback for draw one line
     */
    screen.lineWritedCb = drawLineCallback;

    screen.init(SCREENX, SCREENY, nullptr); // Sets the raster buffer as nullptr
}

color16_t *get_available_buffer(size_t *size = nullptr) {
    if (size) {
        *size = lineElements;
    }
    return reinterpret_cast<color16_t *>(lcd.getCurrentDMABuffer());
}

void screen_draw() {
    rasterTime = 0;
    lcd.beginFrame(true);
    lcd.waitAll();
    implRaster.ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData(),
                                                   get_available_buffer, true);
    lcd.waitAll();
    lcd.endFrame();
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

    ImGui::GetStyle().TouchExtraPadding = ImVec2(4.0f, 4.0f);

    // Para debug, vamos printar heap e outras infos do ESP32 no Serial
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    Serial.printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
                  chip_info.cores,
                  (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                  (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    Serial.printf("silicon revision %d, ", chip_info.revision);
    Serial.printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
                  (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded"
                                                                : "external");
    Serial.printf("Heap size (free): %d bytes\n", esp_get_free_heap_size());
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

    // bool istouched = ts.touched();

    implRaster.ImGui_ImplSoftraster_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowPos(ImVec2(0.0, 0.0));
    ImGui::SetWindowSize(ImVec2(SCREENX, SCREENY));

    unsigned int deltaTime = millis() - t;
    t += deltaTime;

    unsigned long rasterTimeMs = rasterTime / 1000;

    deltaTime -= (drawTime + renderTime + rasterTimeMs);

    ImGui::Text("Hardware write time %d ms", drawTime);
    ImGui::Text("Render time %d ms", renderTime);
    ImGui::Text("Raster time %d ms", rasterTimeMs);
    ImGui::Text("Remaining time %d ms", deltaTime);

    feedTouchToImGui(1.0f / 60.0f);

    if (true) {
        TS_Point p{};
        touch_raw(p.x, p.y, p.z);

        ImGui::Text("Touch: %d,%d,%d", p.x, p.y, p.z);
    }
    ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);

    ImGui::Render();
    renderTime = millis() - startRenderTime;

    //  tft.startWrite();
    drawTime = millis();
    screen_draw();
    drawTime = millis() - drawTime;
}
