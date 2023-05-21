/*
* Driver for BSU HCI
*/

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include "pos_base_hci.h"

struct k_msgq * hci_queue;
struct k_event *hci_ev;

sys_slist_t* hci_list;

uint8_t pedals[3];

int period;

static int64_t time_stamp;
LOG_MODULE_REGISTER(app);

struct k_heap node_heap;
K_HEAP_DEFINE(node_heap, sizeof(uint8_t) * 100);

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart),
	     "Console device is not ACM CDC UART device");

static int cmd_pedal(const struct shell *shell, size_t argc, char **argv)
{
	pedals[0] = atoi(argv[1]);
	pedals[1] = atoi(argv[2]);
	pedals[2] = atoi(argv[3]);
	printk("values: %d, %d, %d\n", pedals[0], pedals[1], pedals[2]);
	return 0;
}

SHELL_CMD_ARG_REGISTER(pedal, NULL, "Pedal data received", cmd_pedal, 0, 3);

void hci_th(struct Data * input)
{
	//k_heap_init(node_heap, pointer, sizeof(uint8_t) * 100);
	hci_queue = input->msgq;
	hci_ev = input->event;
	hci_list = input->n_list;
	const struct device *dev;
	uint32_t baudrate, dtr = 0U;
	int ret;
	period = 1;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));

	time_stamp = k_uptime_get();

	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_WRN("Failed to set DCD, ret code %d", ret);
	}

	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_WRN("Failed to set DSR, ret code %d", ret);
	}

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_WRN("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_INF("Baudrate detected: %d", baudrate);
	}

	uint32_t events = 0;

	while (1) {
		k_msleep(100);
	}
}