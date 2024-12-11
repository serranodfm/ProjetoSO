#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/wait.h>

#include "constants.h"
#include "parser.h"
#include "operations.h"
#include "string.h"

int main(int argc, char* argv[]) {
  int fd, file = 0, index = 0, *fds = NULL, count = 0, bck_count = 1;
  size_t MAX_CHILDREN;
  size_t MAX_THREADS;
  int child_count = 0;
  DIR *dirp = NULL;
  char *dirpath = NULL;

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }
  //escolher entre ficheiros ou terminal
  if (argc == 1) {
    fd = STDIN_FILENO;
  } else {
    file = 1;
    dirp = open_dir(argv[1]);
    fds = read_files_in_directory(dirp, argv[1], &count); 

    dirpath = malloc(strlen(argv[1]) + 1);
    strcpy(dirpath, argv[1]);
    sscanf(argv[2], "%ld", &MAX_CHILDREN);
    sscanf(argv[3], "%ld", &MAX_THREADS);
  }

  while (1) {
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
        if (num_pairs == 0 && !file) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_write(num_pairs, keys, values)) {
          fprintf(stderr, "Failed to write pair\n");
        }

        break;

      case CMD_READ:
        num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0 && !file) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_read(num_pairs, keys)) {
          fprintf(stderr, "Failed to read pair\n");
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0 && !file) {
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
        if (parse_wait(fd, &delay, NULL) == -1 && !file) {
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
        if (!file) {fprintf(stderr, "Invalid command. See HELP for usage\n");}
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
          printf("teste %d\n", index+1);
          kvs_clean();
          index++;
          break;
        } 
        free(dirpath);
        close_files(dirp, fds, count);
        kvs_terminate();
        return 0;
    }
  }
}
