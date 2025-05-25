idf_component_register(SRCS "app_main.c" "mqtt_handler.c" "lcd_display.c" "decode_jpeg.c" "fontx.c" "st7735s.c"
                    PRIV_REQUIRES spi_flash nvs_flash esp_netif esp_wifi mqtt spiffs
                    INCLUDE_DIRS "")
                    # REQUIRES arduino TFT_eSPI TJpg_Decoder)
