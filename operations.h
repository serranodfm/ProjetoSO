#ifndef KVS_OPERATIONS_H
#define KVS_OPERATIONS_H

#include <stddef.h>

#define MAX_TASKS 100

typedef struct {
    char key[MAX_STRING_SIZE];
    char value[MAX_STRING_SIZE];
    //pthread_rwlock_t lock; nao sei se aqui da jeito, é usado no kvs_write
} KeyValuePair;

/// Initializes the KVS state.
/// @return 0 if the KVS state was initialized successfully, 1 otherwise.
int kvs_init();

/// Destroys the KVS state.
/// @return 0 if the KVS state was terminated successfully, 1 otherwise.
int kvs_terminate();

/// Writes a key value pair to the KVS. If key already exists it is updated.
/// @param num_pairs Number of pairs being written.
/// @param keys Array of keys' strings.
/// @param values Array of values' strings.
/// @return 0 if the pairs were written successfully, 1 otherwise.
int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]);

/// Reads values from the KVS.
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @param fd File descriptor to write the (successful) output.
/// @return 0 if the key reading, 1 otherwise.
int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out);

/// Deletes key value pairs from the KVS.
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @return 0 if the pairs were deleted successfully, 1 otherwise.
int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out);

/// Writes the state of the KVS.
/// @param fd File descriptor to write the output.
void kvs_show(int fd_out);

/// Creates a backup of the KVS state and stores it in the correspondent
/// backup file
/// @return 0 if the backup was successful, 1 otherwise.
int kvs_backup(char *dirpath, int bck_count, int index);

/// Waits for the last backup to be called.
void kvs_wait_backup();

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void kvs_wait(unsigned int delay_ms);


DIR *open_dir(const char *dirpath);
int *read_files_in_directory(DIR *dirp, const char *dirpath, int *count);
void close_files(DIR *dirp, int *fds, int count);
int init_out(int index);
void kvs_out(char *string, int fd_out);
char *createFormattedString(const char *format, ...);
void process_job(int fd, int index);
int compareStrings(const void *a, const void *b);
int compareKeyValuePairs(const void *a, const void *b);
void backup_mutex_init();
void* thread_function(void* arg);

#endif  // KVS_OPERATIONS_H
