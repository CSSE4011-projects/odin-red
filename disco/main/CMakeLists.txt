# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(CONF_FILE prj.conf)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include_directories(~/csse4011/odin-red/disco/drivers)
target_include_directories(app PRIVATE ~/csse4011/odin-red/disco/drivers)

target_sources(app PRIVATE ~/csse4011/odin-red/disco/drivers/angle_sensors/angle_sensors.c)
target_sources(app PRIVATE ~/csse4011/odin-red/disco/drivers/distance_sensors/distance_sensors.c)
target_sources(app PRIVATE ~/csse4011/odin-red/disco/drivers/serial_comms/serial_comms.c)

project(odin_red)