idf_component_register(SRCS "mosaico_tof.c" INCLUDE_DIRS "include" PRIV_REQUIRES esp_driver_gpio freertos)
include(driver_fix.cmake)
