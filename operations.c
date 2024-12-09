#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "kvs.h"
#include "constants.h"

#define MAX_FILES 100

static struct HashTable* kvs_table = NULL;
char *filename = NULL;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

int kvs_init() {
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }

  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }

  return 0;
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  printf("[");
  for (size_t i = 0; i < num_pairs; i++) {
    char* result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
      printf("(%s,KVSERROR)", keys[i]);
    } else {
      printf("(%s,%s)", keys[i], result);
    }
    free(result);
  }
  printf("]\n");
  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        printf("[");
        aux = 1;
      }
      printf("(%s,KVSMISSING)", keys[i]);
    }
  }
  if (aux) {
    printf("]\n");
  }

  return 0;
}

void kvs_show() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      printf("(%s, %s)\n", keyNode->key, keyNode->value);
      keyNode = keyNode->next; // Move to the next node
    }
  }
}

int kvs_backup(char *dirpath) {
  //usar FORK
  //usar wait para fazer com que algum processo acabe
  int fd;
  char count_str[20];
  filename[strlen(filename) - 4] = '\0';

  snprintf(count_str, sizeof(count_str), "%d", 1);

  size_t bck_filename_len = strlen(dirpath) + strlen(filename) + strlen("-") + strlen(count_str) + 5 /*4(.bck) + 1("/0")*/;
  char *bck_filename = malloc(bck_filename_len);

  sprintf(bck_filename, "%s%s-%s.bck", dirpath, filename, count_str);

  fd = open(bck_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("Erro ao abrir arquivo");
  }

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      char *value = keyNode->value;
      char *key = keyNode->key;
      size_t bck_len = strlen(value) + strlen(key) + 6;
      char *bck = malloc(bck_len);

      sprintf(bck, "(%s, %s)\n", key, value);

      if (write(fd, bck, bck_len) == -1) {
        perror("Erro ao escrever no arquivo");
      }

      free(bck);
      keyNode = keyNode->next; 
    }
  }
  free(bck_filename);
  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}


DIR *open_dir(const char *dirpath) {
  return opendir(dirpath);
}

int *read_files_in_directory(DIR *dirp, const char *dirpath, int *count) {
  struct dirent *dp;
  static int fds[MAX_FILES]; 
  int size = 0;

  for (;;) {
    dp = readdir(dirp);
    if (dp == NULL)
      break;

    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
      continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dp->d_name);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
      perror("Erro ao abrir arquivo");
      continue;
    }
    fds[size++] = fd;
  }

  *count = size; // Retorna o número de fds armazenados
  return fds;
}

void close_files(DIR *dirp, int* fds, int count) {
  for (int i = 0; i < count; i++) {
    close(fds[i]);
  }
  closedir(dirp);
  free(filename);
}