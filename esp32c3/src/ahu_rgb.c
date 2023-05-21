
#include "ahu_rgb.h"

/*
Defining private macro functions to write a '1' or a '0' to
the specified gpio register and pin. Based on macros defined in ahu_rgb.h
*/
#define ONE_BIT(reg, pin) do { \
	__asm volatile (SET_HIGH \
			DELAY_T1H \
			SET_LOW \
			DELAY_T1L \
			:: \
			[out_reg] "r" (reg), \
			[gpio_pin] "i" (pin)); } while (false)

#define ZERO_BIT(reg, pin) do { \
	__asm volatile (SET_HIGH \
			DELAY_T0H \
			SET_LOW \
			DELAY_T0L \
			:: \
			[out_reg] "r" (reg), \
			[gpio_pin] "i" (pin)); } while (false)

/* colour as an uint32_t (using the first 24 bits) in the order of GRB */
#define WRITE_RGB(reg, pin, colour) do { \
	__asm__ volatile ( \
			"li t2, 0x1000000\n\t" \
			"loop_start: srli t2, t2, 1\n\t" \
			"bnez t2, bit_high\n\t" \
			"j loop_end\n\t" \
			"bit_high: and t3, %[rgb_value], t2\n\t" \
			"beqz t3, bit_low\n\t" \
			SET_HIGH DELAY_T1H SET_LOW DELAY_T1L \
			"j loop_start\n\t" \
			"bit_low: " SET_HIGH DELAY_T0H SET_LOW DELAY_T0L \
			"j loop_start\n\t" \
			"loop_end: " \
			:: \
			[out_reg] "r" (reg), \
			[gpio_pin] "i" (pin), \
			[rgb_value] "r" (colour) \
			: "t0", "t1", "t2", "t3", "memory" \
		); \
	} while (false)

/* Initialising message queue to be used by ahu rgb thread */
K_MSGQ_DEFINE(
    ahu_rgb_msgq,
    sizeof(struct ahu_rgb_colour),
    AHU_RGB_MSGQ_MAX_MSG,
    AHU_RGB_MSGQ_ALIGN
);

/* Function definition */
void send_rgb(uint8_t red, uint8_t green, uint8_t blue);


/* The devicetree node identifier for the "rgb_led" alias. */
#define RGB_LED_NODE DT_ALIAS(rgb)

/* Get GPIO pin information from devicetree */
static const struct gpio_dt_spec rgb = GPIO_DT_SPEC_GET(RGB_LED_NODE, gpios);

void ahu_rgb_thread(void *, void *, void *)
{
    /* Ensure that the GPIO pin for rgb control is ready */
	if (!gpio_is_ready_dt(&rgb)) {
		return;
	}

    /* Initialise the GPIO pin to be logic level low */
    gpio_pin_configure_dt(&rgb, GPIO_OUTPUT_INACTIVE);

	/* Ensure rgb gpio low */
	gpio_pin_set_dt(&rgb, 0);

    /* Struct to receive ahu rgb control data */
    struct ahu_rgb_colour colour;
	colour.red=0; colour.green=0; colour.blue=0;

    while (1) {
        /* Check message queue for rgb control structs */
        k_msgq_get(&ahu_rgb_msgq, &colour, K_NO_WAIT);

		send_rgb(0, 0, 0);
		send_rgb(colour.red, colour.green, colour.blue);

        /* Sleep for 500ms */
        k_msleep(500);

    }

}

/* Helper function to send rgb values to the addressable led */
void send_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
	/* Retrieve gpio output register and gpio pin */
	volatile uint32_t *out_reg = (uint32_t *)RGB_GPIO_REG;
	const uint32_t gpio_pin = (uint32_t)RGB_GPIO_PIN;

	/* Create a byte array to store the rgb colour in the
		correct order (GRB) */
	uint8_t colour_buf[NUM_COLOURS];
	colour_buf[0] = green;
	colour_buf[1] = red;
	colour_buf[2] = blue;
	uint8_t *p_colour_buf = colour_buf;

	/* Disable interrupts and preemption */
	k_sched_lock();
	unsigned int key = irq_lock();

	/* Loop through three colours */
	for (int i = 0; i < NUM_COLOURS; i++) {
		
		uint32_t b = *p_colour_buf++;

		for (int32_t j = 7; j >= 0; j--) {
			if (b & BIT(j)) {
				ONE_BIT(out_reg, gpio_pin);
			} else {
				ZERO_BIT(out_reg, gpio_pin);
			}
		}

	}

	/* Re-enable interrupts and preemption */
	irq_unlock(key);
	k_sched_unlock();

    /* Reset delay to ensure LED latches */
	k_busy_wait(RGB_RESET_DELAY_US);

}
