#ifndef _ROVERUART_H_
#define _ROVERUART_H_
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#define LOG_LEVEL 4
#define MSG_PREAMBLE 0xFF
#define MSG_RESET_ANGLE 0x69
typedef uint8_t roveruart_transmit_t;

typedef struct rover_position_info_t {
    uint8_t x; 
    uint8_t y; 
} rover_position_info_t;

struct roveruart_info {
    struct k_msgq transmit; 
    char __aligned(4) transmit_buf[10 * sizeof(roveruart_transmit_t)];
    struct k_msgq receive; 
    char __aligned(4) receive_buf[10 * sizeof(rover_position_info_t)];
    const struct device* dev;
};



extern int roveruart_init(const struct device* dev, struct roveruart_info* info);
void roveruart_rx_cb(const struct device* dev, void* user_data);
int roveruart_get_new_position(
		struct roveruart_info* info, 
		struct rover_position_info_t* pos);
extern void roveruart_reset_angle(struct roveruart_info* info);

#endif 