idf_component_register(SRCS "main.cpp"
                       PRIV_REQUIRES "opendroneid" "esp_event" "esp_netif" "esp_wifi" "nvs_flash" "driver" "mavlink2"
                       INCLUDE_DIRS "../../src")
