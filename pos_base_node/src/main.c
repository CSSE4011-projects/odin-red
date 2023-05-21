/* main.c - Application main entry point */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <console/console.h>
#include "../../oslib/pos_base_node/pos_base_bt.h"
#include "../../oslib/pos_base_node/pos_base_hci.h"


#include <ctype.h>
#include <stdio.h>

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif


#define STACKSIZE 1024
#define PRIORITY 7
#define MSG_SIZE 32

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

sys_slist_t node_list;
struct k_event sens;
static struct Data data = {&uart_msgq, &sens, &node_list};


void thread1(void) {
	bt_th(&data);
}

void thread2(void) {
	bt_read();
}

void thread3(void) {
	hci_th(&data);
}



K_THREAD_DEFINE(t1, STACKSIZE, thread1, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(t2, STACKSIZE, thread2, NULL, NULL, NULL,
		PRIORITY, 0, 0);
K_THREAD_DEFINE(t3, STACKSIZE * 3, thread3, NULL, NULL, NULL,
		PRIORITY, 0, 0);
