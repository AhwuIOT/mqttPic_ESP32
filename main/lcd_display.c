#include "lcd_display.h"
#include "st7735s.h"
#include "decode_jpeg.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#define LCD_WIDTH 128
#define LCD_HEIGHT 160
#define LCD_OFFSET_X 0
#define LCD_OFFSET_Y 0
#define GPIO_MOSI 23
#define GPIO_SCLK 18
#define GPIO_CS 5
#define GPIO_DC 2
#define GPIO_RESET 4
#define FRAME_BUFFER true

static const char *TAG = "LCD_DISPLAY";
static TFT_t dev;

void lcd_init(void)
{
    spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
    lcdInit(&dev, LCD_WIDTH, LCD_HEIGHT, LCD_OFFSET_X, LCD_OFFSET_Y, FRAME_BUFFER);
    lcdFillScreen(&dev, BLACK);
    lcdDrawFinish(&dev);
}
void display_jpg_from_buf(const uint8_t *jpg_buf, size_t jpg_len)
{
    // 1. 寫入 JPEG 到 /spiffs/tmp.jpg
    const char *filepath = "/spiffs/tmp.jpg";
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "❌ 無法開啟圖片檔寫入: %s", filepath);
        return;
    }

    size_t written = fwrite(jpg_buf, 1, jpg_len, fp);
    fclose(fp);
    ESP_LOGI(TAG, "📄 已寫入 %s: %d bytes", filepath, written);
    if (written == 0) {
        ESP_LOGE(TAG, "❌ 寫入為 0，放棄顯示圖片");
        return;
    }

    // 2. 再次確認實際檔案大小（用 fstat）
    struct stat st;
    if (stat(filepath, &st) == 0) {
        ESP_LOGI(TAG, "📦 stat 確認檔案大小: %ld bytes", st.st_size);
    } else {
        ESP_LOGE(TAG, "❌ stat 檢查失敗");
    }

    // 3. 解碼並顯示圖片
    pixel_jpeg **pixels;
    int imageWidth, imageHeight;
    esp_err_t res = decode_jpeg(&pixels, filepath, LCD_WIDTH, LCD_HEIGHT, &imageWidth, &imageHeight);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "✅ 解碼成功, 解析度 %dx%d", imageWidth, imageHeight);
        int offsetX = (LCD_WIDTH - imageWidth) / 2;
        int offsetY = (LCD_HEIGHT - imageHeight) / 2;
        uint16_t *line_buf = malloc(sizeof(uint16_t) * imageWidth);
        for (int y = 0; y < imageHeight; y++) {
            for (int x = 0; x < imageWidth; x++) {
                line_buf[x] = pixels[y][x];
            }
            lcdDrawMultiPixels(&dev, offsetX, offsetY + y, imageWidth, line_buf);
        }
        lcdDrawFinish(&dev);
        free(line_buf);
        release_image(&pixels, LCD_WIDTH, LCD_HEIGHT);
    } else {
        ESP_LOGE(TAG, "❌ JPEG 解碼失敗, decode_jpeg 回傳錯誤碼: %d", res);
    }
}
