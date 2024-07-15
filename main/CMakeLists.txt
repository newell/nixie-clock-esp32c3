idf_component_register(SRCS "motion.c" "vfs.c" "config.c" "wifi_prov.c" "clock.c" "ws_server.c" "main.c" "sntp.c" "leds.c" "audio.c"
                    SRC_DIRS "."
                    INCLUDE_DIRS "."
                    EMBED_FILES "favicon.ico" "body.html" "iro.min.js" "settings.js" "styles.css")

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
