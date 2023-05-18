
#include "serial_comms.h"

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* Initialising message queue to be used by hci thread for requests */
K_MSGQ_DEFINE(
    serial_comms_msgq,
    sizeof(struct serial_comms_data),
    SERIAL_COMMS_MSGQ_MAX_MSG,
    SERIAL_COMMS_MSGQ_ALIGN
);

/* Semaphore to signal refernce angle updates */
K_SEM_DEFINE(serial_ref_angle_sem, 0, 1);

/* Obtain the node for uart4 */
#define UART_DEVICE_NODE DT_NODELABEL(uart4)

/* Get UART information from devicetree */
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
char rx_buf[MSG_SIZE];
int rx_buf_pos;

/* Callback used for UART receive interupt. Ignores serial input
    until the byte 0x69 is detected, and then reads up until
    the specified length of the message. */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {

		// TODO: handle updating angle here (probably semaphore)
		if (c == UPDATE_ANGLE_BYTE) {
            /* Give semaphore to signal update of reference angle */
            k_sem_give(&serial_ref_angle_sem);
        }

	}
}

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void serial_comms_thread(void *, void *, void *)
{
    /* Check if UART is ready */
    if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

    /* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return;
	}
	uart_irq_rx_enable(uart_dev);

    /* Struct for receiving messages from msgq */
    struct serial_comms_data outgoing_position;
    
    while (1) {

        /* Check for outgoing messages sent from main */
        if(!k_msgq_get(&serial_comms_msgq, &outgoing_position, K_NO_WAIT)) {
            /* New outgoing message */
            char outgoing_buf[MSG_SIZE];
            outgoing_buf[0] = PREAMBLE;
            outgoing_buf[1] = outgoing_position.x_position;
            outgoing_buf[2] = outgoing_position.y_position;
            outgoing_buf[3] = '\0';
            print_uart(outgoing_buf, 3);
        }

        k_sleep(K_MSEC(100));
    }
}