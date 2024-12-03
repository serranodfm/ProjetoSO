#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "kvs.h"
#include "constants.h"

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

int kvs_backup() {
  //usar FORK
  //usar wait para fazer com que algum processo acabe
  //int fd;
  char count_str[20];
  snprintf(count_str, sizeof(count_str), "%d", 1);
  size_t bck_filename_len = strlen(filename) + strlen("-") + strlen(count_str) + 1;
  char *bck_filename = malloc(bck_filename_len);
  strcpy(bck_filename, filename);
  strcat(bck_filename, "-");
  strcat(bck_filename, count_str);
  //printf("%s", bck_filename);
  free(bck_filename);
  //const char *nome_ficheiro = ".bck";
  //open(filename)
  /*for (valores hashtable)
    obter valores
    guardar valores em nomefich-count.bck
  fechar
  */ 
  
  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}


DIR *open_dir(const char *dirpath) {
  return opendir(dirpath);
}

int read_files_in_directory(DIR *dirp, const char *dirpath) {
  struct dirent *dp;
  int fd;
  for (;;) {
    dp = readdir(dirp);
    if (dp == NULL)
      break;
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
      //. -> diretorio atual
      //.. -> diretorio pai
      continue;

    char filepath[1024];
    free(filename); 
    filename = malloc(strlen(dp->d_name) + 1);
    strcpy(filename, dp->d_name);
    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dp->d_name);


    fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir arquivo");
        break;
    }
  }
  return fd;
}

void close_files(DIR *dirp, int fd) {
  close(fd);
  closedir(dirp);
  free(filename);
}