file(GLOB_RECURSE EXTRA_SRCS "../include/*.c")
idf_component_register(SRCS "app_main.c"  ${EXTRA_SRCS}
                    PRIV_REQUIRES spi_flash nvs_flash esp_netif esp_wifi mqtt spiffs
                    INCLUDE_DIRS "../include")
                    # REQUIRES arduino TFT_eSPI TJpg_Decoder)
