#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>  // Added this header to use 'wait()'
#include <signal.h>
#include <inttypes.h>


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
struct OrderList orderlist[100];

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
void get_user_input(char *Username, int *usernumber, float *priceThreshold, struct OrderList *orderlist);
void process_all_stores();
void* process_store(void* arg);
void* process_category(void* arg);
void* process_file(void* arg);
int compareOrderName(FILE *file, struct OrderList *orderlist);
char** get_slices_input_dataset(char *inputstr, int num_slices, char *splitter);

// Main function
int main() {
    // Declare variables
    int usernumber = 0;
    char Username[50];
    float priceThreshold = -1;
    
    // Call function to get user input
    // while:
    get_user_input(Username, &usernumber, &priceThreshold, orderlist);

    // After gathering input, you can continue further tasks
    printf("User Input Completed!\n");
    printf("Number of Order: %d\n", usernumber);
    printf("Price threshold: %lf\n", priceThreshold);

    for (int i = 0; i < usernumber; i++) {
        printf("Name: %s, Quantity: %d\n", orderlist[i].name, orderlist[i].quantity);
    }

    // Create Thread (if needed)
    // Create Terminal
    process_all_stores();
    return 0;
}

// Function to split input by space and return an array of strings (tokens)
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter) {
    char *str = strdup(inputstr); 
    if (str == NULL) {
        perror("Failed to duplicate input string");
        return NULL;
    }

    // Allocate memory for the array of strings (tokens)
    char **tokens = malloc(MAX_PRODUCTS * sizeof(char*));
    if (tokens == NULL) {
        perror("Failed to allocate memory for tokens");
        free(str);
        return NULL;
    }

    *num_tokens = 0;  // Initialize token count
    char *token = strtok(str, splitter);  // Split by space

    // Loop through all tokens and store them in the array
    while (token != NULL) {
        tokens[*num_tokens] = strdup(token);
        if (tokens[*num_tokens] == NULL) {
            perror("Failed to duplicate token");
            free(str);
            return NULL;
        }
        (*num_tokens)++;
        token = strtok(NULL, " ");
    }

    free(str);  // Free the duplicated input string

    return tokens;
}

// Function to split input by a custom delimiter and return a limited number of slices
char** get_slices_input_dataset(char *inputstr, int num_slices, char *splitter) {
    // Duplicate the input string to prevent modifying the original string
    char *str = strdup(inputstr); 
    if (str == NULL) {
        perror("Failed to duplicate input string");
        return NULL;
    }

    // Allocate memory for the array of strings (tokens)
    char **tokens = malloc(MAX_PRODUCTS * sizeof(char*));
    if (tokens == NULL) {
        perror("Failed to allocate memory for tokens");
        free(str);
        return NULL;
    }

    int token_count = 0;  // Initialize token count
    char *token = strtok(str, splitter);

    // Loop through all tokens and store them in the array
    while (token != NULL && (num_slices == 0 || token_count < num_slices)) {
        tokens[token_count] = strdup(token);
        if (tokens[token_count] == NULL) {
            perror("Failed to duplicate token");
            free(str);
            return NULL;
        }
        token_count++;
        token = strtok(NULL, splitter);
    }

    // If num_slices is 0, split till the end
    if (num_slices == 0) {
        num_slices = token_count;
    }

    // Clean up and return the result
    free(str);  // Free the duplicated input string

    return tokens;
}

// Function to get user input
void get_user_input(char *Username, int *usernumber, float *priceThreshold, struct OrderList *orderlist) {
    printf("Username: ");
    scanf("%s", Username);
    getchar();  // Clear the newline character left by scanf
    printf("OrderList: \n");
    char item[20];
    int number;
    int num_tokens = 0;
    int count = 0;

    // Read input until an empty line is entered
    while (1) {
        fgets(input, sizeof(input), stdin);

        // If the line is empty, break out of the loop
        if (strcmp(input, "\n") == 0) {
            printf("Price threshold:");

            fgets(input, sizeof(input), stdin);
            if (strcmp(input, "\n") == 0) {
                break;
            }
            *priceThreshold = strtof(input, NULL);
            break;
        }

        // Get the tokens from the input
        char **tokens = get_slices_input(input, &num_tokens, " ");

        char *combinedstr = strcpy(item, tokens[0]);
        for (int i = 1; i < num_tokens - 1; i++) {
            strcat(combinedstr, " "); 
            strcat(combinedstr, tokens[i]); 
        }

        number = atoi(tokens[num_tokens - 1]);
        printf("Item: %s, Quantity: %d\n", combinedstr, number);

        // Use count to track the index for orderlist
        orderlist[count].quantity = number;
        strncpy(orderlist[count].name, combinedstr, sizeof(orderlist[count].name) - 1);
        orderlist[count].name[sizeof(orderlist[count].name) - 1] = '\0';  // Ensure null-termination

        count++;  // Increment count for the next order

        // Free the allocated memory for tokens
        for (int i = 0; i < num_tokens; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    *usernumber = count;
}


void* process_file(void* arg) {
    char* file_name = (char*) arg;

    // printf("Processinasdasbjacvhg file: %s\n", file_name);
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        pthread_exit(NULL);
    }
    else if (compareOrderName(file, orderlist)) {
        printf("Name matches!\n");
        printf("Processing file: %s\n", file_name);
    } else {
        // printf("Name does not match\n");
        fclose(file); 
        pthread_exit(NULL);
    }

    fclose(file);
    return 0;
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
            // printf("Creating thread for file: %s\n", entry->d_name);
            // In a real case, the thread function would process the file.
            // printf("dir %s dname %s\n",category_path, entry->d_name);

            char **tokens = get_slices_input_dataset(category_path, 4, "/");

            char *new_category = tokens[0];
            strcat(new_category, "/");
            for (int i = 1; i < 4; i++) {
                strcat(new_category, tokens[i]); 
                strcat(new_category, "/");
            }

            // get_slices_input(category_path, 0, "/");
            
            // printf("\nnew_category %s\n", new_category);
            // printf("entry->d_name %s\n", entry->d_name);
            // printf("strcat(new_category, entry->d_name) %s\n\n",strcat(new_category, entry->d_name));
            // printf("new_category %s\n", strcat(new_category, entry->d_name));
            int x = pthread_create(&threads[thread_index], NULL, process_file, (void*) strcat(new_category, entry->d_name));
            // printf("tedad threads = %d\n", thread_index);
            // if(x != 1)
            //     pthread_join(threads[thread_index], NULL);
            // else
            //     printf("x == %d", x);
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
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
               perror("signal");
               exit(EXIT_FAILURE);
           }

    for (int i = 0; i < 8; i++) {
        char category_path[200];
        snprintf(category_path, sizeof(category_path), "%s/%s", store_path, category_paths[i]);

        // snprintf(store_path, sizeof(store_path), "./Dataset/%s", ()arg);

        // Create a process for each store
        category_pid = fork();
        switch (category_pid) {
           case -1:
                perror("fork");
                exit(EXIT_FAILURE);
           case 0:
                process_category((void*) category_path);
                puts("Child exiting.");
                exit(EXIT_SUCCESS);
           default:
                printf("Child is PID %jd\n", (intmax_t) category_pid);
                puts("Parent exiting.");
                // exit(EXIT_SUCCESS);
                break;
           }
    }
    return NULL;
}

// Function to process all stores (create a process for each store)
void process_all_stores() {
    char store_paths[3][100] = {"Store1", "Store2", "Store3"};
    
    pid_t store_pid;
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
               perror("signal");
               exit(EXIT_FAILURE);
           }
    for (int i = 0; i < 3; i++) {
        char store_path[200];
        snprintf(store_path, sizeof(store_path), "./Dataset/%s", store_paths[i]);

        // Create a process for each store
        store_pid = fork();
        switch (store_pid) {
           case -1:
                perror("fork");
                exit(EXIT_FAILURE);
           case 0:
                process_store((void*) store_path);
                puts("Child exiting.");
                exit(EXIT_SUCCESS);
           default:
                printf("Child is PID %jd\n", (intmax_t) store_pid);
                puts("Parent exiting.");
                // exit(EXIT_SUCCESS);
                break;
           }
    }

    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
}

int compareOrderName(FILE *file, struct OrderList *orderlist) {
    char line[256];
    char name[100];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "Name: %99[^\n]", name) == 1) {
            // Compare Name
            if (strcmp(name, orderlist->name) == 0) {
                return 1; // Match found
            }
        }
    }

    return 0; // No match
}

