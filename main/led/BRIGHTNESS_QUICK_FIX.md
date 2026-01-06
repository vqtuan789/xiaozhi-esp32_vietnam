# 🔧 Brightness Persistence - Tóm Tắt Nhanh

## ✅ Đã Sửa

```
❌ Trước: Brightness reset 100% sau reboot / chuyển chế độ
✅ Sau:  Brightness được lưu & khôi phục từ Flash
```

## 📝 Cách Dùng

### Sử dụng
```
Nói: "Giảm độ sáng còn 50%"
→ AI gọi: self.light.set_brightness (brightness=50)
→ Lưu vào Flash tự động
```

### Kiểm Tra
```
Bước 1: Bảo "LED sáng 30%"
  Tool: self.light.set_brightness (brightness=30)
  
Bước 2: Mất nguồn & bật lại

Bước 3: Bảo "LED sáng đỏ"
  → LED vẫn sáng 30% (KHÔNG PHẢI 100%) ✅
```

## 🔄 Hoạt Động

| Bước | Hành Động | RAM | Flash |
|------|----------|-----|-------|
| 1 | Boot | brightness_scale_=255 | brightness=255 |
| 2 | SetBrightness(50) | brightness_scale_=128 | brightness=128 ✅ |
| 3 | Reset | ❌ Mất | ✅ Giữ 128 |
| 4 | Boot lại | brightness_scale_=128 | brightness=128 |
| 5 | SetColor() | Xài 128 | - |

## 📊 So Sánh

| Tính Năng | Trước | Sau |
|-----------|--------|-----|
| Brightness lưu vĩnh viễn | ❌ RAM only | ✅ Flash |
| Khôi phục sau reboot | ❌ Mất | ✅ Nhớ |
| Giữ khi chuyển chế độ | ❌ Reset 100% | ✅ Giữ |
| Giữ khi chuyển color order | ❌ Reset 100% | ✅ Giữ |

## 💾 Nơi Lưu

**Settings namespace**: `"led_strip"`

**Key**: `"brightness"` (giá trị 0-255)

**Khi nào save**:
- Mỗi khi gọi `SetBrightness()`
- Tự động flush vào Flash

## 📁 Files Thay Đổi

- ✅ `ws2812_led.cc`
  - `Initialize()` - Load brightness từ Flash
  - `SetBrightness()` - Save brightness vào Flash

- ✅ Docs:
  - `LED_WS2812_GUIDE.md` - Cập nhật
  - `FIX_NOTES_VI.md` - Thêm vấn đề 3
  - `BRIGHTNESS_PERSISTENCE.md` - Chi tiết kỹ thuật

---

**Bây giờ brightness hoạt động giống `numled` - nhớ được thay đổi!** ✨
