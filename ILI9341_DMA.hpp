#pragma once

#include <Arduino.h>

extern "C" {
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
}

class ILI9341_DMA {
  public:
    struct Pins {
        int mosi = 23;
        int miso = 19;
        int sclk = 18;
        int cs = 5;
        int dc = 21;
        int rst = -1; // opcional
    };

    // host: SPI2_HOST (HSPI) ou SPI3_HOST (VSPI) no ESP32 clássico.
    // clock_hz: ex. 40 MHz / 80 MHz (ver nota de NO_DUMMY).
    // queueDepth: 2..8 (4 costuma ser bom). Cada slot tem 1 transação + 1
    // buffer DMA.
    bool begin(const Pins &p, int width, int height,
               spi_host_device_t host = SPI2_HOST,
               int clock_hz = 40 * 1000 * 1000, int queueDepth = 4,
               bool outputOnlyNoDummy = true) {
        pins_ = p;
        w_ = width;
        h_ = height;
        host_ = host;

        if (queueDepth < 2)
            queueDepth = 2;
        if (queueDepth > kMaxQ)
            queueDepth = kMaxQ;
        qDepth_ = queueDepth;

        gpio_set_direction((gpio_num_t)pins_.dc, GPIO_MODE_OUTPUT);
        if (pins_.rst >= 0) {
            gpio_set_direction((gpio_num_t)pins_.rst, GPIO_MODE_OUTPUT);
            hardReset_();
        }

        spi_bus_config_t buscfg{};
        buscfg.mosi_io_num = pins_.mosi;
        buscfg.miso_io_num = pins_.miso;
        buscfg.sclk_io_num = pins_.sclk;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;

        // stripe/push tamanho máximo configurável depois via
        // setMaxTransferBytes().
        buscfg.max_transfer_sz = maxTransferBytes_;

        // DMA habilitado: buffers devem ser DMA-capable.
        // :contentReference[oaicite:4]{index=4}
        ESP_ERROR_CHECK(spi_bus_initialize(host_, &buscfg, SPI_DMA_CH_AUTO));

        spi_device_interface_config_t devcfg{};
        devcfg.clock_speed_hz = clock_hz;
        devcfg.mode = 0;
        devcfg.spics_io_num = pins_.cs;
        devcfg.queue_size = qDepth_;
        devcfg.pre_cb = &preCbThunk_;

        // Para write-only, dá para desabilitar dummy/frequency check (permite
        // 80MHz em mais casos). :contentReference[oaicite:5]{index=5}
        devcfg.flags = SPI_DEVICE_HALFDUPLEX;
        if (outputOnlyNoDummy) {
            devcfg.flags |= SPI_DEVICE_NO_DUMMY;
        }

        ESP_ERROR_CHECK(spi_bus_add_device(host_, &devcfg, &spi_));

        // pool de transações + buffers DMA
        for (int i = 0; i < qDepth_; i++) {
            memset(&trans_[i], 0, sizeof(spi_transaction_t));
            dmaBuf_[i] = (uint16_t *)heap_caps_malloc(
                dmaBufWords_ * sizeof(uint16_t),
                MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
            if (!dmaBuf_[i])
                return false;
        }

        // init mínima do ILI9341 (você pode expandir conforme seu setup)
        initILI_();

        return true;
    }

    // Ajuste se você quiser enviar stripes maiores. Deve ser >= bytes do seu
    // push.
    void setMaxTransferBytes(int bytes) { maxTransferBytes_ = bytes; }

    // Se você quer “frame burst”, pode travar o bus (evita outra device no
    // meio).
    void beginFrame(bool acquireBus = true) {
        if (acquireBus) {
            // back-to-back recomendado pelo driver.
            // :contentReference[oaicite:6]{index=6}
            spi_device_acquire_bus(spi_, portMAX_DELAY);
            busAcquired_ = true;
        }
    }

    void endFrame() {
        waitAll();
        if (busAcquired_) {
            spi_device_release_bus(spi_);
            busAcquired_ = false;
        }
    }

    // setAddrWindow por polling (poucos bytes -> overhead de IRQ não compensa,
    // como no exemplo). :contentReference[oaicite:7]{index=7}
    inline void setAddrWindow(int x, int y, int w, int h) {
        uint8_t data[4];

        cmd_(0x2A);
        int xe = x + w - 1;
        data[0] = (x >> 8);
        data[1] = x & 0xFF;
        data[2] = (xe >> 8);
        data[3] = xe & 0xFF;
        data_(data, 4);

        cmd_(0x2B);
        int ye = y + h - 1;
        data[0] = (y >> 8);
        data[1] = y & 0xFF;
        data[2] = (ye >> 8);
        data[3] = ye & 0xFF;
        data_(data, 4);

        cmd_(0x2C); // RAMWR
    }

    // Enfileira pixels (RGB565). Se swapBytes=true, converte little->big
    // durante a cópia para o buffer DMA. Retorna quando a transação foi
    // enfileirada (DMA segue “em segundo plano”).
    inline void pushPixelsAsync(const uint16_t *pixels, size_t count,
                                bool swapBytes = true) {
        // precisa de slot livre; se ring lotar, espera 1 completar
        if (inFlight_ == qDepth_)
            waitOne();

        const int slot = head_;
        uint16_t *dst = dmaBuf_[slot];

        if (count > dmaBufWords_) {
            // por simplicidade: limita ao tamanho do buffer; você pode
            // fragmentar se quiser
            count = dmaBufWords_;
        }

        if (swapBytes) {
            for (size_t i = 0; i < count; i++) {
                dst[i] = __builtin_bswap16(pixels[i]);
            }
        } else {
            memcpy(dst, pixels, count * sizeof(uint16_t));
        }

        spi_transaction_t *t = &trans_[slot];
        memset(t, 0, sizeof(*t));
        t->length = count * 16; // bits
        t->tx_buffer = dst;
        t->user = (void *)(((uintptr_t)pins_.dc << 1) | 1u); // pin + DC=1

        // transação deve permanecer válida até completar.
        // :contentReference[oaicite:9]{index=9}
        ESP_ERROR_CHECK(spi_device_queue_trans(spi_, t, portMAX_DELAY));

        head_ = (head_ + 1) % qDepth_;
        inFlight_++;
    }

    inline void waitOne() {
        if (inFlight_ == 0)
            return;
        spi_transaction_t *r = nullptr;
        ESP_ERROR_CHECK(spi_device_get_trans_result(spi_, &r, portMAX_DELAY));
        tail_ = (tail_ + 1) % qDepth_;
        inFlight_--;
    }

    inline void waitAll() {
        while (inFlight_ > 0)
            waitOne();
    }

    int width() const { return w_; }
    int height() const { return h_; }

    static constexpr uint8_t MADCTL_MY = 0x80;
    static constexpr uint8_t MADCTL_MX = 0x40;
    static constexpr uint8_t MADCTL_MV = 0x20;
    static constexpr uint8_t MADCTL_BGR = 0x08;

    int rotation_ = 0;

    void setRotation(uint8_t m) {
        rotation_ = (m & 3);

        uint8_t madctl = 0;
        switch (rotation_) {
        case 0:
            madctl = (MADCTL_MX | MADCTL_BGR);
            w_ = 240;
            h_ = 320;
            break;
        case 1:
            madctl = (MADCTL_MV | MADCTL_BGR);
            w_ = 320;
            h_ = 240;
            break;
        case 2:
            madctl = (MADCTL_MY | MADCTL_BGR);
            w_ = 240;
            h_ = 320;
            break;
        case 3:
            madctl = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            w_ = 320;
            h_ = 240;
            break;
        }

        // ILI9341_MADCTL = 0x36
        cmd_(0x36);
        data_(&madctl, 1);
    }

  private:
    static constexpr int kMaxQ = 8;

    Pins pins_{};
    int w_ = 0, h_ = 0;
    spi_host_device_t host_{SPI2_HOST};
    spi_device_handle_t spi_{nullptr};

    bool busAcquired_ = false;

    int qDepth_ = 4;
    int head_ = 0;
    int tail_ = 0;
    int inFlight_ = 0;

    // tamanho do buffer DMA por slot (em pixels 16-bit):
    // ajuste conforme seu stripe: ex. 320*16 = 5120.
    size_t dmaBufWords_ = 320 * 16;
    int maxTransferBytes_ = 320 * 16 * 2;

    spi_transaction_t trans_[kMaxQ];
    uint16_t *dmaBuf_[kMaxQ]{};

    // pre_cb precisa ser static; usa t->user como DC (0=cmd,1=data), como no
    // exemplo oficial. :contentReference[oaicite:10]{index=10}
    static void preCbThunk_(spi_transaction_t *t);

    inline void cmd_(uint8_t c) {
        spi_transaction_t t{};
        t.length = 8;
        t.tx_buffer = &c;
        t.user = (void *)(((uintptr_t)pins_.dc << 1) | 0u); // pin + DC=0
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_, &t));
    }

    inline void data_(const uint8_t *d, int len) {
        if (!len)
            return;
        spi_transaction_t t{};
        t.length = len * 8;
        t.tx_buffer = d;
        t.user = (void *)(((uintptr_t)pins_.dc << 1) | 1u); // pin + DC=1
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_, &t));
    }

    inline void hardReset_() {
        if (pins_.rst < 0)
            return;
        digitalWrite(pins_.rst, LOW);
        delay(20);
        digitalWrite(pins_.rst, HIGH);
        delay(120);
    }

    static inline int globalDC_ = -1;

    void initILI_() {
        globalDC_ = pins_.dc;

        // Init minimal (você pode colar aqui sua sequência completa se quiser).
        cmd_(0x01);
        delay(5);   // SWRESET
        cmd_(0x28); // DISPOFF

        // PIXFMT = 16-bit
        cmd_(0x3A);
        uint8_t pix = 0x55;
        data_(&pix, 1);

        cmd_(0x11);
        delay(120); // SLPOUT
        cmd_(0x29); // DISPON
    }
};
