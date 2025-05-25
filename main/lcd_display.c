// lcd_display.c
#include "lcd_display.h"
#include "TFT_eSPI.h"
#include "TJpg_Decoder.h"
#include "esp_log.h"

static const char *TAG = "LCD_DISPLAY";

TFT_eSPI tft = TFT_eSPI();

// 回調函式：JPEG 解碼器將像素繪製到 TFT
static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (y >= tft.height()) return 0;
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

void lcd_init(void) {
    tft.init();
    tft.setRotation(1);  // 可根據需要調整方向
    tft.fillScreen(TFT_BLACK);

    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

    ESP_LOGI(TAG, "LCD 初始化完成");
}

void display_jpg_from_buf(const uint8_t *jpg_buf, size_t jpg_len) {
    ESP_LOGI(TAG, "顯示 JPEG 圖片, size=%d", jpg_len);
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawJpg(0, 0, jpg_buf, jpg_len);
}
