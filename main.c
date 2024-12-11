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
int child_count = 0;
pthread_mutex_t job_mutex;
pthread_mutex_t backup_mutex;
char *dirpath = NULL;
int *fds = NULL;

int main(int argc, char* argv[]) {
  int fd, index = 0, count = 0, bck_count = 1;
  DIR *dirp = NULL;
  pthread_mutex_t job_mutex;
  pthread_mutex_init(&job_mutex, NULL);
  pthread_mutex_init(&backup_mutex, NULL);

  size_t strl = strlen(argv[1]) + 2;
  dirpath = malloc(strl);
  snprintf(dirpath, strl, "%s/", argv[1]);
  dirp = open_dir(dirpath);
  fds = read_files_in_directory(dirp, dirpath, &count); 

  sscanf(argv[2], "%ld", &MAX_CHILDREN);
  sscanf(argv[3], "%ld", &MAX_THREADS);

  if (count < MAX_THREADS) MAX_THREADS = count;

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  pthread_t threads[MAX_THREADS];
  for (int i = 0; i < MAX_THREADS; i++) {
    if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
      perror("Erro ao criar thread");
      return 1;
    }
  }

  for (int i = 0; i < MAX_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  free(dirpath);
  close_files(dirp, fds, count);
  kvs_terminate();
  return 0;


  /*while (1) {
    if (file) {
      fd = fds[index];
    }
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;

    if (!file) {printf("> ");}  
    fflush(stdout);

    switch (get_next(fd)) {
      case CMD_WRITE:
        num_pairs = parse_write(fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_write(num_pairs, keys, values)) {
          fprintf(stderr, "Failed to write pair\n");
        }

        break;

      case CMD_READ:
        num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_read(num_pairs, keys)) {
          fprintf(stderr, "Failed to read pair\n");
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_delete(num_pairs, keys)) {
          fprintf(stderr, "Failed to delete pair\n");
        }
        break;

      case CMD_SHOW:
        kvs_show();
        break;

      case CMD_WAIT:
        if (parse_wait(fd, &delay, NULL) == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          kvs_wait(delay);
        }
        break;

      case CMD_BACKUP:
        while (1) {
          if (child_count < (int)MAX_CHILDREN) {
            pid_t pid = fork();
            if (pid == 0) {
              if (kvs_backup(dirpath, bck_count)) {
                fprintf(stderr, "Failed to perform backup.\n");
              }
              exit(0);
            } else {
              bck_count++;
              child_count++;
              break;
            }
          } 
          else {
            wait(NULL);
            child_count--;
          }
        }
        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf( 
            "Available commands:\n"
            "  WRITE [(key,value)(key2,value2),...]\n"
            "  READ [key,key2,...]\n"
            "  DELETE [key,key2,...]\n"
            "  SHOW\n"
            "  WAIT <delay_ms>\n"
            "  BACKUP\n" 
            "  HELP\n"
        );

        break;
        
      case CMD_EMPTY:
        break;

      case EOC:
        if (index < count-1) {
          index++;
          new_index(index);
          kvs_next();
          break;
        } 
        free(dirpath);
        close_files(dirp, fds, count);
        kvs_terminate();
        return 0;
    }
  }*/
}
