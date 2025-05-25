#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "lcd_display.h"
#define MQTT_BROKER "mqtt://test.mosquitto.org"
#define MQTT_TOPIC "esp32/test"

static const char *TAG = "MQTT_HANDLER";

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
        if (strcmp(topic, "esp32/test") == 0)
        {
            // ✅ 處理文字訊息
            ESP_LOGI(TAG, "收到文字訊息: %.*s", event->data_len, event->data);
        }
        else if (strcmp(topic, "esp32/display/image") == 0)
        {
            // ✅ 處理圖片
            ESP_LOGI(TAG, "收到圖片資料 (base64 長度: %d)", event->data_len);
            display_jpg_from_buf(jpg_buf, decoded_len);

            // 建議進一步處理：base64 解碼 → 顯示（你下一步會實作這個）
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