#include <arpa/inet.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "helpers.h"

#define MAX_CLIENTS 10
#define PORT 8080

typedef struct {
  int client_socket;
  char username[50];
  bool is_active;
} client;

// locks
pthread_mutex_t server_clock_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t server_clock = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static client client_list[MAX_CLIENTS];

void client_init(client client_list[]) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    client_list[i].client_socket = 0;
    client_list[i].is_active = false;
    memset(client_list[i].username, 0, sizeof(client_list[i].username));
  }
}

void empty_client_fds(client client_list[], client my_client) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_list[i].client_socket == my_client.client_socket) {
      client_list[i].client_socket = 0;
      client_list[i].is_active = false;
      memset(client_list[i].username, 0, sizeof(client_list[i].username));
      break;
    }
  }
}

void msg_formatter(char new_msg[], size_t new_msg_size, uint64_t lc,
                   const char timestamp[], const char msg[],
                   const char username[]) {
  snprintf(new_msg, new_msg_size,
           "[LC:%" PRIu64 "] [\033[1;34m%s\033[0m] \033[1;35m%s\033[0m: %s\n",
           lc, timestamp, username, msg);
}

void *handle_client(void *arg) {
  client my_client = *(client *)arg;
  free(arg);
  char buffer[1024];
  char username[50];

  // Receive username from client
  if (read_full(my_client.client_socket, username, sizeof(username)) != 0) {
    pthread_mutex_lock(&mutex);
    empty_client_fds(client_list, my_client);
    pthread_mutex_unlock(&mutex);
    return NULL;
  }
  username[sizeof(username) - 1] = '\0';
  // Save client's username
  pthread_mutex_lock(&mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_list[i].client_socket == my_client.client_socket) {
      strncpy(client_list[i].username, username,
              sizeof(client_list[i].username) - 1);
      client_list[i].username[sizeof(client_list[i].username) - 1] = '\0';
      break;
    }
  }
  pthread_mutex_unlock(&mutex);

  printf("\033[1;32m%s has connected\033[0m\n", username);
  bool break_outer_loop = false;
  ssize_t bytes_received;
  while (1) {
    bytes_received =
        recv(my_client.client_socket, buffer, sizeof(buffer) - 1, 0);

    // Error checking
    if (bytes_received <= 0) {
      printf("\033[1;31m%s disconnected\033[0m\n", username);
      break;
    }
    buffer[bytes_received] = '\0';

    // Timestamps
    struct tm *ptr;
    time_t t = time(NULL);
    ptr = localtime(&t);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", ptr);
    char *msg;
    char *end;
    msg = buffer;

    /* split messages by their delimiter ('\n') since TCP is a streaming
     * protocole meaning the buffer could fit Multiple messages at once and we
     * need to split them.
     */

    while ((end = strchr(msg, '\n'))) {
      *end = '\0';
      int len = strlen(msg);
      if (len > 0 && msg[len - 1] == '\r') {
        msg[len - 1] = '\0';
      }
      // printf("DEBUG: Received message: '%s'\n", msg);
      if (strcmp(msg, ":end") == 0) {
        printf("\033[1;31m%s disconnected\033[0m\n", username);
        fflush(stdout);
        break_outer_loop = true;
        break;
      } else if (strcmp(msg, ":online") == 0) {
        printf("User %s requested online users list\n", username);
        pthread_mutex_lock(&mutex);
        char online_users[2048] = "\n=== Online Users ===\n";
        int user_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (client_list[i].is_active && strlen(client_list[i].username) > 0) {
            user_count++;
            char user_line[100];
            snprintf(user_line, sizeof(user_line), "%d. %s\n", user_count,
                     client_list[i].username);
            strcat(online_users, user_line);
          }
        }
        char footer[100];
        snprintf(footer, sizeof(footer),
                 "==================\nTotal: %d users online\n", user_count);
        strcat(online_users, footer);
        pthread_mutex_unlock(&mutex);

        write_full(my_client.client_socket, online_users, strlen(online_users));
        msg = end + 1;
        continue;
      } else {
        uint64_t incoming_lc = 0;
        char actual_msg[1024] = {0};
        uint64_t broadcast_lc = 0;

        // parse lamport timestamps lc|msg
        if (sscanf(msg, "%" SCNu64 "|%1023[^\n]", &incoming_lc, actual_msg) ==
            2) {
          // l_serv = max(l_client, l_server) + 1
          pthread_mutex_lock(&server_clock_mutex);
          server_clock = MAX(incoming_lc, server_clock) + 1;
          broadcast_lc = server_clock;
          pthread_mutex_unlock(&server_clock_mutex);
        } else {
          // if a client sends without prefix lc (netcat)
          pthread_mutex_lock(&server_clock_mutex);
          server_clock++;
          broadcast_lc = server_clock;
          pthread_mutex_unlock(&server_clock_mutex);

          strncpy(actual_msg, msg, sizeof(actual_msg) - 1);
          actual_msg[sizeof(actual_msg) - 1] = '\0';
        }

        // 2. Format with LC header and ANSI colors
        char formatted_msg[1200];
        msg_formatter(formatted_msg, sizeof(formatted_msg), broadcast_lc,
                      timestamp, actual_msg, username);

        // 3. Broadcast to all active peers
        pthread_mutex_lock(&mutex);
        int target_sockets[MAX_CLIENTS];
        int target_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (client_list[i].is_active &&
              client_list[i].client_socket != my_client.client_socket) {
            target_sockets[target_count++] = client_list[i].client_socket;
          }
        }
        pthread_mutex_unlock(&mutex);

        for (int i = 0; i < target_count; i++) {
          write_full(target_sockets[i], formatted_msg, strlen(formatted_msg));
        }
        printf("%s", formatted_msg);
      }
      msg = end + 1;
    }
    if (break_outer_loop) {
      break;
    }
  }
  close(my_client.client_socket);
  pthread_mutex_lock(&mutex);
  empty_client_fds(client_list, my_client);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(void) {
  signal(SIGPIPE,
         SIG_IGN); // ignore broken pipe signals when clients disconnect and we
                   // try to send them a message
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    die("socket failed");
  }
  int opt = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    die("bind failed");
  }
  if (listen(server_socket, MAX_CLIENTS) < 0) {
    die("listen failed");
  }
  printf("server listening on port %d\n", PORT);
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
      perror("accept failed");
      continue;
    }
    pthread_mutex_lock(&mutex);
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!client_list[i].is_active) {
        slot = i;
        break;
      }
    }
    if (slot == -1) {
      pthread_mutex_unlock(&mutex);
      printf("max clients reached, rejecting connection\n");
      close(client_socket);
      continue;
    }
    client_list[slot].client_socket = client_socket;
    client_list[slot].is_active = true;
    client *arg = malloc(sizeof(client));
    *arg = client_list[slot];
    pthread_mutex_unlock(&mutex);
    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, arg) != 0) {
      perror("pthread_create failed");
      close(client_socket);
      pthread_mutex_lock(&mutex);
      client_list[slot].is_active = false;
      pthread_mutex_unlock(&mutex);
      free(arg);
      continue;
    }
    pthread_detach(tid);
  }
  pthread_mutex_destroy(&mutex);
  close(server_socket);
  return 0;
}
