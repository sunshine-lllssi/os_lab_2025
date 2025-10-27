#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "common.h"

uint64_t global_result = 1;
uint64_t global_mod = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};


void* ProcessServer(void* args) {
  struct ThreadArgs* thread_args = (struct ThreadArgs*)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(thread_args->server.port);
  
  if (hostname->h_addr_list[0] != NULL) {
    memcpy(&server.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
  } else {
    fprintf(stderr, "No address found for %s\n", thread_args->server.ip);
    return NULL;
  }

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  int bytes_received = recv(sck, response, sizeof(response), 0);
  if (bytes_received < 0) {
    fprintf(stderr, "Receive failed from %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    return NULL;
  }

  if (bytes_received != sizeof(uint64_t)) {
    fprintf(stderr, "Invalid response size from %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    return NULL;
  }

  uint64_t partial_result = 0;
  memcpy(&partial_result, response, sizeof(uint64_t));
  
  printf("Received partial result from %s:%d: %llu (range %llu-%llu)\n",
         thread_args->server.ip, thread_args->server.port,
         (unsigned long long)partial_result,
         (unsigned long long)thread_args->begin,
         (unsigned long long)thread_args->end);

  pthread_mutex_lock(&result_mutex);
  global_result = MultModulo(global_result, partial_result, global_mod);
  pthread_mutex_unlock(&result_mutex);

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  char servers[255] = {'\0'};

  while (true) {
    optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        global_mod = mod;
        break;
      case 2:
        strncpy(servers, optarg, sizeof(servers) - 1);
        servers[sizeof(servers) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == 0 || mod == 0 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *servers_file = fopen(servers, "r");
  if (!servers_file) {
    perror("Failed to open servers file");
    return 1;
  }

  int servers_num = 0;
  struct Server *to = NULL;
  char line[256];

  while (fgets(line, sizeof(line), servers_file)) {
    line[strcspn(line, "\n")] = 0;
    
    if (line[0] == '\0' || line[0] == '#') continue;

    char *colon = strchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Invalid server format: %s\n", line);
      continue;
    }

    *colon = '\0';
    to = realloc(to, (servers_num + 1) * sizeof(struct Server));
    
    strncpy(to[servers_num].ip, line, sizeof(to[servers_num].ip) - 1);
    to[servers_num].ip[sizeof(to[servers_num].ip) - 1] = '\0';
    to[servers_num].port = atoi(colon + 1);
    
    servers_num++;
  }
  fclose(servers_file);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in %s\n", servers);
    free(to);
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  if ((uint64_t)servers_num > k) {
    servers_num = (int)k;
  }


  uint64_t chunk_size = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current = 1;

  pthread_t *threads = malloc(servers_num * sizeof(pthread_t));
  struct ThreadArgs *thread_args = malloc(servers_num * sizeof(struct ThreadArgs));

  for (int i = 0; i < servers_num; i++) {
    thread_args[i].server = to[i];
    thread_args[i].begin = current;
    thread_args[i].end = current + chunk_size - 1;
    thread_args[i].mod = mod;


    if ((uint64_t)i < remainder) {
      thread_args[i].end++;
    }

    if (thread_args[i].end > k) {
      thread_args[i].end = k;
    }

    printf("Server %d (%s:%d) will compute range %llu-%llu\n",
           i, to[i].ip, to[i].port,
           (unsigned long long)thread_args[i].begin,
           (unsigned long long)thread_args[i].end);

    current = thread_args[i].end + 1;

    if (pthread_create(&threads[i], NULL, ProcessServer, &thread_args[i]) != 0) {
      perror("Failed to create thread");
      free(threads);
      free(thread_args);
      free(to);
      return 1;
    }
  }

  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Final result: %llu! mod %llu = %llu\n", 
         (unsigned long long)k, 
         (unsigned long long)mod, 
         (unsigned long long)global_result);

  free(threads);
  free(thread_args);
  free(to);

  return 0;
}