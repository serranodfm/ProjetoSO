#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>


#include "kvs.h"
#include "constants.h"
#include "operations.h"


static struct HashTable* kvs_table = NULL;
char *directorypath = NULL;
int fd_out;
static char *filenms[MAX_FILES]; 
int index = 0;


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
  init_out();
  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  free_table(kvs_table);
  close(fd_out);
  return 0;
}

void kvs_clean() {
  kvs_terminate();
  kvs_table = NULL;
  kvs_init();
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

  HashTable *readHash = create_hash_table();
  for (size_t i = 0; i < num_pairs; i++) {
    write_pair(readHash, keys[i], "");
  }

  kvs_out(createFormattedString("["));
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = readHash->table[i];
    while (keyNode != NULL) {
      char* result = read_pair(kvs_table, keyNode->key);
      if (result == NULL) {
        kvs_out(createFormattedString("(%s,KVSERROR)", keyNode->key));
      } else {
        kvs_out(createFormattedString("(%s,%s)", keyNode->key, result));
      }
      free(result); 
      keyNode = keyNode->next;
    }
  }
  kvs_out(createFormattedString("]\n"));
  free_table(readHash);
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
        kvs_out(createFormattedString("["));
        aux = 1;
      }
      kvs_out(createFormattedString("(%s,KVSMISSING)", keys[i]));
    }
  }
  if (aux) {
    kvs_out(createFormattedString("]\n"));
  }

  return 0;
}

void kvs_show() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      kvs_out(createFormattedString("(%s, %s)\n", keyNode->key, keyNode->value));
      keyNode = keyNode->next; // Move to the next node
    }
  }
}

int kvs_backup(char *dirpath, int bck_count) {
  int fd;
  char count_str[20];

  snprintf(count_str, sizeof(count_str), "%d", bck_count);

  size_t bck_filename_len = strlen(dirpath) + strlen(filenms[index]) + strlen("-") + strlen(count_str) + 5 /*4(.bck) + 1("/0")*/;
  char *bck_filename = malloc(bck_filename_len);

  sprintf(bck_filename, "%s%s-%s.bck", dirpath, filenms[index], count_str);

  fd = open(bck_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("Erro ao abrir arquivo");
    return 1;
  }

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      char *value = keyNode->value;
      char *key = keyNode->key;
<<<<<<< HEAD
      size_t bck_len = strlen(value) + strlen(key) + 5;
      char *bck = malloc(bck_len + 1);
=======
      size_t bck_len = strlen(value) + strlen(key) + 6;
      char *bck = malloc(bck_len); 
>>>>>>> origin/main

      snprintf(bck, bck_len, "(%s, %s)\n", key, value); 

      if (write(fd, bck, bck_len) == -1) { 
        free(bck);
        return -1;
      }

      free(bck);
      keyNode = keyNode->next;
    }
  }

  free(bck_filename);
  close(fd);
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

    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || strcmp(dp->d_name + strlen(dp->d_name) - 4, ".job") != 0)
      continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dp->d_name);
    free(directorypath); 
    directorypath = malloc(strlen(dirpath) + 1);
    strcpy(directorypath, dirpath); //guardar o dir

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
      perror("Erro ao abrir arquivo");
      continue;
    }
    filenms[size] = malloc(strlen(dp->d_name)+1);
    strcpy(filenms[size], dp->d_name); //guardar o nome do ficheiro
    fds[size++] = fd;
  }

  *count = size; // Retorna o número de fds armazenados
  return fds;
}

void init_out() {
  filenms[index][strlen(filenms[index]) - 4] = '\0';

  size_t out_filename_len = strlen(directorypath) + strlen(filenms[index]) + 5 /*4(.out) + 1("/0")*/;
  char *out_filename = malloc(out_filename_len);

  sprintf(out_filename, "%s%s.out", directorypath, filenms[index]);

  fd_out = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_out == -1) {
    perror("Erro ao abrir arquivo");
  }
  free(out_filename);
}

void kvs_out(char *string) {
  size_t string_len = strlen(string); // Calcula o tamanho da string
  if (write(fd_out, string, string_len) == -1) { // Escreve a string diretamente no arquivo
    perror("Erro ao escrever no arquivo");
  }
  free(string);
}

void close_files(DIR *dirp, int* fds, int count) {
  for (int i = 0; i < count; i++) {
    close(fds[i]);
    free(filenms[i]);
  }
  closedir(dirp);
  free(directorypath);
}

char *createFormattedString(const char *format, ...) {
  va_list args;
  va_start(args, format);

  int size = vsnprintf(NULL, 0, format, args) + 1; 
  va_end(args);

  char *formattedString = (char *)malloc((size_t)size);
  if (formattedString == NULL) {
    perror("Erro ao alocar memória");
    exit(EXIT_FAILURE);
  }

  va_start(args, format);
  vsnprintf(formattedString, (size_t)size, format, args);
  va_end(args);

  return formattedString; 
}

void new_index(int new_index) {
  index = new_index;
}