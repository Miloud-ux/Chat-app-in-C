#include "helpers.h"
#include <arpa/inet.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define REPLY_SIZE 2048
#define PORT 8080

pthread_mutex_t client_clock_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t client_clock = 0;

void *receive_server_data(void *arg) {
  int client_socket = *(int *)arg;
  char msg_received[REPLY_SIZE];
  ssize_t bytes_recv;
  while (1) {
    memset(msg_received, 0, sizeof(msg_received));
    bytes_recv = read(client_socket, msg_received, sizeof(msg_received) - 1);
    if (bytes_recv > 0) {
      msg_received[bytes_recv] = '\0';

      // Get the max clock
      uint64_t server_lc = 0;
      if (sscanf(msg_received, "[LC:%" SCNu64 "]", &server_lc) == 1) {
        pthread_mutex_lock(&client_clock_mutex);
        client_clock = MAX(server_lc, client_clock) + 1;
        pthread_mutex_unlock(&client_clock_mutex);
      }

      printf("\r\033[K");
      printf("%s", msg_received);
      printf("You :");
      fflush(stdout);
    } else
      break;
  }
  if (bytes_recv == 0) {
    printf("server disconnected\n");
  } else {
    perror("Error while receiving server replies");
  }
  return NULL;
}

int main(void) {
  signal(SIGPIPE, SIG_IGN);
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    die("socket() failed");
  }
  const struct sockaddr_in server_info = {.sin_family = AF_INET,
                                          .sin_port = htons(PORT),
                                          .sin_addr.s_addr =
                                              inet_addr("127.0.0.1")};
  if (connect(client_socket, (struct sockaddr *)&server_info,
              sizeof(server_info)) < 0) {
    die("connect() failed");
  }
  printf("Connected to server at 127.0.0.1:%d\n", PORT);
  char buffer[MAX_SIZE];
  char username[50];
  printf("Enter your username \n");
  fgets(username, sizeof(username), stdin);
  username[strcspn(username, "\n")] = '\0';
  memset(username + strlen(username), 0, sizeof(username) - strlen(username));
  if (write_full(client_socket, username, sizeof(username)) != 0) {
    die("Writing username failed");
  }
  pthread_t recv_thread;
  if (pthread_create(&recv_thread, NULL, receive_server_data,
                     (void *)&client_socket) != 0) {
    die("error creating the thread");
  }

  printf("You :");
  fflush(stdout);
  while (1) {
    if (!fgets(buffer, MAX_SIZE, stdin)) {
      printf("\nConnection closed by client.\n");
      break;
    }
    if (strcmp(buffer, ":end\n") == 0) {
      write_full(client_socket, ":end\n", 5);
      printf("Exiting...\n");
      break;
    }

    if (strcmp(buffer, ":online\n") == 0) {
      write_full(client_socket, ":online\n", 8);
      continue;
    }

    pthread_mutex_lock(&client_clock_mutex);
    client_clock++;
    uint64_t lc = client_clock;
    pthread_mutex_unlock(&client_clock_mutex);

    char payload[MAX_SIZE + 64];
    snprintf(payload, sizeof(payload), "%" PRIu64 "|%s", lc, buffer);

    if (write_full(client_socket, payload, strlen(payload)) != 0) {
      perror("write() failed");
      break;
    }
    printf("You :");
    fflush(stdout);
  }
  shutdown(client_socket, SHUT_WR);
  pthread_join(recv_thread, NULL);
  close(client_socket);
  return 0;
}
