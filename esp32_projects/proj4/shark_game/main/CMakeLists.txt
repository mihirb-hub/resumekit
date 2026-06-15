idf_component_register(SRCS "shark_game.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES rgblcd1602 esp_timer esp_driver_i2c esp_driver_gpio
)
