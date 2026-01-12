#ifndef _LED_WS2812_H_
#define _LED_WS2812_H_

#include <esp_err.h>
#include "led.h"

// Color order enum for different LED types
enum ColorOrder {
    kColorOrderRGB = 0,   // Standard RGB order
    kColorOrderGRB = 1,   // WS2812B, SK6812
    kColorOrderBGR = 2,   // Some LED types
    kColorOrderGBR = 3,   // User's current chip
    kColorOrderBRG = 4,
    kColorOrderRBG = 5
};

class WS2812Led : public Led {
public:
    WS2812Led();
    ~WS2812Led();
    
    void OnStateChanged() override;
    
    // Initialize LED strip
    void Initialize();
    
    // Set LED color (0-255 for each channel)
    esp_err_t SetColor(uint8_t r, uint8_t g, uint8_t b);
    
    // Set brightness (0-100%)
    void SetBrightness(uint8_t percent);
    
    // Set number of LEDs
    void SetNumLeds(uint8_t num);
    
    // Get number of LEDs
    uint8_t GetNumLeds() const { return numled_; }
    
    // Set color order (for different LED chip types)
    void SetColorOrder(ColorOrder order);
    
    // Visual effects
    void EffectRainbowSpin(int bass);
    void EffectBassWave(int bass);
    void EffectPulseRing(int bass);
    void EffectPulseSimple(uint8_t r, uint8_t g, uint8_t b, int speed);
    void EffectColorBreathe(uint8_t r, uint8_t g, uint8_t b, int speed);
    void EffectColorCycle(int speed);
    
    // Control
    void TurnOn();
    void TurnOff();
    bool IsOn() const { return led_on_; }
    
    // Task control for continuous LED effects
    void StartEffectTask(int effect_type, int param1 = 0, uint8_t param_r = 0, uint8_t param_g = 0, uint8_t param_b = 0);
    void StopEffectTask();
    
private:
    // LED state
    bool led_on_ = false;
    uint8_t r_ = 0, g_ = 0, b_ = 0;
    uint8_t brightness_scale_ = 255;  // 0–255
    uint8_t brightness_level_ = 255;   // 0–255
    uint8_t numled_ = 30;              // Number of LEDs
    void* led_strip_ = nullptr;        // led_strip_handle_t
    ColorOrder color_order_ = kColorOrderGBR;  // Default to user's chip (GBR)
    
    // Task management
    TaskHandle_t led_effect_task_handle_ = nullptr;  // FreeRTOS task handle
    bool led_effect_task_should_stop_ = false;
    
    // Current effect state
    int current_effect_type_ = -1;  // -1 = none
    int effect_param1_ = 0;
    uint8_t effect_param_r_ = 0;
    uint8_t effect_param_g_ = 0;
    uint8_t effect_param_b_ = 0;
    
    // Animation state variables (used in effect functions)
    float effect_offset_ = 0.0f;    // For rainbow spin
    float effect_phase_ = 0.0f;     // For bass wave
    float effect_pulse_ = 0.0f;     // For pulse ring
    float effect_breathe_ = 0.0f;   // For color breathe
    float effect_cycle_ = 0.0f;     // For color cycle
    
    // Task wrapper and main function
    static void ledEffectTaskWrapper(void* arg);
    void ledEffectTask();
    
    // Helper function to remap RGB colors based on color order
    void RemapColor(uint8_t& r, uint8_t& g, uint8_t& b) const;
    void SetPixelWithRemap(int index, uint8_t r, uint8_t g, uint8_t b);
};

#endif // _LED_WS2812_H_
