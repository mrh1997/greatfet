/*
 * This file is part of GreatFET
 */

#include "operacake.h"

#include <greatfet_core.h>
#include <pins.h>
#include <libopencm3/lpc43xx/scu.h>

#define OPERACAKE_PIN_OE(x)      (x<<7)
#define OPERACAKE_PIN_U2CTRL1(x) (x<<6)
#define OPERACAKE_PIN_U2CTRL0(x) (x<<5)
#define OPERACAKE_PIN_U3CTRL1(x) (x<<4)
#define OPERACAKE_PIN_U3CTRL0(x) (x<<3)
#define OPERACAKE_PIN_U1CTRL(x)  (x<<2)
#define OPERACAKE_PIN_LEDEN2(x)  (x<<1)
#define OPERACAKE_PIN_LEDEN(x)   (x<<0)

#define OPERACAKE_PORT_A1 (OPERACAKE_PIN_U2CTRL0(0) | OPERACAKE_PIN_U2CTRL1(0))
#define OPERACAKE_PORT_A2 (OPERACAKE_PIN_U2CTRL0(1) | OPERACAKE_PIN_U2CTRL1(0))
#define OPERACAKE_PORT_A3 (OPERACAKE_PIN_U2CTRL0(0) | OPERACAKE_PIN_U2CTRL1(1))
#define OPERACAKE_PORT_A4 (OPERACAKE_PIN_U2CTRL0(1) | OPERACAKE_PIN_U2CTRL1(1))

#define OPERACAKE_PORT_B1 (OPERACAKE_PIN_U3CTRL0(0) | OPERACAKE_PIN_U3CTRL1(0))
#define OPERACAKE_PORT_B2 (OPERACAKE_PIN_U3CTRL0(1) | OPERACAKE_PIN_U3CTRL1(0))
#define OPERACAKE_PORT_B3 (OPERACAKE_PIN_U3CTRL0(0) | OPERACAKE_PIN_U3CTRL1(1))
#define OPERACAKE_PORT_B4 (OPERACAKE_PIN_U3CTRL0(1) | OPERACAKE_PIN_U3CTRL1(1))

#define OPERACAKE_SAMESIDE  OPERACAKE_PIN_U1CTRL(1)
#define OPERACAKE_CROSSOVER OPERACAKE_PIN_U1CTRL(0)
#define OPERACAKE_EN_LEDS (OPERACAKE_PIN_LEDEN2(1) | OPERACAKE_PIN_LEDEN(0))
#define OPERACAKE_GPIO_ENABLE OPERACAKE_PIN_OE(0)
#define OPERACAKE_GPIO_DISABLE OPERACAKE_PIN_OE(1)

#define OPERACAKE_REG_INPUT    0x00
#define OPERACAKE_REG_OUTPUT   0x01
#define OPERACAKE_REG_POLARITY 0x02
#define OPERACAKE_REG_CONFIG   0x03

#define OPERACAKE_DEFAULT_OUTPUT (OPERACAKE_GPIO_DISABLE | OPERACAKE_SAMESIDE \
								  | OPERACAKE_PORT_A1 | OPERACAKE_PORT_B1 \
								  | OPERACAKE_EN_LEDS)
// All outputs controlled by I2C expander
#define OPERACAKE_CONFIG_ALL_OUTPUT (0x00)
// OE and LEDs are outputs, the rest are not
#define OPERACAKE_CONFIG_GPIO_MODE (0x7C)
#define OPERACAKE_DEFAULT_ADDRESS 0x18

i2c_bus_t* const oc_bus = &i2c0;
uint8_t operacake_boards[8] = {0,0,0,0,0,0,0,0};

/* read single register */
uint8_t operacake_read_reg(i2c_bus_t* const bus, uint8_t address, uint8_t reg) {
	const uint8_t data_tx[] = { reg };
	uint8_t data_rx[] = { 0x00 };
	i2c_bus_transfer(bus, address, data_tx, 1, data_rx, 1);
	return data_rx[0];
}

/* Write to one of the PCA9557 registers */
void operacake_write_reg(i2c_bus_t* const bus, uint8_t address, uint8_t reg, uint8_t value) {
	const uint8_t data[] = {reg, value};
	i2c_bus_transfer(bus, address, data, 2, NULL, 0);
}

uint8_t operacake_init(void) {
	uint8_t reg, addr, i, j = 0;
	/* Find connected operacakes */
	for(i=0; i<8; i++) {
		addr = OPERACAKE_DEFAULT_ADDRESS | i;
		operacake_write_reg(oc_bus, addr, OPERACAKE_REG_OUTPUT,
		                    OPERACAKE_DEFAULT_OUTPUT);
		operacake_write_reg(oc_bus, addr, OPERACAKE_REG_CONFIG,
		                    OPERACAKE_CONFIG_ALL_OUTPUT);
		reg = operacake_read_reg(oc_bus, addr, OPERACAKE_REG_CONFIG);
		if(reg==OPERACAKE_CONFIG_ALL_OUTPUT)
			operacake_boards[j++] = addr;
	}
	return j;
}

static struct gpio_t u1ctrl  = GPIO(2, 5);
static struct gpio_t u2ctrl0 = GPIO(2, 3);
static struct gpio_t u2ctrl1 = GPIO(2, 6);
static struct gpio_t u3ctrl0 = GPIO(2, 4);
static struct gpio_t u3ctrl1 = GPIO(2, 2);

void operacake_gpio(void) {
    // U1CTRL
    scu_pinmux(SCU_PINMUX_GPIO2_5, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    gpio_output(&u1ctrl);
    gpio_write(&u1ctrl, 1);
    // U2CTRL0
    scu_pinmux(SCU_PINMUX_GPIO2_3, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    gpio_output(&u2ctrl0);
    gpio_write(&u2ctrl0, 0);
    // U2CTRL1
    scu_pinmux(SCU_PINMUX_GPIO2_6, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    gpio_output(&u2ctrl1);
    gpio_write(&u2ctrl1, 0);
    // U3CTRL0
    scu_pinmux(SCU_PINMUX_GPIO2_4, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    gpio_output(&u3ctrl0);
    gpio_write(&u3ctrl0, 0);
    // U3CTRL1
    scu_pinmux(SCU_PINMUX_GPIO2_2, SCU_GPIO_FAST | SCU_CONF_FUNCTION0);
    gpio_output(&u3ctrl1);
    gpio_write(&u3ctrl1, 0);
	// Configure pins on I/O expander to not control GPIO pins
	operacake_write_reg(oc_bus, OPERACAKE_DEFAULT_ADDRESS,
	                    OPERACAKE_REG_CONFIG, OPERACAKE_CONFIG_GPIO_MODE);
	operacake_write_reg(oc_bus, OPERACAKE_DEFAULT_ADDRESS,
	                    OPERACAKE_REG_OUTPUT,
	                    OPERACAKE_GPIO_ENABLE | OPERACAKE_EN_LEDS);
}

uint8_t port_to_pins(uint8_t port) {
	switch(port) {
		case OPERACAKE_PA1:
			return OPERACAKE_PORT_A1;
		case OPERACAKE_PA2:
			return OPERACAKE_PORT_A2;
		case OPERACAKE_PA3:
			return OPERACAKE_PORT_A3;
		case OPERACAKE_PA4:
			return OPERACAKE_PORT_A4;

		case OPERACAKE_PB1:
			return OPERACAKE_PORT_B1;
		case OPERACAKE_PB2:
			return OPERACAKE_PORT_B2;
		case OPERACAKE_PB3:
			return OPERACAKE_PORT_B3;
		case OPERACAKE_PB4:
			return OPERACAKE_PORT_B4;
	}
	return 0xFF;
}

uint8_t operacake_set_ports(uint8_t address, uint8_t PA, uint8_t PB) {
	uint8_t side, pa, pb, reg;
	/* Start with some error checking,
	 * which should have been done either
	 * on the host or elsewhere in firmware
	 */
	if((PA > OPERACAKE_PB4) || (PB > OPERACAKE_PB4)) {
		return 1;
	}
	/* Check which side PA and PB are on */
	if((((PA <= OPERACAKE_PA4) && (PB <= OPERACAKE_PA4))
	    || ((PA > OPERACAKE_PA4) && (PB > OPERACAKE_PA4)))
	    && !(PA == 0 && PB == 0)) {
		return 1;
	}
	
	if(PA > OPERACAKE_PA4) {
		side = OPERACAKE_CROSSOVER;
	} else {
		side = OPERACAKE_SAMESIDE;
	}

	pa = port_to_pins(PA);
	pb = port_to_pins(PB);
		
	reg = (OPERACAKE_GPIO_DISABLE | side
					| pa | pb | OPERACAKE_EN_LEDS);
	operacake_write_reg(oc_bus, address, OPERACAKE_REG_OUTPUT, reg);
	return 0;
}

typedef struct {
	uint16_t freq_min;
	uint16_t freq_max;
	uint8_t portA;
	uint8_t portB;
} operacake_range;

static operacake_range ranges[MAX_OPERACAKE_RANGES * sizeof(operacake_range)];
static uint8_t range_idx = 0;

uint8_t operacake_add_range(uint16_t freq_min, uint16_t freq_max, uint8_t port) {
	if(range_idx >= MAX_OPERACAKE_RANGES) {
		return 1;
	}
	ranges[range_idx].freq_min = freq_min;
	ranges[range_idx].freq_max = freq_max;
	ranges[range_idx].portA = port;
	ranges[range_idx].portB = 7;
	if(port <= OPERACAKE_PA4) {
		ranges[range_idx].portB = range_idx+4;
	} else {
		ranges[range_idx].portB = OPERACAKE_PA1;
	}
	range_idx++;
	return 0;
}

#define FREQ_ONE_MHZ (1000000ull)
static uint8_t current_range = 0xFF;

uint8_t operacake_set_range(uint32_t freq_mhz) {
	if(range_idx == 0) {
		return 1;
	}
	int i;
	for(i=0; i<range_idx; i++) {
		if((freq_mhz >= ranges[i].freq_min) 
		  && (freq_mhz <= ranges[i].freq_max)) {
			break;
		}
	}
	if(i == current_range) {
		return 1;
	}
	
	operacake_set_ports(operacake_boards[0], ranges[i].portA, ranges[i].portB);
	current_range = i;
	return 0;
}
