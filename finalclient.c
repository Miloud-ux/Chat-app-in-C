#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE 1024
#define REPLY_SIZE 2048

void clean_buffer(char *buffer) {
  for (int i = 0; i < MAX_SIZE; i++) {
    buffer[i] = '\0';
  }
}

void *receive_server_data(void *arg) {
  int client_socket = *(int *)arg;
  char msg_received[REPLY_SIZE];
  ssize_t bytes_recv;

  while (1) {
    memset(msg_received, 0, sizeof(msg_received));
    bytes_recv = read(client_socket, msg_received, sizeof(msg_received) - 1);

    if (bytes_recv > 0) {
      msg_received[bytes_recv] = '\0';
      if (strcmp(msg_received, "Ok\n") == 0) {
        continue;
      }
      
      // wipe the current line so the prompt doesn't get messy
      printf("\r\033[K");
      printf("%s", msg_received);
      printf("You :");
      fflush(stdout);
    } else
      break;
  }

  if (bytes_recv == 0) {
    printf("server disconnected");
  } else {
    perror("Error while receiving server replies");
  }
  return NULL;
}

int main() {
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    perror("socket() failed");
    return 1;
  }

  const struct sockaddr_in server_info = {.sin_family = AF_INET,
                                          .sin_port = htons(2000),
                                          .sin_addr.s_addr =
                                              inet_addr("127.0.0.1")};

  if (connect(client_socket, (struct sockaddr *)&server_info,
              sizeof(server_info)) < 0) {
    perror("connect() failed");
    close(client_socket);
    return 1;
  }
  printf("Connected to server at 127.0.0.1:2000\n");

  char buffer[MAX_SIZE];
  char username[50];
  
  printf("Enter your username \n");
  fgets(username, sizeof(username), stdin);
  username[strcspn(username, "\n")] = '\0';

  if (write(client_socket, username, strlen(username)) <= 0) {
    perror("Writing username failed");
    close(client_socket);
    return 1;
  }

  // spin up a thread to handle incoming messages
  pthread_t recv_thread;
  if (pthread_create(&recv_thread, NULL, receive_server_data,
                     (void *)&client_socket) < 0) {
    perror("error creating the thread");
    return 1;
  }

  while (1) {
    printf("You :");
    if (!fgets(buffer, MAX_SIZE, stdin)) {
      printf("\nConnection closed by client.\n");
      break;
    }

    if (strcmp(buffer, ":end\n") == 0) {
      write(client_socket, "end", 3);
      printf("Exiting...\n");
      break;
    }

    if (write(client_socket, buffer, strlen(buffer)) <= 0) {
      perror("write() failed");
      break;
    }
  }

  close(client_socket);
  return 0;
}
