#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>  // Added this header to use 'wait()'

#define MAX_PRODUCTS 100
#ifndef DT_REG
    #define DT_REG 8
#endif

// Global Variable
char input[100];
struct OrderList {
    char name[50];
    int quantity;
};

// Function to split input by space and return an array of strings (tokens)
// char** get_slices_input(char *inputstr, int *num_tokens) {
//     char *str = strdup(inputstr); 
//     if (str == NULL) {
//         perror("Failed to duplicate input string");
//         return NULL;
//     }

//     // Allocate memory for the array of strings (tokens)
//     char **tokens = malloc(MAX_PRODUCTS * sizeof(char*));
//     if (tokens == NULL) {
//         perror("Failed to allocate memory for tokens");
//         free(str);
//         return NULL;
//     }

//     *num_tokens = 0;  // Initialize token count
//     char *token = strtok(str, " ");  // Split by space

//     // Loop through all tokens and store them in the array
//     while (token != NULL) {
//         tokens[*num_tokens] = strdup(token);
//         if (tokens[*num_tokens] == NULL) {
//             perror("Failed to duplicate token");
//             free(str);
//             return NULL;
//         }
//         (*num_tokens)++;
//         token = strtok(NULL, " ");
//     }

//     free(str);  // Free the duplicated input string

//     return tokens;
// }

// Function to simulate processing a file (thread)
void* process_file(void* arg) {
    char* file_name = (char*) arg;
    printf("Processing file: %s\n", file_name);
    ////// processing file
    return NULL;
}

// Function to handle a category (create threads for each file)
void* process_category(void* arg) {
    char* category_path = (char*) arg;
    DIR* dir = opendir(category_path);
    if (dir == NULL) {
        perror("Failed to open category directory");
        return NULL;
    }

    struct dirent* entry;
    int file_count = 0;
    
    // Count the number of files in the category
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            file_count++;
        }
    }
    closedir(dir);

    pthread_t threads[file_count];
    dir = opendir(category_path);
    int thread_index = 0;

    // Create a thread for each file in the category
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            printf("Creating thread for file: %s\n", entry->d_name);
            // In a real case, the thread function would process the file.
            pthread_create(&threads[thread_index], NULL, process_file, (void*) entry->d_name);
            thread_index++;
        }
    }
    closedir(dir);

    // Wait for all threads to finish
    for (int i = 0; i < file_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return NULL;
}

// Function to process a store (create a process for each category)
void* process_store(void* arg) {
    char* store_path = (char*) arg;
    char category_paths[8][100] = {  // Adjusted the size to 8
        "Apparel", "Beauty", "Digital", "Food", "Home", "Market", "Sports", "Toys"
    };

    pid_t category_pid;
    for (int i = 0; i < 8; i++) {
        char category_path[200];
        snprintf(category_path, sizeof(category_path), "%s/%s", store_path, category_paths[i]);

        // Create a process for each category
        category_pid = fork();
        if (category_pid == 0) {
            // Child process handles category
            process_category((void*) category_path);
            exit(0);  // Exit after processing the category
        } else if (category_pid < 0) {
            perror("Fork failed for category");
        }
    }

    // Wait for all category processes to finish
    for (int i = 0; i < 8; i++) {
        wait(NULL);
    }

    return NULL;
}

// Function to process all stores (create a process for each store)
void process_all_stores() {
    char store_paths[3][100] = {"Store1", "Store2", "Store3"};
    
    pid_t store_pid;
    for (int i = 0; i < 3; i++) {
        char store_path[200];
        snprintf(store_path, sizeof(store_path), "./Dataset/%s", store_paths[i]);

        // Create a process for each store
        store_pid = fork();
        if (store_pid == 0) {
            // Child process handles store
            process_store((void*) store_path);
            exit(0);  // Exit after processing the store
        } else if (store_pid < 0) {
            perror("Fork failed for store");
        }
    }

    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
}

int main() {
    // Main function to initialize and start processing stores
    process_all_stores();

    return 0;
}
