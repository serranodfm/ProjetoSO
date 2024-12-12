#include "operations.h"

static struct HashTable* kvs_table = NULL;
char *directorypath = NULL;
static char *filenms[MAX_FILES]; 

extern int job_count_g;  // Número de tarefas na fila -> count
int job_index_g = 0;
int jobs_completed_g = 0; // Número de tarefas concluídas -> ???

// Variável de controle para encerrar as threads
int finished_g = 0; // -> ???

// Mutex para sincronizar o acesso à fila de tarefas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t child_semaphore;

extern pthread_mutex_t job_mutex;
extern pthread_mutex_t backup_mutex;
extern size_t MAX_CHILDREN;
extern size_t MAX_THREADS;
extern int child_count_g;
extern char *dirpath_g;
extern int *fd_s;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

int kvs_init() {
  if (sem_init(&child_semaphore, 0, (unsigned int)MAX_CHILDREN) != 0) {
    perror("Erro ao inicializar o semáforo");
    return EXIT_FAILURE;
  }
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
  sem_destroy(&child_semaphore);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  KeyValuePair *pairs = malloc(num_pairs * sizeof(KeyValuePair));
  for (size_t i = 0; i < num_pairs; i++) {
    strncpy(pairs[i].key, keys[i], MAX_STRING_SIZE);
    strncpy(pairs[i].value, values[i], MAX_STRING_SIZE);
  }

  qsort(pairs, num_pairs, sizeof(KeyValuePair), compareKeyValuePairs);

  for (size_t i = 0; i < num_pairs; i++) {
    strncpy(keys[i], pairs[i].key, MAX_STRING_SIZE);
    strncpy(values[i], pairs[i].value, MAX_STRING_SIZE);
  }
  free(pairs);

  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }
  return 0;
}

int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  qsort(keys, num_pairs, MAX_STRING_SIZE, compareStrings);

  kvs_out(createFormattedString("["), fd_out);
  for (size_t i = 0; i < num_pairs; i++) {
    char* result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
      kvs_out(createFormattedString("(%s,KVSERROR)", keys[i]), fd_out);
    } else {
      kvs_out(createFormattedString("(%s,%s)", keys[i], result), fd_out);
    }
    free(result);
  }
  kvs_out(createFormattedString("]\n"), fd_out);
  return 0;
}

int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;
  qsort(keys, num_pairs, MAX_STRING_SIZE, compareStrings);

  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        kvs_out(createFormattedString("["), fd_out);
        aux = 1;
      }
      kvs_out(createFormattedString("(%s,KVSMISSING)", keys[i]), fd_out);
    }
  }
  if (aux) {
    kvs_out(createFormattedString("]\n"), fd_out);
  }

  return 0;
}

void kvs_show(int fd_out) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    KeyNode *prevNode = NULL;
    if (keyNode != NULL) pthread_rwlock_rdlock(&keyNode->lock);
    while (keyNode != NULL) {
      kvs_out(createFormattedString("(%s, %s)\n", keyNode->key, keyNode->value), fd_out);

      if (keyNode->next != NULL) pthread_rwlock_rdlock(&keyNode->next->lock);
      prevNode = keyNode;
      keyNode = keyNode->next; // Move to the next node
      pthread_rwlock_unlock(&prevNode->lock);
    }
  }
}

int kvs_backup(char *dirpath, int bck_count, int index) {
  int fd;
  char count_str[20];

  snprintf(count_str, sizeof(count_str), "%d", bck_count);

  size_t bck_filename_len = strlen(dirpath) + strlen(filenms[index]) + strlen("-") + strlen(count_str) + 6 /*4(.bck) + 1("/0")*/;
  char *bck_filename = malloc(bck_filename_len);

  sprintf(bck_filename, "%s/%s-%s.bck", dirpath, filenms[index], count_str);

  fd = open(bck_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("Erro ao abrir arquivo");
    return 1;
  }

  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    KeyNode *prevNode = NULL;
    if (keyNode != NULL) pthread_rwlock_rdlock(&keyNode->lock);
    while (keyNode != NULL) {
      char *value = keyNode->value;
      char *key = keyNode->key;
      size_t bck_len = strlen(value) + strlen(key) + 6;
      char *bck = malloc(bck_len);

      bck_len = (size_t) snprintf(bck, bck_len, "(%s, %s)\n", key, value);

      if (write(fd, bck, bck_len) == -1) { 
        free(bck);
        return -1;
      }

      free(bck);
      if (keyNode->next != NULL) pthread_rwlock_rdlock(&keyNode->next->lock);
      prevNode = keyNode;
      keyNode = keyNode->next; // Move to the next node
      pthread_rwlock_unlock(&prevNode->lock);
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

int init_out(int index) {
  //o erro esta aqui, a mesma thread corre isto duas vezes
  filenms[index][strlen(filenms[index]) - 4] = '\0';
  size_t out_filename_len = strlen(directorypath) + strlen(filenms[index]) + 6 /*4(.out) + 1("/0")*/;
  char *out_filename = malloc(out_filename_len);

  sprintf(out_filename, "%s/%s.out", directorypath, filenms[index]);
  int fd_out = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_out == -1) {
    perror("Erro ao abrir arquivo");
  }
  free(out_filename);
  return fd_out;
}

void kvs_out(char *string, int fd_out) {
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

void process_job(int fd, int index) {
  int fd_out = init_out(index);
  while (1) {
    int bck_count = 1;
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;

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

        if (kvs_read(num_pairs, keys, fd_out)) {
          fprintf(stderr, "Failed to read pair\n");
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_delete(num_pairs, keys, fd_out)) {
          fprintf(stderr, "Failed to delete pair\n");
        }
        break;

      case CMD_SHOW:
          kvs_show(fd_out);
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
            sem_wait(&child_semaphore);

            pid_t pid = fork();
            if (pid == 0) {
              if (kvs_backup(dirpath_g, bck_count, index)) {
                  fprintf(stderr, "Failed to perform backup.\n");
              }
              _exit(0); 
            } else if (pid > 0) {
              bck_count++;
              break; 
            } else {
              perror("Erro ao criar processo com fork");
              sem_post(&child_semaphore); 
              break;
            }
          }

          while (waitpid(-1, NULL, WNOHANG) > 0) {
            sem_post(&child_semaphore); 
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
        close(fd_out);
        return;
    }
  }
}

//
void* thread_function(void* arg) {
  (void) arg;
  while (1) {
    int fd;
    int has_task = 0;
    int index;

    pthread_mutex_lock(&job_mutex);

    if (job_index_g < job_count_g) {
      fd = fd_s[job_index_g];
      job_index_g++;
      has_task = 1; 
    } else if (jobs_completed_g >= job_count_g) {
      finished_g = 1;
    }
    index = job_index_g-1;
    pthread_mutex_unlock(&job_mutex);

    if (has_task) {
      process_job(fd, index);
      pthread_mutex_lock(&job_mutex);
      jobs_completed_g++;
      pthread_mutex_unlock(&job_mutex);
    }

    if (finished_g) {
      break;
    }
  }
  return NULL;
}

int compareStrings(const void *a, const void *b) {
  const char *str1 = (const char *)a;
  const char *str2 = (const char *)b;
  return strcmp(str1, str2); 
}

int compareKeyValuePairs(const void *a, const void *b) {
  const KeyValuePair *pair1 = (const KeyValuePair *)a;
  const KeyValuePair *pair2 = (const KeyValuePair *)b;
  return strcmp(pair1->key, pair2->key);
}

void backup_mutex_init() {
  pthread_mutex_init(&backup_mutex, NULL);
}

void job_mutex_init() {
  pthread_mutex_init(&job_mutex, NULL);
}