idf_component_register(SRCS "app_main.c" "mqtt_handler.c" 
                    PRIV_REQUIRES spi_flash nvs_flash esp_netif esp_wifi mqtt
                    INCLUDE_DIRS "")
