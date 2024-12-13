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

// Define Order struct
struct Order {
    char name[50];
    int quantity;
};

// Define User struct
struct User {
    char username[50];  // Allocate memory for username
    struct Order orderlist[MAX_PRODUCTS];  // Use an array for the orderlist
    float priceThreshold;
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
struct User get_user_details();
void handle_all_stores();
void* handle_store(void* arg);
void* handle_category(void* arg);
void* handle_file(void* arg);
// char** get_slices_input_dataset(char *inputstr, int num_slices, char *splitter);

// Main function
int main() {
    // do {
    // create process for user
    struct User user = get_user_details();

    // After gathering input, you can continue further tasks
    printf("User Input Completed!\n");

    printf("\nOrder List:\n");
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        if (user.orderlist[i].quantity == 0) {
            // Assuming a quantity of 0 means no more valid orders in the list
            break;
        }
        printf("Item: %s, Quantity: %d\n", user.orderlist[i].name, user.orderlist[i].quantity);
    }
    handle_all_stores();
    // } while(1);
    return 0;
}

// Function to split input by space and return an array of strings (tokens)
// char** get_slices_input(char *inputstr, int *num_tokens , char *splitter) {
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
//     char *token = strtok(str, splitter);  // Split by space

//     // Loop through all tokens and store them in the array
//     while (token != NULL) {
//         tokens[*num_tokens] = strdup(token);  // Allocate memory for each token
//         if (tokens[*num_tokens] == NULL) {
//             perror("Failed to duplicate token");
//             free(str);
//             free(tokens);  // Free the allocated memory for tokens
//             return NULL;
//         }
//         (*num_tokens)++;
//         token = strtok(NULL, " ");
//     }

//     free(str);  // Free the duplicated input string
//     return tokens;  // Don't free tokens here; return them to the caller
// }

char** get_slices_input(char *inputstr, int *num_tokens , char *splitter) {
    char **result = malloc(10 * sizeof(char *));  // allocate for 10 tokens
    char *token = strtok(inputstr, splitter);
    int index = 0;

    while (token != NULL) {
        result[index++] = strdup(token);  // duplicate the token to store it
        token = strtok(NULL, splitter);
    }
    *num_tokens = index;
    return result;
}

// Function to split input by a custom delimiter and return a limited number of slices
// char** get_slices_input_dataset(char *inputstr, int num_slices, char *splitter) {
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

//     int token_count = 0;  // Initialize token count
//     char *token = strtok(str, splitter);

//     // Loop through all tokens and store them in the array
//     while (token != NULL && (num_slices == 0 || token_count < num_slices)) {
//         tokens[token_count] = strdup(token);
//         if (tokens[token_count] == NULL) {
//             perror("Failed to duplicate token");
//             free(str);
//             return NULL;
//         }
//         token_count++;
//         token = strtok(NULL, splitter);
//     }

//     // If num_slices is 0, split till the end
//     if (num_slices == 0) {
//         num_slices = token_count;
//     }

//     free(str);  // Free the duplicated input string
//     return tokens;
// }

// Function to get user input
struct User get_user_details() {
    struct User user;
    user.priceThreshold = -1.0;  // Initialize the priceThreshold
    int count = 0;
    
    // Ask for username
    printf("Username: ");
    fgets(user.username, sizeof(user.username), stdin);
    user.username[strcspn(user.username, "\n")] = 0; // Remove newline character

    printf("Order List:\n");

    // Read input until an empty line is entered
    while (1) {
        fgets(input, sizeof(input), stdin);

        // If the line is empty, break out of the loop
        if (strcmp(input, "\n") == 0) {
            // Prompt for price threshold
            printf("Price threshold: ");
            fgets(input, sizeof(input), stdin);

            if (strcmp(input, "\n") == 0) {
                // If user just hits Enter, priceThreshold stays -1
                break;
            }

            // Convert input to float for priceThreshold
            user.priceThreshold = strtof(input, NULL);
            break;
        }

        // Get the tokens from the input
        int num_tokens = 0;
        char **tokens = get_slices_input(input, &num_tokens, " ");

        if (num_tokens < 2) {
            printf("Invalid input format, please try again.\n");
            free(tokens);
            continue;
        }

        // Allocate the item name and store the order
        char *combinedstr = malloc(strlen(tokens[0]) + 1);
        strcpy(combinedstr, tokens[0]);
        for (int i = 1; i < num_tokens - 1; i++) {
            combinedstr = realloc(combinedstr, strlen(combinedstr) + strlen(tokens[i]) + 2);
            strcat(combinedstr, " ");
            strcat(combinedstr, tokens[i]);
        }

        int quantity = atoi(tokens[num_tokens - 1]);
        // printf("Item: %s, Quantity: %d\n", combinedstr, quantity);

        // Store the order details in the orderlist
        if (count < MAX_PRODUCTS) {
            strncpy(user.orderlist[count].name, combinedstr, sizeof(user.orderlist[count].name) - 1);
            user.orderlist[count].name[sizeof(user.orderlist[count].name) - 1] = '\0';  // Ensure null-termination
            user.orderlist[count].quantity = quantity;
            count++;
        }

        // Free the allocated memory for tokens and combined string
        free(combinedstr);
        for (int i = 0; i < num_tokens; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    return user;
}

// Function to handle file processing
void* handle_file(void* arg) {
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

        // Compare the name from the file with orderlist[i].name
        for (int i = 0; i < sizeof(orderlist)/sizeof(orderlist[0]); i++) {
            if (strcmp(name, orderlist[i].name) == 0) {
                printf("Match found at: %s\n", file_name);
                // check entity
                // returning price * score appen to a global List
                // 
                break;
            }
        }

        fclose(file);
    }
    // Returning a List od useful things + TID
    return 0;
}

// Function to process files in each category
void* handle_category(void* arg) {
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
            int x = pthread_create(&threads[thread_index], NULL, handle_file, (void*) file_path);
            if (x != 0) {
                perror("Error creating thread");
                free(file_path);  // Clean up memory if thread creation fails
            }

            thread_index++;
        }
    }
    closedir(d);

    // get many list of useful items from threads
    // deciding which one to close and which ones to sleep
    for (int i = 0; i < file_count; i++) {
        pthread_join(threads[i], NULL);
    }
    // returning a list of useful things + CID
    return NULL;
}

void* handle_store(void* arg) {
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
                handle_category((void*) category_path);
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

void handle_all_stores() {
    char store_paths[3][100] = {"Store1", "Store2", "Store3"};
    
    pid_t store_pid;
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
               perror("signal");
               exit(EXIT_FAILURE);
           }
    // if a thread finds a products
    /* 
        create 3 threads for
        Orders: ارزش گذاری سبد های خرید فروشگاه
        Scores: امتیازگذاری مجدد و آ\دیت امتیاز محصولات 
        Final: آبدیت کردن محصولات نهایی سبد خرید کاربر
        sleep all three threads
    */
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
                handle_store((void*) store_path);
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

