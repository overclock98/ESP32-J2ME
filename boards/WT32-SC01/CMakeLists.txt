cmake_minimum_required(VERSION 3.16)

# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(WT32-SC01_FlintJVM)

add_custom_command(
    TARGET app
    POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} $ENV{IDF_PATH}/components/fatfs/wl_fatfsgen.py
            --output_file storage.bin
            --partition_size 0xE70000
            --sector_size 512
            --long_name_support
            ../../../files
)

add_custom_command(
    TARGET app
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ../../build
    COMMAND esptool.py
            --chip esp32 merge_bin --flash_mode dio --flash_size 16MB --flash_freq 80m
            -o ../../build/WT32-SC01_FlintJVM_Release.bin
            0x1000 bootloader/bootloader.bin
            0x8000 partition_table/partition-table.bin
            0x10000 WT32-SC01_FlintJVM.bin
            0x190000 storage.bin
)
