#include <stdio.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lcd_display.h"
#include "mqtt_handler.h"
#include "esp_spiffs.h"


#define WIFI_SSID "ahwufamily"
#define WIFI_PASS "29670221"

static const char *TAG = "APP_MAIN";
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI("WIFI", "Got IP, starting MQTT");
        mqtt_app_start(); // ✅ Wi-Fi 連上後才啟動 MQTT
    }
}
void mount_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}
void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t sta_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_start();
}
void show_last_image_if_exists() {
    const char *img_path = "/spiffs/tmp.img";

    FILE *fp = fopen(img_path, "rb");
    if (!fp) {
        ESP_LOGW("BOOT_IMAGE", "❗ 沒有上次圖片可顯示");
        return;
    }

    // 讀出檔案內容
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    uint8_t *buf = malloc(size);
    if (!buf) {
        ESP_LOGE("BOOT_IMAGE", "❌ 無法配置記憶體");
        fclose(fp);
        return;
    }

    fread(buf, 1, size, fp);
    fclose(fp);

    // 顯示
    extern void display_img_from_buf(const uint8_t *buf, size_t len);
    display_img_from_buf(buf, size);
    free(buf);
}

void app_main(void)
{
    nvs_flash_init();
    mount_spiffs();
    lcd_init();
    show_last_image_if_exists(); 
    wifi_init_sta();
}