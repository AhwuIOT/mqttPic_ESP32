#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "lcd_display.h"
#define MQTT_BROKER "mqtt://test.mosquitto.org"
#define MQTT_TOPIC "esp32/test"
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
        esp_mqtt_client_subscribe(client, "esp32/test", 0);
        esp_mqtt_client_subscribe(client, "esp32/display/image", 0);
        break;
    // case MQTT_EVENT_DATA:
    //     ESP_LOGI(TAG, "Received data:");
    //     ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
    //     ESP_LOGI(TAG, "DATA: %.*s", event->data_len, event->data);
    //     break;
    case MQTT_EVENT_DATA:
    {
        esp_mqtt_event_handle_t event = event_data;

        // 將 topic 轉成 C 字串處理
        char topic[event->topic_len + 1];
        memcpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = '\0';

        // 判斷是哪個 topic 來的
        if (strcmp(topic, "esp32/display/image") == 0)
        {
            ESP_LOGI(TAG, "收到圖片 base64 字串, 長度 %d", event->data_len);

            char *b64_str = calloc(event->data_len + 1, 1);
            memcpy(b64_str, event->data, event->data_len);
            b64_str[event->data_len] = '\0';

            // 移除非法 base64 字元
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

            // 解碼
            size_t decoded_len = (j * 3 / 4) + 4; // ⚠️ 加保險空間
            uint8_t *jpg_buf = malloc(decoded_len);
            size_t actual_len = 0;

            int ret = mbedtls_base64_decode(
                jpg_buf,
                decoded_len,
                &actual_len,
                (const uint8_t *)cleaned_str,
                strlen(cleaned_str));

            if (ret != 0)
            {
                ESP_LOGE(TAG, "Base64 decode failed: -0x%x", -ret);
                ESP_LOGI(TAG, "Cleaned base64 前50字元：%.*s", 50, cleaned_str);
            }
            else
            {
                ESP_LOGI(TAG, "解碼成功，長度 %d", actual_len);
                display_jpg_from_buf(jpg_buf, actual_len);
            }
            free(cleaned_str);
            free(jpg_buf);
        }
        if (strcmp(topic, "esp32/test") == 0)
        {
            ESP_LOGI(TAG, "收到文字訊息: %.*s", event->data_len, event->data);
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