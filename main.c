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
#ifndef DT_DIR
    #define DT_DIR 3
#endif
#define NORMAL_COLOR  "\x1B[0m"
#define GREEN  "\x1B[32m"
#define BLUE  "\x1B[34m"

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
    get_user_input(Username, &usernumber, &priceThreshold, orderlist);

    // After gathering input, you can continue further tasks
    printf("User Input Completed!\n");
    printf("Number of Order: %d\n", usernumber);
    printf("Price threshold: %lf\n", priceThreshold);

    for (int i = 0; i < usernumber; i++) {
        printf("Name: %s, Quantity: %d\n", orderlist[i].name, orderlist[i].quantity);
    }

    // Process all stores
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
        tokens[*num_tokens] = strdup(token);  // Allocate memory for each token
        if (tokens[*num_tokens] == NULL) {
            perror("Failed to duplicate token");
            free(str);
            free(tokens);  // Free the allocated memory for tokens
            return NULL;
        }
        (*num_tokens)++;
        token = strtok(NULL, " ");
    }

    free(str);  // Free the duplicated input string
    return tokens;  // Don't free tokens here; return them to the caller
}

// Function to split input by a custom delimiter and return a limited number of slices
char** get_slices_input_dataset(char *inputstr, int num_slices, char *splitter) {
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
            printf("Price threshold: ");
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

        // Store the order details
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

// Function to handle file processing
void* process_file(void* arg) {
    char* file_name = (char*) arg;
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        pthread_exit(NULL);
    } else {
        char line[256];
        char name[100];

        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Name:", 5) == 0) {
                // Skip "Name: " (5 characters), and store the actual product name
                strtok(line, "\n");
                strcpy(name, line + 6);  // Copy the name part after "Name: "
                break;  // Exit the loop after reading the name
            }
        }
        
        fclose(file);

        // Compare the name from the file with orderlist[i].name
        for (int i = 0; i < sizeof(orderlist)/sizeof(orderlist[0]); i++) {
            if (strcmp(name, orderlist[i].name) == 0) {
                printf("Match found at: %s\n", file_name);
                // Perform additional logic if necessary
                break;
            }
        }
    }
    return 0;
}

// Function to process files in each category
void* process_category(void* arg) {
    char* category_path = (char*) arg;
    DIR * d = opendir(category_path);
    if (d == NULL) {
        perror("Failed to open category directory");
        return NULL;
    }
    struct dirent* dir;
    int file_count = 0;    
    // Count the number of files in the category
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {  // Regular file
            file_count++;
        }
    }
    closedir(d);

    int thread_index = 0;
    pthread_t threads[file_count];
    d = opendir(category_path);
    // Create a thread for each file in the category
    while ((dir = readdir(d)) != NULL) {
        if (dir-> d_type != DT_DIR) {  // Regular file
            // Dynamically calculate the size for new_category
            size_t new_category_size = strlen(category_path) + strlen(dir->d_name) + 2;  // +2 for '/' and '\0'
            char *file_path = malloc(new_category_size);
            if (file_path == NULL) {
                perror("Failed to allocate memory for new_category");
                closedir(d);
                return NULL;
            }

            // Build the path safely using snprintf
            snprintf(file_path, new_category_size, "%s/%s", category_path, dir->d_name);

            // Create a new thread to process the file
            int x = pthread_create(&threads[thread_index], NULL, process_file, (void*) file_path);
            if (x != 0) {
                perror("Error creating thread");
                free(file_path);  // Clean up memory if thread creation fails
            }

            thread_index++;
        }
    }
    closedir(d);

    // Wait for all threads to finish
    for (int i = 0; i < file_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return NULL;
}

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
                // puts("Child(category) exiting.");
                exit(EXIT_SUCCESS);
           default:
                // printf("Child is PID %jd\n", (intmax_t) category_pid);
                // puts("Parent exiting.");
                // exit(EXIT_SUCCESS);
                break;
           }
    }
    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
    return NULL;
}

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
                // puts("Child (Store) exiting.");
                exit(EXIT_SUCCESS);
            default:
                // printf("Child is PID %jd\n", (intmax_t) store_pid);
                // puts("Parent exiting.");
                // exit(EXIT_SUCCESS);
                break;
           }
    }

    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
}

