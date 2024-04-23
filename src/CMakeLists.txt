idf_component_register(SRCS "main.cpp" "routing.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES "opendroneid" "esp_event" "esp_netif" "esp_wifi" "nvs_flash" "esp_timer" "driver" "mavlink2")
