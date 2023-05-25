#include "roveruart.h"

LOG_MODULE_REGISTER(roveruart);


/** 
 * Initializes uart communications with disco. 
 * Uart is handled interrupt based, with configuration 
 * data stored in handler struct to avoid global variables. 
 * @param dev: zephyr uart device 
 * @param info: empty, to be populated 
 */
int roveruart_init(const struct device* dev, struct roveruart_info* info) 
{
	if (!device_is_ready(dev)) {
		LOG_ERR("UART: Device is not ready.\n");
		return -1;
	}

	/* Create queue with handler and buffer stored in hanlder struct. */
	k_msgq_init(&(info->receive), &(info->receive_buf[0]), sizeof(rover_position_info_t), 10);

	/* Enable uart character ISR. Send handler struct to callback. */
    int ret = uart_irq_callback_user_data_set(dev, roveruart_rx_cb, (void*) info);
	uart_irq_rx_enable(dev);
	info->dev = dev;
	return ret; 
}


/**
 * Callback for when a character is received over uart. 
 */
void roveruart_rx_cb(const struct device* dev, void* user_data) 
{	
	struct roveruart_info* info = ((struct roveruart_info*) user_data);
	uint8_t rxd;
	static int num_rxd = 0; 
	rover_position_info_t pos; 
	// Update our uart IRQ, and make sure it's ready to avoid spurious calls. 
	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	// Read each char from the fifo and process. 
	while (uart_fifo_read(dev, &rxd, 1) == 1) {
		if (rxd == MSG_PREAMBLE) {
			// Preamble byte
			num_rxd = 1; 
		} else if (num_rxd == 1) {
			// first byte - x position
			pos.x = rxd;
			num_rxd++; 
		} else if (num_rxd > 1){
			// Last byte, update y and put in queue for main to process. 
			pos.y = rxd; 
			num_rxd = 0;
			k_msgq_put(&info->receive, &pos, K_NO_WAIT);
		}

	}
}


/** 
 * Sends command over UART to reset the angle reference of the disco
 * @param info: handler struct
 */
// Realistically this should run more than quick enough to not need threading. 
void roveruart_reset_angle(struct roveruart_info* info) 
{
	LOG_INF("Resetting angle");
	uart_poll_out(info->dev, 0x69);
}


/**
 * Returns -EAGAIN if no message is waiting in queue, -ENOMSG if other failure, 
 * otherwsie populates pos struct pointer with values. 
 * @param info: handler struct 
 * @param pos: empty struct to populate in this call. 
 */
int roveruart_get_new_position(
		struct roveruart_info* info, 
		rover_position_info_t* pos) 
{
	int res = k_msgq_get(&(info->receive), (void*) pos, K_NO_WAIT);
	return res; 
}