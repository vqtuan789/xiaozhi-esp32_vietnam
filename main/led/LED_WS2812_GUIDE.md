# WS2812 LED Integration Guide

## Overview
Đã tích hợp thành công hệ thống điều khiển LED RGB WS2812B vào dự án xiaozhi qua giao thức MCP (Model Context Protocol).

## Files Added/Modified

### New Files Created
1. **main/led/ws2812_led.h** - Header file định nghĩa class `WS2812Led`
2. **main/led/ws2812_led.cc** - Implementation của WS2812Led class

### Modified Files
1. **main/CMakeLists.txt** - Thêm `led/ws2812_led.cc` vào danh sách source files
2. **main/mcp_server.cc** - Thêm LED tools và khởi tạo LED

## MCP Tools Available

### Color Order Fix (NEW!)
**Tool: self.light.set_color_order**
**Mô tả**: Sửa lỗi hoán đổi màu bằng cách thay đổi thứ tự channel màu

```
Tham số:
  - order (0-5): Thứ tự màu
    0 = RGB  (Chuẩn)
    1 = GRB  (WS2812B tiêu chuẩn)
    2 = BGR
    3 = GBR  (Chip của bạn - HÃY THỬ CÁI NÀY TRƯỚC)
    4 = BRG
    5 = RBG
    
Trả về: true (thành công)

Ví dụ: Nếu bạn ra lệnh LED đỏ nhưng nó sáng xanh lá, hãy thử:
  order=3 (GBR)
```

### 1. self.light.get_power
**Mô tả**: Kiểm tra trạng thái bật/tắt của LED
```
Không có tham số
Trả về: true (bật) hoặc false (tắt)
```

### 2. self.light.turn_on
**Mô tả**: Bật LED với màu trắng
```
Không có tham số
Trả về: true (thành công)
```

### 3. self.light.turn_off
**Mô tả**: Tắt LED
```
Không có tham số
Trả về: true (thành công)
```

### 4. self.light.set_rgb
**Mô tả**: Đặt màu RGB tùy ý cho LED
```
Tham số:
  - r (0-255): Giá trị đỏ
  - g (0-255): Giá trị xanh
  - b (0-255): Giá trị xanh lam
Trả về: true (thành công)

Ví dụ: Bật LED màu đỏ
  r=255, g=0, b=0
```

### 5. self.light.set_brightness
**Mô tả**: Điều chỉnh độ sáng LED (0-100%)
```
Tham số:
  - brightness (0-100): Độ sáng theo phần trăm
Trả về: true (thành công)

Ví dụ: Đặt độ sáng 50%
  brightness=50
```

### 6. self.light.set_num_leds
**Mô tả**: Thiết lập số lượng LED trong dải
```
Tham số:
  - num_leds (1-255): Số lượng LED
Trả về: true (thành công)

Mặc định: 30 LED
```

### 7. self.light.effect_rainbow_spin
**Mô tả**: Hiệu ứng vòng cung (rainbow) xoay tùy theo bass
```
Tham số:
  - bass (0-255): Mức độ bass (tốc độ xoay)
Trả về: true (thành công)
```

### 8. self.light.effect_bass_wave
**Mô tả**: Hiệu ứng sóng bass
```
Tham số:
  - bass (0-255): Mức độ bass
Trả về: true (thành công)
```

### 9. self.light.effect_pulse_ring
**Mô tả**: Hiệu ứng xung (pulse) hình vòng
```
Tham số:
  - bass (0-255): Mức độ bass (tốc độ xung)
Trả về: true (thành công)
```

### 10. self.light.effect_pulse_simple (NEW!)
**Mô tả**: Hiệu ứng xung đơn giản với màu tùy ý - DỄ NHÌN HƠN
```
Tham số:
  - r (0-255): Giá trị đỏ
  - g (0-255): Giá trị xanh
  - b (0-255): Giá trị xanh lam
  - speed (1-100): Tốc độ xung
Trả về: true (thành công)

Ví dụ: Xung màu xanh lam ở tốc độ 50
  r=0, g=0, b=255, speed=50
```

### 11. self.light.effect_color_breathe (NEW!)
**Mô tả**: Hiệu ứng thở mượt mà với màu tùy ý
```
Tham số:
  - r (0-255): Giá trị đỏ
  - g (0-255): Giá trị xanh
  - b (0-255): Giá trị xanh lam
  - speed (1-100): Tốc độ thở
Trả về: true (thành công)

Ví dụ: Thở màu tím ở tốc độ 30
  r=255, g=0, b=255, speed=30
```

### 12. self.light.effect_color_cycle (NEW!)
**Mô tả**: Hiệu ứng chuyển màu vòng quanh các LED
```
Tham số:
  - speed (1-100): Tốc độ chuyển màu
Trả về: true (thành công)

Ví dụ: Chuyển màu tốc độ cao
  speed=80
```

## Hardware Configuration

- **GPIO**: GPIO_NUM_14 (TX0)
- **Model**: LED_MODEL_WS2812 (WS2812B)
- **Clock Source**: RMT_CLK_SRC_DEFAULT
- **Resolution**: 10 MHz
- **Default Number of LEDs**: 30 (có thể thay đổi bằng `set_num_leds`)

## Usage Examples

### Ví dụ 1: Bật LED màu đỏ
```
Tool: self.light.set_rgb
Parameters: r=255, g=0, b=0
```

### Ví dụ 2: Bật LED và điều chỉnh độ sáng 80%
```
Tool 1: self.light.turn_on
Tool 2: self.light.set_brightness (brightness=80)
```

### Ví dụ 3: Chạy hiệu ứng rainbow với bass level 150
```
Tool: self.light.effect_rainbow_spin (bass=150)
```

### Ví dụ 4: Thiết lập 50 LED thay vì 30
```
Tool: self.light.set_num_leds (num_leds=50)
```

## Settings Storage

Cấu hình LED được lưu trữ tự động trong `led_strip` settings:
- `numled`: Số lượng LED (mặc định: 30) - **Lưu vĩnh viễn**
- `brightness`: Độ sáng 0-255 (mặc định: 255) - **Lưu vĩnh viễn**

**Quan trọng**: 
- Những thay đổi sẽ được duy trì sau khi thiết bị khởi động lại
- Ngay cả khi mất nguồn, giá trị brightness sẽ được khôi phục
- Brightness vẫn giữ nguyên khi chuyển đổi giữa các chế độ (màu, hiệu ứng)
- Khi thay đổi color order, brightness hiện tại vẫn được giữ

## C++ API (Direct Usage)

Nếu cần sử dụng trực tiếp từ code C++:

```cpp
#include "led/ws2812_led.h"

// Khởi tạo
WS2812Led led;
led.Initialize();

// Đặt màu
led.SetColor(255, 0, 0);  // Red

// Điều chỉnh độ sáng
led.SetBrightness(80);    // 80%

// Kiểm tra trạng thái
if (led.IsOn()) {
    // LED đang bật
}

// Hiệu ứng
led.EffectRainbowSpin(100);
led.EffectBassWave(150);
led.EffectPulseRing(200);

// Bật/tắt
led.TurnOn();
led.TurnOff();
```

## Integration Flow

1. **Khởi tạo tự động**: WS2812Led được khởi tạo khi `AddCommonTools()` được gọi trong startup
2. **Truy cập qua MCP**: AI assistant có thể gọi các LED tools thông qua giao thức MCP
3. **Voice Control**: Người dùng nói yêu cầu → AI xác định → Gọi tool thích hợp → LED thực thi

## Troubleshooting

### ❌ Vấn đề 1: LED không hiển thị đúng màu (Đỏ thành Xanh Lá, v.v.)

**Nguyên nhân**: Chip LED của bạn sử dụng thứ tự channel khác (không phải RGB chuẩn)

**Giải pháp**: Sử dụng `self.light.set_color_order` để đổi thứ tự màu

**Hướng dẫn**:
1. Bạn nói "LED sáng đỏ" → nhưng nó sáng xanh lá
2. Nó có nghĩa R channel điều khiển G LED
3. Thử `order=3` (GBR) đầu tiên:
   ```
   Tool: self.light.set_color_order
   Parameters: order=3
   ```
4. Nếu vẫn không đúng, thử các giá trị khác (0, 1, 2, 4, 5)

**Bản đồ thử nghiệm**:
- Đỏ → Xanh lá, Xanh lá → Xanh dương, Xanh dương → Đỏ = **Thử order=3 (GBR)**
- Đỏ → Xanh dương, Xanh lá → Đỏ, Xanh dương → Xanh lá = **Thử order=1 (GRB)**
- Đỏ → Xanh dương = **Thử order=2 (BGR)**

### ❌ Vấn đề 2: LED không sáng

- Kiểm tra GPIO 14 có kết nối đúng không
- Kiểm tra nguồn điện cho LED strip (5V, GND)
- Kiểm tra số lượng LED (`self.light.set_num_leds`)
- Thử `self.light.turn_on` trước

### ❌ Vấn đề 3: Hiệu ứng không chạy hoặc khác lạ

- **Giải pháp**: Sử dụng các hiệu ứng MỚI được cải tiến
  - `effect_pulse_simple` - Dễ nhìn, không phức tạp
  - `effect_color_breathe` - Mượt mà, đẹp mắt
  - `effect_color_cycle` - Chuyển màu vòng quanh

### ❌ Vấn đề 4: Độ sáng quá thấp

- Sử dụng `self.light.set_brightness` để tăng độ sáng
- Ví dụ: `brightness=100` (100%)

## LED RGB WS2812B
**Cấu hình hiện tại**: LED không sáng
1. Mở Terminal
2. Kiểm tra GPIO 14 có bị chiếm dụng không
3. Kiểm tra kết nối vật lý

## Notes

- LED được khởi tạo **một lần duy nhất** khi MCP Server khởi động
- Tất cả các tool đều **không yêu cầu tham số bắt buộc** ngoài các tham số được liệt kê
- **Brightness được lưu tự động** - Khi bạn thay đổi brightness, nó sẽ được lưu trong Settings
- **Brightness tồn tại lâu dài** - Ngay cả sau khi reset hay mất nguồn, brightness sẽ được khôi phục
- **Brightness được giữ khi chuyển chế độ** - Khi bạn chuyển từ màu này sang màu khác hoặc hiệu ứng khác, brightness vẫn giữ nguyên
- Settings tự động lưu và khôi phục sau reboot
