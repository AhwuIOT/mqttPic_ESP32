#include "lcd_display.h"
#include "st7735s.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
TFT_t dev;

void lcd_init(void) {
    spi_master_init(&dev, GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RESET);
    lcdInit(&dev, LCD_WIDTH, LCD_HEIGHT, LCD_OFFSET_X, LCD_OFFSET_Y, FRAME_BUFFER);
    lcdFillScreen(&dev, BLACK);
    lcdDrawFinish(&dev);
    
}

bool is_jpeg(const uint8_t *buf, size_t len) {
    return len >= 3 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF;
}

bool is_png(const uint8_t *buf, size_t len) {
    return len >= 8 &&
           buf[0] == 0x89 &&
           buf[1] == 0x50 &&
           buf[2] == 0x4E &&
           buf[3] == 0x47 &&
           buf[4] == 0x0D &&
           buf[5] == 0x0A &&
           buf[6] == 0x1A &&
           buf[7] == 0x0A;
}
TickType_t JPEGTest(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	pixel_jpeg **pixels;
	int imageWidth;
	int imageHeight;
	esp_err_t err = decode_jpeg(&pixels, file, width, height, &imageWidth, &imageHeight);
	ESP_LOGI(__FUNCTION__, "decode_image err=%d imageWidth=%d imageHeight=%d", err, imageWidth, imageHeight);
	if (err == ESP_OK) {
		ESP_LOGD(__FUNCTION__, "imageWidth=%d imageHeight=%d", imageWidth, imageHeight);

		uint16_t jpegWidth = width;
		uint16_t offsetX = 0;
		if (width > imageWidth) {
			jpegWidth = imageWidth;
			offsetX = (width - imageWidth) / 2;
		}
		ESP_LOGD(__FUNCTION__, "jpegWidth=%d offsetX=%d", jpegWidth, offsetX);

		uint16_t jpegHeight = height;
		uint16_t offsetY = 0;
		if (height > imageHeight) {
			jpegHeight = imageHeight;
			offsetY = (height - imageHeight) / 2;
		}
		ESP_LOGD(__FUNCTION__, "jpegHeight=%d offsetY=%d", jpegHeight, offsetY);
		uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * jpegWidth);
		if (colors == NULL) {
			ESP_LOGE(__FUNCTION__, "malloc fail");
			release_image(&pixels, width, height);
			return 0;
		}

#if 0
		for(int y = 0; y < jpegHeight; y++){
			for(int x = 0;x < jpegWidth; x++){
				//pixel_s pixel = pixels[y][x];
				//uint16_t color = rgb565(pixel.red, pixel.green, pixel.blue);
				uint16_t color = pixels[y][x];
				lcdDrawPixel(dev, x+offsetX, y+offsetY, color);
			}
			vTaskDelay(1);
		}
#endif

		for(int y = 0; y < jpegHeight; y++){
			for(int x = 0;x < jpegWidth; x++){
				//pixel_s pixel = pixels[y][x];
				//colors[x] = rgb565(pixel.red, pixel.green, pixel.blue);
				colors[x] = pixels[y][x];
			}
			lcdDrawMultiPixels(dev, offsetX, y+offsetY, jpegWidth, colors);
			vTaskDelay(1);
		}

		lcdDrawFinish(dev);
		free(colors);
		release_image(&pixels, width, height);
		ESP_LOGD(__FUNCTION__, "Finish");
	} else {
		ESP_LOGE(__FUNCTION__, "decode_image err=%d", err);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%"PRIu32,diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

TickType_t PNGTest(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	// open PNG file
	FILE* fp = fopen(file, "rb");
	if (fp == NULL) {
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0;
	}

	char buf[1024];
	size_t remain = 0;
	int len;

	pngle_t *pngle = pngle_new(width, height);
	ESP_LOGD(__FUNCTION__, "pngle=%p", pngle);
	if (pngle == NULL) {
		fclose(fp);
		return 0;
	}

	pngle_set_init_callback(pngle, png_init);
	pngle_set_draw_callback(pngle, png_draw);
	pngle_set_done_callback(pngle, png_finish);

	double display_gamma = 2.2;
	pngle_set_display_gamma(pngle, display_gamma);


	while (!feof(fp)) {
		if (remain >= sizeof(buf)) {
			ESP_LOGE(__FUNCTION__, "Buffer exceeded");
			while(1) vTaskDelay(1);
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
		if (len <= 0) {
			//printf("EOF\n");
			break;
		}

		int fed = pngle_feed(pngle, buf, remain + len);
		if (fed < 0) {
			ESP_LOGE(__FUNCTION__, "ERROR; %s", pngle_error(pngle));
			pngle_destroy(pngle, width, height);
			return 0;
		}

		remain = remain + len - fed;
		if (remain > 0) memmove(buf, buf + fed, remain);
	}
	fclose(fp);

	uint16_t pngWidth = width;
	uint16_t offsetX = 0;
	if (width > pngle->imageWidth) {
		pngWidth = pngle->imageWidth;
		offsetX = (width - pngle->imageWidth) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngWidth=%d offsetX=%d", pngWidth, offsetX);

	uint16_t pngHeight = height;
	uint16_t offsetY = 0;
	if (height > pngle->imageHeight) {
		pngHeight = pngle->imageHeight;
		offsetY = (height - pngle->imageHeight) / 2;
	}
	ESP_LOGD(__FUNCTION__, "pngHeight=%d offsetY=%d", pngHeight, offsetY);
	uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * pngWidth);
	if (colors == NULL) {
		ESP_LOGE(__FUNCTION__, "malloc fail");
		pngle_destroy(pngle, width, height);
		return 0;
	}

#if 0
	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			//pixel_png pixel = pngle->pixels[y][x];
			//uint16_t color = rgb565(pixel.red, pixel.green, pixel.blue);
			uint16_t color = pngle->pixels[y][x];
			lcdDrawPixel(dev, x+offsetX, y+offsetY, color);
		}
		vTaskDelay(1);
	}
#endif

	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			//pixel_png pixel = pngle->pixels[y][x];
			//colors[x] = rgb565(pixel.red, pixel.green, pixel.blue);
			colors[x] = pngle->pixels[y][x];
		}
		lcdDrawMultiPixels(dev, offsetX, y+offsetY, pngWidth, colors);
		vTaskDelay(1);
	}
	lcdDrawFinish(dev);
	free(colors);
	pngle_destroy(pngle, width, height);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%"PRIu32,diffTick*portTICK_PERIOD_MS);
	return diffTick;
}

void display_img_from_buf(const uint8_t *buf, size_t len) {
    const char *path = "/spiffs/tmp.img";
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "❌ 無法寫入圖片檔");
        return;
    }
    size_t written = fwrite(buf, 1, len, fp);
    fclose(fp);

    ESP_LOGI(TAG, "📄 已寫入圖片 %s 大小: %d bytes", path, written);
    if (written == 0) {
        ESP_LOGE(TAG, "❌ 寫入為 0，放棄顯示圖片");
        return;
    }

    if (is_jpeg(buf, len)) {
        ESP_LOGI(TAG, "📸 圖片格式為 JPEG");
        JPEGTest(&dev, (char *)path, LCD_WIDTH, LCD_HEIGHT);
    } else if (is_png(buf, len)) {
        ESP_LOGI(TAG, "🖼️ 圖片格式為 PNG");
        PNGTest(&dev, (char *)path, LCD_WIDTH, LCD_HEIGHT);
    } else {
        ESP_LOGW(TAG, "❌ 不支援的圖片格式");
    }
}
