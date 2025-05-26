#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "lcd_display.h"
#include "mbedtls/base64.h"
static const char *TAG = "MQTT_HANDLER";
int mbedtls_base64_decode(
    unsigned char *dst, size_t dst_len, size_t *olen,
    const unsigned char *src, size_t src_len);

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT Broker");
        esp_mqtt_client_subscribe(client, "your_mqtt_string_topic", 0);
        esp_mqtt_client_subscribe(client, "your_mqtt_photo_topic", 0);
        break;
    // case MQTT_EVENT_DATA:
    //     ESP_LOGI(TAG, "Received data:");
    //     ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
    //     ESP_LOGI(TAG, "DATA: %.*s", event->data_len, event->data);
    //     break;
    case MQTT_EVENT_DATA:
    {
        esp_mqtt_event_handle_t event = event_data;

        // 將 topic 轉成 C 字串
        char topic[event->topic_len + 1];
        memcpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = '\0';

        // 圖片處理
        if (strcmp(topic, "your_mqtt_photo_topic") == 0)
        {
            ESP_LOGI(TAG, "Received image base64 string, length: %d", event->data_len);

            // 原始字串複製
            char *b64_str = calloc(event->data_len + 1, 1);
            memcpy(b64_str, event->data, event->data_len);
            b64_str[event->data_len] = '\0';

            // 清除非 base64 合法字元
            char *cleaned_str = calloc(event->data_len + 1, 1);
            int j = 0;
            for (int i = 0; i < event->data_len; i++)
            {
                char c = b64_str[i];
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    c == '+' || c == '/' || c == '=')
                {
                    cleaned_str[j++] = c;
                }
            }
            cleaned_str[j] = '\0';
            free(b64_str);

            // base64 decode
            size_t decoded_len = (j * 3 / 4) + 4;
            uint8_t *img_buf = malloc(decoded_len);
            size_t actual_len = 0;

            int ret = mbedtls_base64_decode(
                img_buf,
                decoded_len,
                &actual_len,
                (const uint8_t *)cleaned_str,
                strlen(cleaned_str));
            free(cleaned_str);

            if (ret != 0)
            {
                ESP_LOGE(TAG, "Base64 decode failed: -0x%x", -ret);
                free(img_buf);
                return;
            }

            ESP_LOGI(TAG, "Decoding successful, length: %d", actual_len);

            // 顯示圖片（自動判斷 JPEG / PNG）
            extern void display_img_from_buf(const uint8_t *buf, size_t len);
            display_img_from_buf(img_buf, actual_len);
            free(img_buf);
        }

        if (strcmp(topic, "your_mqtt_string_topic") == 0)
        {
            ESP_LOGI(TAG, "Received text message: %.*s", event->data_len, event->data);
        }

        break;
    }

    default:
        break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org", // v5.4.1 必須這樣寫

    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);
}