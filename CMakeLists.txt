# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(scd41_send_test)

# Add the executable target
#add_executable(app src/main.c)

target_sources(app PRIVATE src/main.c
	src/scd4x_i2c.c
	src/sensirion_common.c
	src/sensirion_i2c.c
	src/sensirion_i2c_hal.c )

# Optionally, if you have header files, you can include their directories
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)