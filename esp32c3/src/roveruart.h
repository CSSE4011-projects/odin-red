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
    /* UNUSED */ 
    struct k_msgq transmit; 
    /* UNUSED */
    char __aligned(4) transmit_buf[10 * sizeof(roveruart_transmit_t)];
    /* Contains each full message received from disco over uart */
    struct k_msgq receive; 
    /* Buffer for values contained in receive queue. */
    char __aligned(4) receive_buf[10 * sizeof(rover_position_info_t)];
    /* zephyr uart device instance */
    const struct device* dev;
};


/** 
 * Initializes uart communications with disco. 
 * Uart is handled interrupt based, with configuration 
 * data stored in handler struct to avoid global variables. 
 * @param dev: zephyr uart device 
 * @param info: empty, to be populated 
 */
extern int roveruart_init(const struct device* dev, struct roveruart_info* info);
void roveruart_rx_cb(const struct device* dev, void* user_data);


/**
 * Returns -EAGAIN if no message is waiting in queue, -ENOMSG if other failure, 
 * otherwsie populates pos struct pointer with values. 
 * @param info: handler struct 
 * @param pos: empty struct to populate in this call. 
 */
extern int roveruart_get_new_position(
		struct roveruart_info* info, 
		struct rover_position_info_t* pos);


/** 
 * Sends command over UART to reset the angle reference of the disco
 * @param info: handler struct
 */
extern void roveruart_reset_angle(struct roveruart_info* info);

#endif 