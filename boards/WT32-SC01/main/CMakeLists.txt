include(${CMAKE_CURRENT_SOURCE_DIR}/../../Common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../FlintJVM.cmake)

set(priv_requires fatfs console esp_adc esp_driver_dac esp_timer nvs_flash driver esp_wifi esp_lcd esp_driver_i2c)

if(CONFIG_EXAMPLE_STORAGE_MEDIA_SPIFLASH)
    list(APPEND priv_requires wear_levelling esp_partition)
endif()

idf_component_register(
    SRCS "../../../espjvm/core/src/esp_board_esp32.cpp"
    SRCS "../../../espjvm/core/src/esp_uart_debugger.cpp"

    SRCS "lcd.c"
    SRCS "touch.c"
    SRCS "flint_native_lcd.cpp"
    SRCS "flint_native_touch.cpp"
    SRCS "flint_native_png.cpp"

    SRCS ${COMMON_SRCS}
    SRCS ${FLINTJVM_SRCS}

    INCLUDE_DIRS "."
    INCLUDE_DIRS ${COMMON_INCS}
    INCLUDE_DIRS ${FLINTJVM_INCS}

    PRIV_REQUIRES "${priv_requires}"
)

add_compile_definitions(ESP32_WT32SC01)
add_compile_definitions(MULTI_HEAP_FREERTOS)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-address)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-volatile)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-uninitialized)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-strict-aliasing)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-cast-function-type)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-ignored-qualifiers)
