#include "zephyr/device.h"
#include "zephyr/kernel.h"
#include "zephyr/shell/shell_fprintf.h"
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>

#define ON_TIME 1600
#define OFF_TIME 1600

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

struct led_struct {
    const struct gpio_dt_spec led_gpio;
  int on_time;
  int off_time;
};

static struct led_struct static_led_struct = {
    .led_gpio = GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    .on_time = ON_TIME,
    .off_time = OFF_TIME,
};

static struct k_work_delayable led_on_work;
static struct k_work_delayable led_off_work;
static struct k_work_delayable led_blinking_on_work;
static struct k_work_delayable led_blinking_off_work;

static void led_on_work_handler(struct k_work *work) {
  gpio_pin_set_dt(&(static_led_struct.led_gpio), 1);
}
static void led_off_work_handler(struct k_work *work) {
  gpio_pin_set_dt(&(static_led_struct.led_gpio), 0);
}

static void led_blinking_on_work_handler(struct k_work *work) {
  k_work_schedule(&led_on_work, K_NO_WAIT);
  k_work_schedule(&led_off_work, K_MSEC(static_led_struct.on_time));
  k_work_schedule(&led_blinking_on_work, K_MSEC(static_led_struct.on_time +
                                                static_led_struct.off_time));
}

static void led_blinking_off_work_handler(struct k_work *work) {
  k_work_cancel_delayable(&led_blinking_on_work);
  k_work_schedule(&led_off_work, K_NO_WAIT);
}

static void led_on_command(const struct shell *sh) {
  k_work_schedule(&led_on_work, K_NO_WAIT);
  shell_print(sh, "turned led on\n");
}

static void led_off_command(const struct shell *sh) {
  k_work_schedule(&led_off_work, K_NO_WAIT);
  shell_print(sh, "turned led off\n");
}

static void led_blinking_on_command(const struct shell *sh, size_t argc,
                                    char **argv) {
  int on_time = -1;
  int off_time = -1;

  if (argv[1])
    on_time = atoi(argv[1]);
  if (argv[2])
    off_time = atoi(argv[2]);

  if (on_time <= 0)
    on_time = ON_TIME;
  if (off_time <= 0)
    off_time = OFF_TIME;

  static_led_struct.on_time = on_time;
  static_led_struct.off_time = off_time;
  k_work_schedule(&led_blinking_on_work, K_NO_WAIT);
  shell_print(sh, "turned led blinking on\n");
}

static void led_blinking_off_command(const struct shell *sh) {
  k_work_schedule(&led_blinking_off_work, K_NO_WAIT);
  shell_print(sh, "turned led blinking off\n");
}

SHELL_STATIC_SUBCMD_SET_CREATE(led_command_blinking,
                               SHELL_CMD_ARG(on, NULL, "Turn led blinking on",
                                             led_blinking_on_command, 3, 0),
                               SHELL_CMD(off, NULL, "Turn led blinking off",
                                         led_blinking_off_command),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(led_command,
                               SHELL_CMD(on, NULL, "Led on", led_on_command),
                               SHELL_CMD(off, NULL, "Led off", led_off_command),
                               SHELL_CMD(blinking, &led_command_blinking,
                                         "Control led blinking", NULL),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led, &led_command, "LED commands", NULL);

void start_led_shell() {
  k_work_init_delayable(&led_on_work, led_on_work_handler);
  k_work_init_delayable(&led_off_work, led_off_work_handler);
  k_work_init_delayable(&led_blinking_on_work, led_blinking_on_work_handler);
  k_work_init_delayable(&led_blinking_off_work, led_blinking_off_work_handler);

  if (!gpio_is_ready_dt(&(static_led_struct.led_gpio))) {
    return;
  }

  int ret =
      gpio_pin_configure_dt(&(static_led_struct.led_gpio), GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return;
  }

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
  const struct device *dev;
  uint32_t dtr = 0;

  dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
  if (!device_is_ready(dev) || usb_enable(NULL)) {
    return;
  }

  while (!dtr) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    k_sleep(K_MSEC(100));
  }
#endif
}
