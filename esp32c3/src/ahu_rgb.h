
#ifndef AHU_RGB_H_
#define AHU_RGB_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

/* Defining macros for nop assembly instructions */
#define NOP1 "nop\n\t"
#define NOP2 NOP1 NOP1
#define NOP4 NOP2 NOP2
#define NOP8 NOP4 NOP4
#define NOP16 NOP8 NOP8
#define NOP32 NOP16 NOP16
#define NOP64 NOP32 NOP32
#define NOP128 NOP64 NOP64

/*
Defining macros for the rgb delays (T0H, T1H, T0L, T0H)

Note that system clock is running at 160MHz and
each nop instruction takes one clock cycle to complete
=> each nop has a delay of 1/160Mhz = 6.25ns

Based on timings for the SK6812 RGB LED:

T0H: 300ns/6.25ns = 48 cycles = 32+16 cycles
T1H: 600ns/6.25ns = 96 cycles = 64+32 cycles
T0L: 900ns/6.25ns = 144 cycles = 128+16 cycles
T1L: 600ns/6.25ns = 96 cycles = 64+32 cycles
*/
#define DELAY_T0H NOP32 NOP16
#define DELAY_T1H NOP64 NOP32
#define DELAY_T0L NOP128 NOP16
#define DELAY_T1L NOP64 NOP32

/* Defining macros with RISC-V assembly to drive specified
   GPIOs high or low */
#define SET_HIGH 	"li t0, 1\n\t" \
					"slli t0, t0, %[gpio_pin]\n\t" \
					"lw t1, 0(%[out_reg])\n\t" \
					"or t1, t1, t0\n\t" \
					"sw t1, 0(%[out_reg])\n\t"

#define SET_LOW 	"li t0, 1\n\t" \
					"slli t0, t0, %[gpio_pin]\n\t" \
					"not t0, t0\n\t" \
					"lw t1, 0(%[out_reg])\n\t" \
					"and t1, t1, t0\n\t" \
					"sw t1, 0(%[out_reg])\n\t"

/* Defining macros for constants */
#define NUM_COLOURS 3
#define RGB_RESET_DELAY_US 80
#define RGB_GPIO_REG 0x60004004
#define RGB_GPIO_PIN 8

/* Macro definitions for ahu rgb thread */
#define AHU_RGB_STACK_SIZE 1024
#define AHU_RGB_PRIORITY 3

/* Macro definitions for ahu rgb control message queue */
#define AHU_RGB_MSGQ_MAX_MSG 10
#define AHU_RGB_MSGQ_ALIGN 4

/* Entry point to ahu rgb thread */
extern void ahu_rgb_thread(void *, void *, void *);

/* struct to control the colour of the ahu rgb */
struct ahu_rgb_colour {
    uint8_t red;
	uint8_t green;
	uint8_t blue;
};

/* Message queue for ahu rgb control */
extern struct k_msgq ahu_rgb_msgq;

#endif