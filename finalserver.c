#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define PORT 8080

typedef struct {
  int client_socket;
  char username[50];
  bool is_active;
} client;

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

void msg_formatter(char new_msg[], size_t new_msg_size, const char timestamp[],
                   const char msg[], const char username[]) {
  snprintf(new_msg, new_msg_size,
           "[\033[1;34m%s\033[0m] \033[1;35m%s\033[0m: %s\n", timestamp,
           username, msg);
}

void *handle_client(void *arg) {
  client my_client = *(client *)arg;
  free(arg);
  char buffer[1024];
  char username[50];
  ssize_t bytes_received;
  bytes_received =
      recv(my_client.client_socket, username, sizeof(username) - 1, 0);
  if (bytes_received <= 0) {
    perror("error receiving the message");
    empty_client_fds(client_list, my_client);
    return NULL;
  }
  username[bytes_received] = '\0';
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_list[i].client_socket == my_client.client_socket) {
      strncpy(client_list[i].username, username,
              sizeof(client_list[i].username) - 1);
      client_list[i].username[sizeof(client_list[i].username) - 1] = '\0';
      break;
    }
  }
  printf("\033[1;32m%s has connected\033[0m\n", username);
  bool break_outer_loop = false;
  while (1) {
    bytes_received =
        recv(my_client.client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
      printf("\033[1;31m%s disconnected\033[0m\n", username);
      break;
    }
    buffer[bytes_received] = '\0';
    struct tm *ptr;
    time_t t = time(NULL);
    ptr = localtime(&t);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", ptr);
    char *msg;
    char *end;
    msg = buffer;
    while ((end = strchr(msg, '\n'))) {
      *end = '\0';
      int len = strlen(msg);
      if (len > 0 && msg[len - 1] == '\r') {
        msg[len - 1] = '\0';
      }
      printf("DEBUG: Received message: '%s'\n", msg);
      if (strcmp(msg, ":end") == 0) {
        printf("\033[1;31m%s disconnected\033[0m\n", username);
        fflush(stdout);
        break_outer_loop = true;
        break;
      } else if (strcmp(msg, ":online") == 0) {
        printf("=== ONLINE COMMAND DETECTED ===\n");
        printf("User %s requested online users list\n", username);
        char online_users[2048] = "\n=== Online Users ===\n";
        int user_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (client_list[i].is_active && strlen(client_list[i].username) > 0) {
            user_count++;
            char user_line[100];
            snprintf(user_line, sizeof(user_line), "%d. %s\n", user_count,
                     client_list[i].username);
            strcat(online_users, user_line);
            printf("Added user: %s\n", client_list[i].username);
          }
        }
        char footer[100];
        snprintf(footer, sizeof(footer),
                 "==================\nTotal: %d users online\n", user_count);
        strcat(online_users, footer);
        send(my_client.client_socket, online_users, strlen(online_users), 0);
        msg = end + 1;
        continue;
      } else {
        char formatted_msg[1200];
        msg_formatter(formatted_msg, sizeof(formatted_msg), timestamp, msg,
                      username);
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (client_list[i].is_active &&
              client_list[i].client_socket != my_client.client_socket) {
            send(client_list[i].client_socket, formatted_msg,
                 strlen(formatted_msg), 0);
          }
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
  empty_client_fds(client_list, my_client);
  return NULL;
}

int main(void) {
  client_init(client_list);
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
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
    perror("bind failed");
    close(server_socket);
    exit(EXIT_FAILURE);
  }
  if (listen(server_socket, MAX_CLIENTS) < 0) {
    perror("listen failed");
    close(server_socket);
    exit(EXIT_FAILURE);
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
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!client_list[i].is_active) {
        slot = i;
        break;
      }
    }
    if (slot == -1) {
      printf("max clients reached, rejecting connection\n");
      close(client_socket);
      continue;
    }
    client_list[slot].client_socket = client_socket;
    client_list[slot].is_active = true;
    client *arg = malloc(sizeof(client));
    *arg = client_list[slot];
    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, arg) != 0) {
      perror("pthread_create failed");
      close(client_socket);
      client_list[slot].is_active = false;
      free(arg);
      continue;
    }
    pthread_detach(tid);
  }
  close(server_socket);
  return 0;
}
