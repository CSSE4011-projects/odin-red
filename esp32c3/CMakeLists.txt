# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(CONF_FILE prj.conf)
set(DTC_OVERLAY_FILE dtc_uart.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources}
                            ./src/motordriver.c
                            ./src/rovermotor.c
                            ./src/roveruart.c
                            ./src/pos_mobile_bt.c
                            )
                            

project(Prac2)