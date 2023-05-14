# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(CONF_FILE prj.conf)
set(DTC_OVERLAY_FILE dtc_shell.overlay)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources}
                            ~/csse4011/odin_red/disco/drivers/angle_sensors/angle_sensors.c
                            ~/csse4011/odin_red/disco/drivers/distance_sensors/distance_sensors.c
                            ~/csse4011/odin_red/disco/drivers/serial_comms/serial_comms.c
                            )
target_include_directories(app PRIVATE ~/csse4011/odin_red/disco/drivers)

project(odin_red)