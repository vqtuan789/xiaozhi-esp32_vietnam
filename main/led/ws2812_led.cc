#include <esp_log.h>
#include <math.h>
#include "ws2812_led.h"
#include "settings.h"
#include "led_strip.h"

#define LED_GPIO GPIO_NUM_14  // TX0
#define TAG "WS2812_LED"

static const led_strip_rmt_config_t bsp_rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10 * 1000 * 1000,
    .flags = {
        .with_dma = false
    }
};

WS2812Led::WS2812Led() {
    ESP_LOGI(TAG, "WS2812Led instance created");
}

WS2812Led::~WS2812Led() {
    if (led_strip_ != nullptr) {
        led_strip_del((led_strip_handle_t)led_strip_);
        led_strip_ = nullptr;
    }
}

void WS2812Led::Initialize() {
    Settings settings("led_strip");
    numled_ = settings.GetInt("numled", 30);
    brightness_level_ = settings.GetInt("brightness", 255);
    
    // Calculate brightness scale from brightness level
    brightness_scale_ = brightness_level_;  // brightness_level_ is already in 0-255 range
    
    ESP_LOGI(TAG, "Initialize: NUMLED=%d, BRIGHTNESS_LEVEL=%d, BRIGHTNESS_SCALE=%d", 
             numled_, brightness_level_, brightness_scale_);

    static const led_strip_config_t bsp_strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = numled_,
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false
        }
    };

    ESP_LOGI(TAG, "LED GPIO: %d", bsp_strip_config.strip_gpio_num);
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&bsp_strip_config, &bsp_rmt_config, (led_strip_handle_t*)&led_strip_));

    // Turn off all LEDs initially
    for (int i = 0; i < numled_; i++) {
        led_strip_set_pixel((led_strip_handle_t)led_strip_, i, 0x00, 0x00, 0x00);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

esp_err_t WS2812Led::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    esp_err_t ret = ESP_OK;

    // Scale brightness
    uint8_t R = (r * brightness_scale_) >> 8;
    uint8_t G = (g * brightness_scale_) >> 8;
    uint8_t B = (b * brightness_scale_) >> 8;

    for (int i = 0; i < numled_; i++) {
        SetPixelWithRemap(i, R, G, B);
    }
    ret |= led_strip_refresh((led_strip_handle_t)led_strip_);

    // Store color
    r_ = r;
    g_ = g;
    b_ = b;

    return ret;
}

void WS2812Led::RemapColor(uint8_t& r, uint8_t& g, uint8_t& b) const {
    uint8_t temp_r = r, temp_g = g, temp_b = b;
    
    switch (color_order_) {
        case kColorOrderRGB:
            // No change needed
            break;
        case kColorOrderGRB:
            // GRB order (standard WS2812B)
            r = temp_g;
            g = temp_r;
            b = temp_b;
            break;
        case kColorOrderBGR:
            // BGR order
            r = temp_b;
            g = temp_g;
            b = temp_r;
            break;
        case kColorOrderGBR:
            // GBR order (user's current chip)
            r = temp_g;
            g = temp_b;
            b = temp_r;
            break;
        case kColorOrderBRG:
            // BRG order
            r = temp_b;
            g = temp_r;
            b = temp_g;
            break;
        case kColorOrderRBG:
            // RBG order
            r = temp_r;
            g = temp_b;
            b = temp_g;
            break;
        default:
            break;
    }
}

void WS2812Led::SetPixelWithRemap(int index, uint8_t r, uint8_t g, uint8_t b) {
    RemapColor(r, g, b);
    led_strip_set_pixel((led_strip_handle_t)led_strip_, index, r, g, b);
}

void WS2812Led::SetColorOrder(ColorOrder order) {
    color_order_ = order;
    ESP_LOGI(TAG, "Color order set to: %d", order);
    
    // Re-apply current color with new color order, maintaining brightness
    // brightness_scale_ is preserved, so color will maintain current brightness
    SetColor(r_, g_, b_);
}

void WS2812Led::SetBrightness(uint8_t percent) {
    if (percent > 100)
        percent = 100;
    
    brightness_scale_ = (percent * 255) / 100;  // Convert 0–100% to 0–255
    brightness_level_ = brightness_scale_;      // Store the actual scale value
    
    // Save brightness to settings for persistence across reboots
    Settings settings("led_strip", true);
    settings.SetInt("brightness", brightness_level_);
    
    ESP_LOGI(TAG, "Brightness set to: %d%% (scale: %d/255)", percent, brightness_scale_);
    
    // Apply current color with new brightness
    SetColor(r_, g_, b_);
}

void WS2812Led::SetNumLeds(uint8_t num) {
    if (num > 0 && num <= 255) {
        numled_ = num;
        Settings settings("led_strip", true);
        settings.SetInt("numled", num);
        ESP_LOGI(TAG, "Number of LEDs set to %d", numled_);
    }
}

void WS2812Led::TurnOn() {
    led_on_ = true;
    SetColor(r_, g_, b_);
}

void WS2812Led::TurnOff() {
    led_on_ = false;
    SetColor(0x00, 0x00, 0x00);
}

// gọi khi trạng thái thiết bị thay đổi
void WS2812Led::OnStateChanged() {
    // This is called when device state changes
    // Can be extended to show different effects based on device state
}

// hiệu ứng quay vòng cầu vồng
void WS2812Led::EffectRainbowSpin(int bass) {
    static float offset = 0.0f;
    offset += (bass / 255.0f) * 0.1f;  // Increased speed
    if (offset > 1.0f) offset -= 1.0f;

    for (int i = 0; i < numled_; i++) {
        float hue = fmodf((float)i / numled_ + offset, 1.0f);

        // Better color distribution
        uint8_t r = 0, g = 0, b = 0;
        
        if (hue < 0.166f) {
            r = 255;
            g = (uint8_t)(255 * (hue / 0.166f));
        } else if (hue < 0.333f) {
            r = (uint8_t)(255 * (1.0f - (hue - 0.166f) / 0.166f));
            g = 255;
        } else if (hue < 0.5f) {
            g = 255;
            b = (uint8_t)(255 * ((hue - 0.333f) / 0.166f));
        } else if (hue < 0.666f) {
            g = (uint8_t)(255 * (1.0f - (hue - 0.5f) / 0.166f));
            b = 255;
        } else if (hue < 0.833f) {
            r = (uint8_t)(255 * ((hue - 0.666f) / 0.166f));
            b = 255;
        } else {
            r = 255;
            b = (uint8_t)(255 * (1.0f - (hue - 0.833f) / 0.166f));
        }

        SetPixelWithRemap(i, r, g, b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// hiệu ứng sóng bass
void WS2812Led::EffectBassWave(int bass) {
    static float phase = 0;
    phase += 0.05f;  // Slower, smoother
    if (phase > 6.28f) phase -= 6.28f;

    for (int i = 0; i < numled_; i++) {
        // Create wave effect
        float wave = sinf((i * 0.3f) + phase) * 0.5f + 0.5f;
        
        uint8_t r = (uint8_t)(wave * bass * 0.8f);
        uint8_t g = (uint8_t)(wave * bass * 0.3f);
        uint8_t b = (uint8_t)((1.0f - wave) * bass);

        SetPixelWithRemap(i, r, g, b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// hiệu ứng vòng nhấp nháy
void WS2812Led::EffectPulseRing(int bass) {
    static float pulse = 0.0f;

    float speed = bass / 255.0f * 0.1f;
    pulse += speed;
    if (pulse > 1.0f)
        pulse = 0.0f;

    for (int i = 0; i < numled_; i++) {
        float pos = (float)i / numled_;
        float distance = fabsf(pos - pulse);
        if (distance > 0.5f) distance = 1.0f - distance;
        
        float intensity = 1.0f - (distance * 2.0f);
        if (intensity < 0) intensity = 0;
        intensity = intensity * intensity;  // Smoothen the curve

        uint8_t r = (uint8_t)(intensity * 255 * 0.8f);
        uint8_t g = (uint8_t)(intensity * 255 * 0.4f);
        uint8_t b = (uint8_t)(intensity * 255);

        SetPixelWithRemap(i, r, g, b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// chế độ nhấp nháy đơn giản
void WS2812Led::EffectPulseSimple(uint8_t r, uint8_t g, uint8_t b, int speed) {
    static float pulse = 0.0f;
    pulse += (speed / 100.0f) * 0.02f;
    if (pulse > 1.0f) pulse -= 1.0f;

    // Create brightness pulse effect
    float brightness = (sinf(pulse * 6.28f) + 1.0f) / 2.0f;  // 0 to 1
    
    uint8_t pulsed_r = (uint8_t)(r * brightness);
    uint8_t pulsed_g = (uint8_t)(g * brightness);
    uint8_t pulsed_b = (uint8_t)(b * brightness);

    for (int i = 0; i < numled_; i++) {
        SetPixelWithRemap(i, pulsed_r, pulsed_g, pulsed_b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// chế độ đảo màu liên tục
void WS2812Led::EffectColorBreathe(uint8_t r, uint8_t g, uint8_t b, int speed) {
    static float breathe = 0.0f;
    breathe += (speed / 100.0f) * 0.01f;
    if (breathe > 1.0f) breathe -= 1.0f;

    // Smooth breathing curve
    float intensity = (sinf(breathe * 6.28f) + 1.0f) / 2.0f;
    intensity = intensity * intensity;  // Smoother curve

    uint8_t br = (uint8_t)(r * intensity);
    uint8_t bg = (uint8_t)(g * intensity);
    uint8_t bb = (uint8_t)(b * intensity);

    for (int i = 0; i < numled_; i++) {
        SetPixelWithRemap(i, br, bg, bb);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// chế đô chạy mầu liên tục
void WS2812Led::EffectColorCycle(int speed) {
    static float cycle = 0.0f;
    cycle += (speed / 100.0f) * 0.01f;
    if (cycle > 1.0f) cycle -= 1.0f;

    for (int i = 0; i < numled_; i++) {
        float hue = fmodf(cycle + (float)i / numled_, 1.0f);
        
        uint8_t r = 0, g = 0, b = 0;
        
        // HSV to RGB conversion
        if (hue < 0.166f) {
            r = 255;
            g = (uint8_t)(255 * (hue / 0.166f));
        } else if (hue < 0.333f) {
            r = (uint8_t)(255 * (1.0f - (hue - 0.166f) / 0.166f));
            g = 255;
        } else if (hue < 0.5f) {
            g = 255;
            b = (uint8_t)(255 * ((hue - 0.333f) / 0.166f));
        } else if (hue < 0.666f) {
            g = (uint8_t)(255 * (1.0f - (hue - 0.5f) / 0.166f));
            b = 255;
        } else if (hue < 0.833f) {
            r = (uint8_t)(255 * ((hue - 0.666f) / 0.166f));
            b = 255;
        } else {
            r = 255;
            b = (uint8_t)(255 * (1.0f - (hue - 0.833f) / 0.166f));
        }

        SetPixelWithRemap(i, r, g, b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}
