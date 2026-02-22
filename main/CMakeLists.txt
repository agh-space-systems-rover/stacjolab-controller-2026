file(GLOB_RECURSE SRC_FILES "src/*.c")
idf_component_register(SRCS "main.c" ${SRC_FILES}
                    PRIV_REQUIRES esp_wifi nvs_flash driver esp_driver_spi
                    INCLUDE_DIRS "include")
