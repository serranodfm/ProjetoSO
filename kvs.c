#include "kvs.h"

// Hash function based on key initial.
// @param key Lowercase alphabetical string.
// @return hash.
// NOTE: This is not an ideal hash function, but is useful for test purposes of the project
int hash(const char *key) {
    int firstLetter = tolower(key[0]);
    if (firstLetter >= 'a' && firstLetter <= 'z') {
        return firstLetter - 'a';
    } else if (firstLetter >= '0' && firstLetter <= '9') {
        return firstLetter - '0';
    }
    return -1; // Invalid index for non-alphabetic or number strings
}


struct HashTable* create_hash_table() {
  HashTable *ht = malloc(sizeof(HashTable));
  if (!ht) return NULL;
  for (int i = 0; i < TABLE_SIZE; i++) {
      ht->table[i] = NULL;
  }
  return ht;
}

int write_pair(HashTable *ht, const char *key, const char *value) {
    int index = hash(key);
    KeyNode *keyNode = ht->table[index];

    while (keyNode != NULL) {
        pthread_rwlock_wrlock(&keyNode->lock);
        if (strcmp(keyNode->key, key) == 0) {
            free(keyNode->value);
            keyNode->value = strdup(value);
            pthread_rwlock_unlock(&keyNode->lock);
            return 0;
        }
        pthread_rwlock_unlock(&keyNode->lock);
        keyNode = keyNode->next;
    }

    keyNode = malloc(sizeof(KeyNode));
    keyNode->key = strdup(key);
    keyNode->value = strdup(value);
    pthread_rwlock_init(&keyNode->lock, NULL);
    pthread_rwlock_wrlock(&keyNode->lock);
    keyNode->next = ht->table[index];
    ht->table[index] = keyNode;
    pthread_rwlock_unlock(&keyNode->lock);
    return 0;
}

char* read_pair(HashTable *ht, const char *key) {
    int index = hash(key);
    KeyNode *keyNode = ht->table[index];
    KeyNode *prevNode = NULL;
    char* value;

    if (keyNode != NULL) pthread_rwlock_rdlock(&keyNode->lock);
    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            value = strdup(keyNode->value);
            pthread_rwlock_unlock(&keyNode->lock);
            return value; // Return copy of the value if found
        }
        prevNode = keyNode;
        if (keyNode->next != NULL) pthread_rwlock_rdlock(&keyNode->next->lock);
        keyNode = keyNode->next; // Move to the next node
        pthread_rwlock_unlock(&prevNode->lock);
    }
    return NULL; // Key not found
}

int delete_pair(HashTable *ht, const char *key) {
    int index = hash(key);
    KeyNode *keyNode = ht->table[index];
    KeyNode *prevNode = NULL;

    // Search for the key node
    if (keyNode != NULL) pthread_rwlock_wrlock(&keyNode->lock);
    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            pthread_rwlock_wrlock(&keyNode->lock);
            // Key found; delete this node
            if (prevNode == NULL) {
                // Node to delete is the first node in the list
                ht->table[index] = keyNode->next; // Update the table to point to the next node
            } else {
                // Node to delete is not the first; bypass it
                prevNode->next = keyNode->next; // Link the previous node to the next node
                pthread_rwlock_unlock(&prevNode->lock);
            }
            pthread_rwlock_unlock(&keyNode->lock);
            // Free the memory allocated for the key and value
            free(keyNode->key);
            free(keyNode->value);
            pthread_rwlock_unlock(&keyNode->lock);
            pthread_rwlock_destroy(&keyNode->lock);
            free(keyNode); // Free the key node itself
            return 0; // Exit the function
        }
        if (prevNode != NULL) pthread_rwlock_unlock(&prevNode->lock);
        prevNode = keyNode;
        if (keyNode->next != NULL) pthread_rwlock_wrlock(&keyNode->next->lock);
        keyNode = keyNode->next; // Move to the next node
    }
    
    return 1;
}

void free_table(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        KeyNode *keyNode = ht->table[i];
        while (keyNode != NULL) {
            KeyNode *temp = keyNode;
            keyNode = keyNode->next;
            free(temp->key);
            free(temp->value);
            pthread_rwlock_destroy(&temp->lock);
            free(temp);
        }
    }
    free(ht);
}