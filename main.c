#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>

#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "string.h"

size_t MAX_CHILDREN;
size_t MAX_THREADS;
int child_count_g = 0;
int job_count_g = 0;
pthread_mutex_t job_mutex;
pthread_mutex_t backup_mutex;
char *dirpath_g = NULL;
int *fd_s = NULL;

int main(int argc, char* argv[]) {
  if (argc != 4) return 1;
  DIR *dirp = NULL;
  // Iniciar mutex
  pthread_mutex_init(&job_mutex, NULL);
  pthread_mutex_init(&backup_mutex, NULL);
  // Obter ficheiros (fd_s)
  size_t strl = strlen(argv[1]) + 2;
  dirpath_g = malloc(strl);
  snprintf(dirpath_g, strl, "%s", argv[1]);
  dirp = open_dir(dirpath_g);
  fd_s = read_files_in_directory(dirp, dirpath_g, &job_count_g); 
  // Resto dos argumentos
  sscanf(argv[2], "%ld", &MAX_CHILDREN);
  sscanf(argv[3], "%ld", &MAX_THREADS);
  // Nao criar threads em excesso
  if (job_count_g < (int) MAX_THREADS) MAX_THREADS = (size_t) job_count_g;

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }
  // Iniciar threads
  pthread_t threads[MAX_THREADS];
  for (int i = 0; i < (int) MAX_THREADS; i++) {
    if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
      perror("Failed to create thread\n");
      return 1;
    }
  }

  for (int i = 0; i < (int)MAX_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  free(dirpath_g);
  close_files(dirp, fd_s, job_count_g);
  kvs_terminate();
  return 0;
}
