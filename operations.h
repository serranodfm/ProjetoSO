#ifndef KVS_OPERATIONS_H
#define KVS_OPERATIONS_H

#include <stddef.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "kvs.h"
#include "constants.h"
#include "parser.h"

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
/// @param fd_out File descriptor to write the (successful) output.
/// @return 0 if the key reading, 1 otherwise.
int kvs_read(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out);

/// Deletes key value pairs from the KVS.
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @return 0 if the pairs were deleted successfully, 1 otherwise.
int kvs_delete(size_t num_pairs, char keys[][MAX_STRING_SIZE], int fd_out);

/// Writes the state of the KVS.
/// @param fd_out File descriptor to write the output.
void kvs_show(int fd_out);

/// Creates a backup of the KVS state and stores it in the correspondent
/// backup file
/// @param dirpath directory path
/// @param bck_count backup counter
/// @param index file index
/// @return 0 if the backup was successful, 1 otherwise.
int kvs_backup(char *dirpath, int bck_count, int index);

/// Waits for the last backup to be called.
void kvs_wait_backup();

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void kvs_wait(unsigned int delay_ms);

/// Opens the directory
/// @param dirpath directory path
/// @return DIR 
DIR *open_dir(const char *dirpath);

/// Reads the files from a directory
/// @param dirp DIR directory
/// @param dirpath directory path
/// @param count file counts
/// @return list of files descriptors
int *read_files_in_directory(DIR *dirp, const char *dirpath, int *count);

/// Closes the directory and opened files
/// @param dirp DIR directory
/// @param dirpath directory path
/// @param count file counts
void close_files(DIR *dirp, int *fds, int count);

/// Initialize the .out fd
/// @param index file index
/// @return fd
int init_out(int index);

/// Write the string on the file .out
/// @param string string to write
/// @param fd_out file descriptor
void kvs_out(char *string, int fd_out);

/// Create a formatted string
/// @param format string format
/// @param ... arguments
/// @return formatted string
char *createFormattedString(const char *format, ...);

/// Process the current .job 
/// @param fd file descriptor
/// @param index file index
void process_job(int fd, int index);

/// Auxiliary function for qsort
/// @param a string to compare
/// @param b another string
/// @return strcmp between a and b
int compareStrings(const void *a, const void *b);

/// Auxiliary function for qsort
/// Same thing but it allows to compare two KeyValuePairs
/// @param a string to compare
/// @param b another string
/// @return strcmp between a and b
int compareKeyValuePairs(const void *a, const void *b);

/// Thread function to process jobs from a shared job queue.
void* thread_function(void* arg);

/// To initialize rwlocks
void bucket_rwlocks_init();

/// To destroy rwlocks
void bucket_rwlocks_destroy();

/// To rd_lock
void bucket_rdlock_all();

/// To unlock
void bucket_unlock_all();

/// To wr_lock specific indexes
/// @param indexes indexes to lock
void bucket_wrlock_indexes(size_t indexes[]);

/// To rd_lock specific indexes
/// @param indexes indexes to lock
void bucket_rdlock_indexes(size_t indexes[]);

/// To unlock specific indexes
/// @param indexes indexes to lock
void bucket_unlock_indexes(size_t indexes[]);

/// Fills the hash table index with the occupied buckets
/// @param num_pairs number of pairs
/// @param keys keys
/// @param indexes indexes
void get_hash_indexes(size_t num_pairs, char keys[][MAX_STRING_SIZE], size_t (*indexes)[TABLE_SIZE]);

#endif  // KVS_OPERATIONS_H
