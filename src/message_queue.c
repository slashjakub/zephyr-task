#include "zephyr/kernel.h"
#include <zephyr/sys/slist.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#define STATIC_MESSAGE "staticka"

struct static_message {
  uint8_t text[16];
} __attribute__((aligned(4)));

K_MSGQ_DEFINE(my_message_queue, sizeof(struct static_message), CONFIG_MESSAGE_QUEUE_SIZE, 4);

static void send_static_message_command(const struct shell *sh, size_t argc, char **argv) {
  struct static_message message;

  strncpy(message.text, argv[1], strnlen(argv[1], 15) + 1);

  if (k_msgq_put(&my_message_queue, &message, K_NO_WAIT) == 0)
    shell_print(sh, "Static message sent: %s\n", message.text);
  else
    printk("Message could not be sent\n");
}

static void receive_static_message_command(const struct shell *sh) {
  struct static_message message;
  if (k_msgq_get(&my_message_queue, &message, K_NO_WAIT) == 0) {
    shell_print(sh, "Static message received: %s\n", message.text);
  } else {
    printk("Message could not be received\n");
  }
}

struct dynamic_message {
  void *fifo_reserved;
  uint8_t* text;
};

K_FIFO_DEFINE(my_fifo);

static void send_dynamic_message_command(const struct shell *sh, size_t argc, char **argv) {
  struct dynamic_message *message;

  message = malloc(sizeof(struct dynamic_message));
  message->text = malloc(strnlen(argv[1] + 1, 15));
  strncpy(message->text, argv[1], strnlen(argv[1], 15) + 1);

  k_fifo_put(&my_fifo, message);
  shell_print(sh, "Dynamic message sent: %s\n", message->text);
}

static void receive_dynamic_message_command(const struct shell *sh) {
  struct dynamic_message *message = k_fifo_get(&my_fifo, K_NO_WAIT);

  if (message) {
    shell_print(sh, "Dynamic message received: %s\n", message->text);
    free(message->text);
    free(message);
  } else {
    shell_print(sh, "FIFO empty\n");
  }
}

struct linked_list_message {
  sys_snode_t snode;
  uint8_t* text;
};

static sys_slist_t my_linked_list;

static void start_message_linked_list() {
  sys_slist_init(&my_linked_list);
}

static void send_linked_list_message_command(const struct shell *sh, size_t argc, char **argv) {
  struct linked_list_message *message;

  message = malloc(sizeof(struct linked_list_message));
  /* message->snode = malloc(sizeof(sys_snode_t)); */
  message->text = malloc(strnlen(argv[1], 15) + 1);
  strncpy(message->text, argv[1], strnlen(argv[1], 15) + 1);

  sys_slist_append(&my_linked_list, &message->snode);
  shell_print(sh, "Linked list message sent: %s\n", message->text);
}

static void receive_linked_list_message_command(const struct shell *sh) {
  struct linked_list_message *message = NULL;
  sys_snode_t *snode_found = sys_slist_peek_tail(&my_linked_list);

  message = SYS_SLIST_CONTAINER(snode_found, message, snode);

  if (message) {
    shell_print(sh, "Linked list message received: %s\n", message->text);
    sys_slist_find_and_remove(&my_linked_list, snode_found);
    free(message->text);
    free(message);
  } else {
    shell_print(sh, "Linked list empty\n");
  }
}

static void print_linked_list_message_command(const struct shell *sh) {
  struct linked_list_message *message = NULL;
  sys_snode_t *snode_found = NULL;
  shell_print(sh, "Linked list {");

  SYS_SLIST_FOR_EACH_NODE(&my_linked_list, snode_found) {
    message = SYS_SLIST_CONTAINER(snode_found, message, snode);
    shell_print(sh, "%s", message->text);
  };

  shell_print(sh, "}");
}

SHELL_STATIC_SUBCMD_SET_CREATE(message_static_command,
                               SHELL_CMD_ARG(send, NULL, "Send static message",
					     send_static_message_command, 2, 0),
                               SHELL_CMD(receive, NULL,
                                         "Receive static message",
                                         receive_static_message_command),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(message_dynamic_command,
                               SHELL_CMD_ARG(send, NULL, "Send dynamic message",
                                         send_dynamic_message_command, 2, 0),
                               SHELL_CMD(receive, NULL,
                                         "Receive dynamic message",
                                         receive_dynamic_message_command),
                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(message_linked_list_command,
                               SHELL_CMD_ARG(send, NULL, "Send linked list message",
                                         send_linked_list_message_command, 2, 0),
                               SHELL_CMD(receive, NULL,
                                         "Receive linked list message",
                                         receive_linked_list_message_command),
			       SHELL_CMD(print, NULL,
                                         "Receive linked list message",
                                         print_linked_list_message_command),

                               SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(message_command,
                               SHELL_CMD(static, &message_static_command,
                                         "Send and receive static messages",
                                         NULL),
                               SHELL_CMD(dynamic, &message_dynamic_command,
                                         "Send and receive static messages",
                                         NULL),
			       SHELL_CMD(linked_list, &message_linked_list_command,
                                         "Send and receive linked list messages",
                                         NULL),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(message, &message_command, "Message commands", NULL);
