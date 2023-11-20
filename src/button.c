#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SLEEP_TIME_MS	1
#define LED1_NODE DT_ALIAS(led0)

const struct gpio_dt_spec led1_gpio = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

struct button_struct {
  const struct gpio_dt_spec button_gpio;
  struct gpio_callback cb_data;
};

static struct button_struct static_button_struct = {
  .button_gpio = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0}),
};

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
  printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());

  int val = gpio_pin_get_dt(&(static_button_struct.button_gpio));

  if (val >= 0) {
    gpio_pin_toggle_dt(&led1_gpio);
  }
}

void start_button() {
  int ret;
	
  if (!gpio_is_ready_dt(&(static_button_struct.button_gpio))) {
    printk("Error: button device %s is not ready\n",
	   static_button_struct.button_gpio.port->name);
    return;
  }

  ret = gpio_pin_configure_dt(&(static_button_struct.button_gpio), GPIO_INPUT);
  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n",
	   ret, static_button_struct.button_gpio.port->name, static_button_struct.button_gpio.pin);
    return;
  }

  ret = gpio_pin_interrupt_configure_dt(&(static_button_struct.button_gpio),
					GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n",
	   ret, static_button_struct.button_gpio.port->name, static_button_struct.button_gpio.pin);
    return;
  }

  gpio_init_callback(&(static_button_struct.cb_data), button_pressed, BIT(static_button_struct.button_gpio.pin));
  gpio_add_callback(static_button_struct.button_gpio.port, &(static_button_struct.cb_data));
  printk("Set up button at %s pin %d\n", static_button_struct.button_gpio.port->name, static_button_struct.button_gpio.pin);
}
