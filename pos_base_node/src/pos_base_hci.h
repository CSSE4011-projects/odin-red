#ifndef BSU_HCI_H
#define BSU_HCI_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

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

struct Data {
	struct k_msgq * msgq;
	struct k_event * event;
	sys_slist_t* n_list;
};

struct Element {
	sys_snode_t* el_node;
	char el_name[10];
	uint8_t el_mac[6];
	uint16_t el_maj;
	uint16_t el_min;
	uint8_t el_x;
	uint8_t el_y;
	char* el_left;
	char* el_right;
};

void hci_th(struct Data*);
void shell_rc(void);

#endif