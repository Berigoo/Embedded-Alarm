idf_component_register(
                    SRCS "main.cpp" i2c_master.cpp lcd1602.cpp socket.cpp storage.cpp wifi.cpp Kconfig.projbuild
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
