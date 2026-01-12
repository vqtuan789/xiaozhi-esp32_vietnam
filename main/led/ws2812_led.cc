#include <esp_log.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "ws2812_led.h"
#include "settings.h"
#include "led_strip.h"

#define LED_GPIO GPIO_NUM_14  // TX0
#define TAG "WS2812_LED"

// Effect types enum
enum LedEffectType {
    EFFECT_NONE = -1,
    EFFECT_RAINBOW_SPIN = 0,
    EFFECT_BASS_WAVE = 1,
    EFFECT_PULSE_RING = 2,
    EFFECT_PULSE_SIMPLE = 3,
    EFFECT_COLOR_BREATHE = 4,
    EFFECT_COLOR_CYCLE = 5
};

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
    // Stop the effect task first
    StopEffectTask();
    
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

    // Stop any running effect when setting static color
    if (current_effect_type_ != EFFECT_NONE) {
        StopEffectTask();
    }

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
    
    // If current color is black (0,0,0), default to white when turning on
    if (r_ == 0 && g_ == 0 && b_ == 0) {
        SetColor(255, 255, 255);  // Default to white on first turn on
    }
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
    effect_offset_ += (bass / 255.0f) * 0.1f;  // Increased speed
    if (effect_offset_ > 1.0f) effect_offset_ -= 1.0f;

    for (int i = 0; i < numled_; i++) {
        float hue = fmodf((float)i / numled_ + effect_offset_, 1.0f);

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
    effect_phase_ += 0.05f;  // Slower, smoother
    if (effect_phase_ > 6.28f) effect_phase_ -= 6.28f;

    for (int i = 0; i < numled_; i++) {
        // Create wave effect
        float wave = sinf((i * 0.3f) + effect_phase_) * 0.5f + 0.5f;
        
        uint8_t r = (uint8_t)(wave * bass * 0.8f);
        uint8_t g = (uint8_t)(wave * bass * 0.3f);
        uint8_t b = (uint8_t)((1.0f - wave) * bass);

        SetPixelWithRemap(i, r, g, b);
    }
    led_strip_refresh((led_strip_handle_t)led_strip_);
}

// hiệu ứng vòng nhấp nháy
void WS2812Led::EffectPulseRing(int bass) {
    float speed = bass / 255.0f * 0.1f;
    effect_pulse_ += speed;
    if (effect_pulse_ > 1.0f)
        effect_pulse_ = 0.0f;

    for (int i = 0; i < numled_; i++) {
        float pos = (float)i / numled_;
        float distance = fabsf(pos - effect_pulse_);
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
    effect_pulse_ += (speed / 100.0f) * 0.02f;
    if (effect_pulse_ > 1.0f) effect_pulse_ -= 1.0f;

    // Create brightness pulse effect
    float brightness = (sinf(effect_pulse_ * 6.28f) + 1.0f) / 2.0f;  // 0 to 1
    
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
    effect_breathe_ += (speed / 100.0f) * 0.01f;
    if (effect_breathe_ > 1.0f) effect_breathe_ -= 1.0f;

    // Smooth breathing curve
    float intensity = (sinf(effect_breathe_ * 6.28f) + 1.0f) / 2.0f;
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
    effect_cycle_ += (speed / 100.0f) * 0.01f;
    if (effect_cycle_ > 1.0f) effect_cycle_ -= 1.0f;

    for (int i = 0; i < numled_; i++) {
        float hue = fmodf(effect_cycle_ + (float)i / numled_, 1.0f);
        
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

// =====================================================
// Task management for continuous LED effects
// =====================================================

void WS2812Led::StartEffectTask(int effect_type, int param1, uint8_t param_r, uint8_t param_g, uint8_t param_b) {
    // Stop existing task if running
    StopEffectTask();
    
    // Turn on LED before starting effect
    TurnOn();
    
    // Reset animation states
    effect_offset_ = 0.0f;
    effect_phase_ = 0.0f;
    effect_pulse_ = 0.0f;
    effect_breathe_ = 0.0f;
    effect_cycle_ = 0.0f;
    
    // Store effect parameters
    current_effect_type_ = effect_type;
    effect_param1_ = param1;
    effect_param_r_ = param_r;
    effect_param_g_ = param_g;
    effect_param_b_ = param_b;
    
    // Start the effect task
    led_effect_task_should_stop_ = false;
    xTaskCreatePinnedToCore(
        ledEffectTaskWrapper,
        "led_effect",       // Task name
        1024 * 4,           // Stack size (4KB - increased from 1KB to handle effect calculations)
        this,               // Parameter
        1,                  // Priority (same as display task)
        &led_effect_task_handle_,
        0                   // Run on core 0 (same as display)
    );
    
    ESP_LOGI(TAG, "LED effect task started (effect_type=%d)", effect_type);
}

void WS2812Led::StopEffectTask() {
    if (led_effect_task_handle_ != nullptr) {
        ESP_LOGI(TAG, "Stopping LED effect task");
        led_effect_task_should_stop_ = true;
        
        // Wait for the task to stop (wait up to 1 second)
        int wait_count = 0;
        while (led_effect_task_handle_ != nullptr && wait_count < 100) {
            vTaskDelay(pdMS_TO_TICKS(10));
            wait_count++;
        }
        
        if (led_effect_task_handle_ != nullptr) {
            ESP_LOGW(TAG, "LED effect task did not stop gracefully, force deleting");
            vTaskDelete(led_effect_task_handle_);
            led_effect_task_handle_ = nullptr;
        } else {
            ESP_LOGI(TAG, "LED effect task stopped successfully");
        }
    }
    
    current_effect_type_ = EFFECT_NONE;
}

void WS2812Led::ledEffectTaskWrapper(void* arg) {
    auto self = static_cast<WS2812Led*>(arg);
    self->ledEffectTask();
}

void WS2812Led::ledEffectTask() {
    ESP_LOGI(TAG, "LED effect task running");
    
    // Main effect loop
    while (!led_effect_task_should_stop_) {
        if (!led_on_) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        
        switch (current_effect_type_) {
            case EFFECT_RAINBOW_SPIN:
                EffectRainbowSpin(effect_param1_);
                break;
                
            case EFFECT_BASS_WAVE:
                EffectBassWave(effect_param1_);
                break;
                
            case EFFECT_PULSE_RING:
                EffectPulseRing(effect_param1_);
                break;
                
            case EFFECT_PULSE_SIMPLE:
                EffectPulseSimple(effect_param_r_, effect_param_g_, effect_param_b_, effect_param1_);
                break;
                
            case EFFECT_COLOR_BREATHE:
                EffectColorBreathe(effect_param_r_, effect_param_g_, effect_param_b_, effect_param1_);
                break;
                
            case EFFECT_COLOR_CYCLE:
                EffectColorCycle(effect_param1_);
                break;
                
            default:
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
        
        // Small delay to control animation frame rate (approximately 30 FPS)
        vTaskDelay(pdMS_TO_TICKS(33));
    }
    
    ESP_LOGI(TAG, "LED effect task stopping");
    led_effect_task_handle_ = nullptr;
    vTaskDelete(nullptr);
}
