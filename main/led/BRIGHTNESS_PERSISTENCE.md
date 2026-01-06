# 💾 Brightness Persistence - Chi Tiết Kỹ Thuật

## 🎯 Vấn Đề & Giải Pháp

### Vấn Đề Cũ
```
Sơ đồ hoạt động CỨ:

User                Settings (Flash)         RAM (brightness_scale_)
  |                        |                         |
  ├─ "50% độ sáng" ──────────────────→ Tính: 128 ──→ brightness_scale_ = 128
  |
  ├─ "LED đỏ" ────────────────────────────────────→ SetColor() xài brightness_scale_
  |
  ├─ Reset / Mất nguồn (RAM mất) ────────────────→ ❌ Mất giá trị 128!
  |
  └─ Khởi động lại
      ├─ Initialize() ────────→ brightness_scale_ = 255 (mặc định) ❌
      └─ "LED đỏ" ────────────→ LED sáng 100% (không phải 50% như mong)
```

### Giải Pháp Mới
```
Sơ đồ hoạt động MỚI:

User                Settings (Flash)         RAM (brightness_scale_)
  |                        |                         |
  ├─ "50% độ sáng" ──────────────────→ Tính: 128 ──→ brightness_scale_ = 128
  |                           ↓
  |                      Save: brightness = 128  (SetBrightness lưu)
  |
  ├─ "LED đỏ" ────────────────────────────────────→ SetColor() xài brightness_scale_=128
  |
  ├─ Reset / Mất nguồn (RAM mất nhưng Flash vẫn) ──→ ✅ Flash vẫn có 128!
  |
  └─ Khởi động lại
      ├─ Initialize() ────────→ Load từ Flash: brightness = 128
      |                   ↓
      |          brightness_scale_ = 128 ✅
      |
      └─ "LED đỏ" ────────────→ LED sáng 50% (đúng như mong!) ✅
```

---

## 📝 Code Chi Tiết

### 1. SetBrightness() - Lưu vào Settings

```cpp
void WS2812Led::SetBrightness(uint8_t percent) {
    if (percent > 100)
        percent = 100;
    
    // Bước 1: Tính brightness_scale từ percent
    brightness_scale_ = (percent * 255) / 100;
    brightness_level_ = brightness_scale_;
    
    // 🔑 Bước 2: LƯU vào Flash Settings
    Settings settings("led_strip", true);  // true = save to flash
    settings.SetInt("brightness", brightness_level_);
    
    ESP_LOGI(TAG, "Brightness set to: %d%% (scale: %d/255)", percent, brightness_scale_);
    
    // Bước 3: Áp dụng ngay lập tức
    SetColor(r_, g_, b_);
}
```

**Điểm quan trọng**:
- `brightness_scale_` = giá trị RAM (0-255) dùng để tính toán màu
- `brightness_level_` = giá trị lưu (0-255) được save vào Flash
- `Settings settings("led_strip", true)` = "led_strip" namespace, true=save to flash

### 2. Initialize() - Khôi Phục từ Settings

```cpp
void WS2812Led::Initialize() {
    // Bước 1: Tạo Settings object để đọc từ Flash
    Settings settings("led_strip");
    
    // Bước 2: Đọc giá trị từ Flash (nếu có), nếu không có dùng mặc định
    numled_ = settings.GetInt("numled", 30);           // Mặc định: 30 LEDs
    brightness_level_ = settings.GetInt("brightness", 255);  // Mặc định: 255 (100%)
    
    // 🔑 Bước 3: Khôi phục brightness_scale_ từ brightness_level_
    brightness_scale_ = brightness_level_;
    
    ESP_LOGI(TAG, "Initialize: NUMLED=%d, BRIGHTNESS_LEVEL=%d, BRIGHTNESS_SCALE=%d", 
             numled_, brightness_level_, brightness_scale_);
    
    // ... khởi tạo LED strip ...
}
```

**Điểm quan trọng**:
- Hàm này chạy khi khởi động → tải giá trị từ Flash vào RAM
- Nếu Flash không có giá trị (lần đầu tiên), dùng giá trị mặc định
- Sau đó tất cả các lệnh LED sẽ dùng brightness_scale_ này

### 3. SetColorOrder() - Giữ Brightness

```cpp
void WS2812Led::SetColorOrder(ColorOrder order) {
    color_order_ = order;
    ESP_LOGI(TAG, "Color order set to: %d", order);
    
    // 🔑 QUAN TRỌNG: Gọi SetColor() sẽ dùng brightness_scale_ hiện tại
    // brightness_scale_ KHÔNG thay đổi, chỉ có cách remap màu thay đổi
    SetColor(r_, g_, b_);
}
```

**Điểm quan trọng**:
- brightness_scale_ không bị reset
- Chỉ có cách remap RGB thay đổi (GBR → RGB, v.v.)
- Độ sáng vẫn giữ nguyên

---

## 🔄 Luồng Hoạt Động Hoàn Chỉnh

### Lần Đầu Tiên Boot (Flash trống)

```
1. Boot → Initialize()
2. Settings.GetInt("brightness", 255) → trả 255 (mặc định)
3. brightness_scale_ = 255
4. Bắt đầu dùng → Brightness = 100% (mặc định)

5. User: "LED 50%"
6. SetBrightness(50) gọi
   ├─ brightness_scale_ = 128
   ├─ Settings.SetInt("brightness", 128) → Lưu vào Flash
   └─ SetColor() áp dụng
```

### Lần Thứ 2 Boot (Flash có dữ liệu)

```
1. Boot → Initialize()
2. Settings.GetInt("brightness", 255) → trả 128 (từ Flash)
3. brightness_scale_ = 128
4. Bắt đầu dùng → Brightness = 50% (được khôi phục) ✅

5. User: "LED đỏ"
6. SetColor(255, 0, 0) gọi
   ├─ R = (255 * 128) >> 8 = 128
   ├─ Hiển thị: LED đỏ 50% sáng (không phải 100%) ✅
```

### Chuyển Color Order (Brightness Vẫn Giữ)

```
1. Trạng thái: brightness_scale_ = 128 (50%)
2. User: "Sửa màu"
3. SetColorOrder(3) gọi
   ├─ color_order_ = 3
   └─ SetColor(r_, g_, b_) // dùng brightness_scale_ = 128
       └─ Brightness vẫn 50% ✅
```

---

## 💡 Ví Dụ Thực Tế

### Scenario 1: Độ Sáng Tối Và Khôi Phục

```
Thời điểm 1:
  User: "Tắt nền, LED sáng 30%"
  → SetBrightness(30)
  → brightness_scale_ = 77
  → Settings.SetInt("brightness", 77)
  → Flash lưu giá trị 77

Thời điểm 2: Mất nguồn (toàn bộ thiết bị tắt)

Thời điểm 3: Bật lại
  → Initialize()
  → brightness_scale_ = 77 (khôi phục từ Flash)
  → User yêu cầu: "LED sáng"
  → LED vẫn sáng 30% (đúng như lúc mất nguồn) ✅
```

### Scenario 2: Chuyển Chế Độ & Giữ Brightness

```
Trạng thái: Brightness = 50% (brightness_scale_ = 128)

Bước 1: User: "LED sáng xanh dương"
  → SetColor(0, 0, 255)
  → B = (255 * 128) >> 8 = 128
  → LED sáng xanh 50%

Bước 2: User: "LED sáng đỏ"
  → SetColor(255, 0, 0)
  → R = (255 * 128) >> 8 = 128
  → LED sáng đỏ 50% (KHÔNG PHẢI 100%!) ✅

Bước 3: User: "Chuyển thành GBR"
  → SetColorOrder(3)
  → Gọi SetColor() với brightness_scale_ = 128
  → LED vẫn 50% sáng ✅
```

---

## 🔍 Các Trạng Thái Của Brightness

| Tên Variable | Nơi Lưu | Mục Đích | Khi Reset |
|-------------|---------|---------|-----------|
| `brightness_scale_` | RAM | Tính toán RGB hiện tại | Mất (reload từ Flash) |
| `brightness_level_` | RAM | Copy của scale, dùng khi save | Mất (reload từ Flash) |
| Settings Flash | Flash | Lưu vĩnh viễn | **Giữ lại** ✅ |

---

## ⚠️ Các Trường Hợp Đặc Biệt

### Trường Hợp 1: Chuyển Color Order
```cpp
SetColorOrder(kColorOrderGBR);
→ Gọi SetColor(r_, g_, b_) 
→ brightness_scale_ giữ nguyên
→ Chỉ remap màu thay đổi
```
✅ Brightness **KHÔNG** thay đổi

### Trường Hợp 2: Gọi SetColor() Trực Tiếp
```cpp
SetColor(255, 0, 0);
→ Sử dụng brightness_scale_ hiện tại
→ Không thay đổi giá trị brightness
```
✅ Brightness **KHÔNG** thay đổi

### Trường Hợp 3: Gọi Hiệu Ứng
```cpp
EffectColorBreathe(255, 0, 255, 50);
→ Hàm tự tính intensity từ 0 đến 1
→ Nhân với r, g, b đã có sẵn
→ Sau đó SetPixelWithRemap() sẽ remap và set
```
✅ Brightness **KHÔNG** thay đổi (hiệu ứng độc lập tính toán)

---

## 📌 Tóm Tắt

| Hành Động | Brightness RAM | Brightness Flash |
|-----------|---------|---------|
| Boot lần 1 | 255 (mặc định) | 255 (mặc định) |
| SetBrightness(50) | 128 | 128 ✅ Lưu |
| SetColor() | 128 | 128 |
| SetColorOrder() | 128 (giữ) | 128 (giữ) |
| Reset / Boot lần 2 | 128 (khôi phục) | 128 (giữ) |

**Kết luận**: Brightness được quản lý **hai lớp** - RAM cho xử lý nhanh, Flash cho lưu trữ lâu dài.

---

Đây là cách đảm bảo brightness vẫn nhớ được như `numled` vậy! 💾✨
