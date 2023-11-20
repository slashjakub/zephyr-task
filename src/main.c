#include "led.c"
#include "button.c"
#include "message_queue.c"

int main(void) {
  start_led_shell();
  start_button();
  start_message_linked_list();
}
