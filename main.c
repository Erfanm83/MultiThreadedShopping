#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>  // Added this header to use 'wait()'
#include <signal.h>
#include <inttypes.h>
#include <sys/syscall.h>   // For syscall and gettid
#include <stdbool.h>
#include <time.h>

#define MAX_PRODUCTS 100
// #define MAX_PATH_LEN 512
#define MAX_PATH_LEN 1024
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

struct Order {
    char name[50];
    int quantity;
};

struct Date {
    int year;
    int month;
    int day;
};

struct Time {
    int hour;
    int minutes;
    int seconds;
};

struct Product {
    char name[100];
    float price;
    float score;
    int entity;
    float cost;
    struct Date date;
    struct Time time;
};

struct ThreadDetails {
    pthread_t tid;            // Thread ID
    struct Product product;   // Product being processed
    pthread_cond_t cond_var;  // Condition variable to wake the thread
    pthread_mutex_t mutex;    // Mutex to protect the shared data
    int new_score;            // Store new score when user rates
    int updated;              // Flag to indicate if the thread has been updated
};

struct User {
    char username[50];
    struct Order orderlist[MAX_PRODUCTS];
    float priceThreshold;
    int totalOrder;
};

struct HandleArgs {
    struct User* user;
    char* file_path;
    int storeIndex;
    struct Product product;
    struct ThreadDetails user_details;
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
int get_user_details(struct User* user);
bool handle_all_stores(struct HandleArgs* args);
void* handle_store(struct HandleArgs* args);
void* handle_file(void* args);
void check_and_ask_to_buy(struct HandleArgs* args, struct User* user);
void* update_product_file(void* args_void);
bool read_product_details(FILE* file, struct Product* product, struct Order* order);
void parse_date_time(char* date_time_str, struct Product* product);
void read_additional_product_info(FILE* file, struct Product* product);

// Main function
int main() {
    bool hasWritten = false;
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs userargs;
    struct User user;  // Declare a global 'user' variable
    // struct ThreadDetails* thread_details = malloc(sizeof(struct ThreadDetails));
    // thread_details->tid = tid;  // Store the thread ID
    // thread_details->product = *product;    // Store product details
    // thread_details->new_score = -1;        // Initialize new_score with invalid value
    // thread_details->updated = 0;           // No update yet
    // pthread_cond_init(&thread_details->cond_var, NULL);
    // pthread_mutex_init(&thread_details->mutex, NULL);
    // do {
    // create process for user
    int orderNum = get_user_details(&user);
    printf("User Input Completed!\n");
    for (int i = 0; i < orderNum; i++) {
        userargs.user = &user;
        userargs.file_path = "./Dataset";
        hasWritten = handle_all_stores(&userargs);
    }

    if (hasWritten) {
        pthread_mutex_lock(&file_mutex);
        FILE *file = fopen("bestlist.dat", "rb");
        if (file) {
            fread(&userargs, sizeof(struct HandleArgs), 1, file);
            fclose(file);
            struct Product* product = &userargs.product;
            printf("The Content On bestlist.dat\n");
            // printf("  file_path: %s\n", userargs.file_path);
            printf("  Name: %s\n", userargs.product.name);
            printf("  Price: %.2f\n", userargs.product.price);
            printf("  Score: %.2f\n", userargs.product.score);
            printf("  Entity: %d\n", userargs.product.entity);
            printf("  Cost: %.2f\n", userargs.product.cost);
            printf("  Date: %04d-%02d-%02d\n", userargs.product.date.year, userargs.product.date.month, userargs.product.date.day);
            printf("  Time: %02d:%02d\n", userargs.product.time.hour, userargs.product.time.minutes);
            printf("------------------------------------\n");
            // check_and_ask_to_buy(&args, &user);
            
        } else {
            printf("Error finding file");
        }
        pthread_mutex_unlock(&file_mutex);
    }
    // } while(1);
    return 0;
}

char** get_slices_input(char *inputstr, int *num_tokens , char *splitter) {
    char **result = malloc(10 * sizeof(char *));  // allocate for 10 tokens
    if (result == NULL) {
        perror("Memory allocation failed for combinedstr");
        exit(EXIT_FAILURE);
    }
    char *token = strtok(inputstr, splitter);
    int index = 0;

    while (token != NULL) {
        result[index++] = strdup(token);  // duplicate the token to store it
        token = strtok(NULL, splitter);
    }
    *num_tokens = index;
    return result;
}

int get_user_details(struct User* user) {
    user->priceThreshold = -1.0;  // Initialize the priceThreshold
    int count = 0;
    
    // Ask for username
    printf("Username: ");
    fgets(user->username, sizeof(user->username), stdin);
    user->username[strcspn(user->username, "\n")] = 0; // Remove newline character

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
                user->priceThreshold = -1;
                // If user just hits Enter, priceThreshold stays -1
                break;
            }

            // Convert input to float for priceThreshold
            user->priceThreshold = strtof(input, NULL);
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

        char *combinedstr = malloc(strlen(tokens[0]) + 1);
        if (combinedstr == NULL) {
            perror("Memory allocation failed for combinedstr");
            exit(EXIT_FAILURE);
        }
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
            strncpy(user->orderlist[count].name, combinedstr, sizeof(user->orderlist[count].name) - 1);
            user->orderlist[count].name[sizeof(user->orderlist[count].name) - 1] = '\0';  // Ensure null-termination
            user->orderlist[count].quantity = quantity;
            count++;
        }

        // Free the allocated memory for tokens and combined string
        free(combinedstr);
        for (int i = 0; i < num_tokens; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    user->totalOrder = count;
    return count;
}

void* handle_file(void* args_void) {
    pthread_mutex_t dat_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs* args = (struct HandleArgs*)args_void;
    struct Product* product = &args->product;
    struct User* user = args->user;
    char* file_path = args->file_path;

    /* used for logger logic
        pid_t tid = syscall(SYS_gettid);
        pid_t category_pid = getpid();
        char formatted_file_id[13];
        snprintf(formatted_file_id, sizeof(formatted_file_id), "%06dID", tid);
        printf("PID %jd created thread for %s TID: %jd\n",
                (intmax_t)category_pid, 
                formatted_file_id, 
                (intmax_t)syscall(SYS_gettid));
    */

    pthread_mutex_lock(&file_mutex);

    // Open file and read product details
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_path);
        pthread_mutex_unlock(&file_mutex);
        pthread_exit(NULL);
    }

    memset(product, 0, sizeof(struct Product));

    for (int i = 0; i < user->totalOrder; i++) {
        if (read_product_details(file, product, &user->orderlist[i])) {
            fclose(file);
            
            // Open file again to read additional details
            FILE* file2 = fopen(file_path, "r");
            if (file2 == NULL) {
                printf("Error opening file: %s\n", file_path);
                pthread_mutex_unlock(&file_mutex);
                pthread_exit(NULL);
            }

            read_additional_product_info(file2, product);
            fclose(file2);

            // If a valid product was found, calculate its cost
            product->cost = product->score * product->price;

            char* dat_path = malloc(MAX_PATH_LEN * sizeof(char));
            if (dat_path == NULL) {
                perror("malloc failed");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_lock(&dat_mutex);
            snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", args->storeIndex);
            FILE *file = fopen(dat_path, "ab");
            if (file != NULL) {
                fwrite(args, sizeof(struct HandleArgs), 1, file);
                fclose(file);
                pthread_mutex_unlock(&dat_mutex);
            } else {
                perror("Failed to write to file");
            }
            pthread_mutex_unlock(&dat_mutex);

        } else {
            fclose(file);
        }
    }

    pthread_mutex_unlock(&file_mutex);

    return NULL;
}

void* handle_category(struct HandleArgs* args) {
    pthread_mutex_t dat_mutex = PTHREAD_MUTEX_INITIALIZER;
    char* file_path = args->file_path;
    char* dat_path;

    DIR * d = opendir(args->file_path);
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

    d = opendir(args->file_path);
    if (d == NULL) {
        perror("Failed to reopen category directory");
        return NULL;
    }
    // Create a thread for each file in the category
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_DIR) {
            // Allocate memory for thread_args for each thread to ensure thread-safety
            struct HandleArgs* thread_args = malloc(sizeof(struct HandleArgs));
            if (thread_args == NULL) {
                perror("Failed to allocate memory for thread_args");
                closedir(d);
                // exit(EXIT_FAILURE);
                return NULL;
            }

            thread_args->user = args->user;
            thread_args->product = args->product;
            // Allocate and copy the file path to thread_args
            thread_args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
            snprintf(thread_args->file_path, MAX_PATH_LEN, "%s/%s", file_path, dir->d_name);

            // Create a thread to handle the file
            pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, thread_args);
            pthread_join(threads[thread_index], NULL);

            // struct Product* product = &thread_args->product;
            // if (product->cost != 0) {
                // Print the product info (for debugging)
                // printf("On handle_category :\n");
                // printf("Name : %s\n", product->name);
                // printf("Price : %.2lf\n", product->price);
                // printf("Score : %lf\n", product->score);
                // printf("Entity : %d\n", product->entity);
                // printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
                // printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
                // printf("Cost : %.2lf\n", product->cost);
                // printf("-----------------------------------------\n");
                // Write the modified args to a file
                // printf("storeIndex at category: %d\n", args->storeIndex);
                // thread_args->storeIndex = args->storeIndex;
                // dat_path = malloc(MAX_PATH_LEN * sizeof(char));
                // if (dat_path == NULL) {
                //     perror("malloc failed");
                //     exit(EXIT_FAILURE);
                // }
                // snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", args->storeIndex);
                // printf("dat_path at category: %s\n", dat_path);

                // Lock the mutex before writing to the file
                // pthread_mutex_lock(&dat_mutex);

                // FILE *file = fopen(dat_path, "wb");
                // if (file != NULL) {
                //     fwrite(thread_args, sizeof(struct HandleArgs), 1, file);
                //     fclose(file);
                //     pthread_mutex_unlock(&dat_mutex);
                // } else {
                //     perror("Failed to write to file");
                // }

                // Now write the file_path to a separate .dat file (e.g., storefile_path.dat)
                // char path_dat_filename[MAX_PATH_LEN];
                // snprintf(path_dat_filename, MAX_PATH_LEN, "storepath%d.dat", args->storeIndex);
                // FILE *path_file = fopen(path_dat_filename, "wb");
                // if (path_file != NULL) {
                //     fwrite(thread_args->file_path, sizeof(char), strlen(thread_args->file_path) + 1, path_file);  // +1 for null-terminator
                //     fclose(path_file);
                // } else {
                //     perror("Failed to write file_path to .dat file");
                // }

                // Unlock the mutex after writing
                // pthread_mutex_unlock(&dat_mutex);
            // }

            free(thread_args->file_path);
            free(thread_args);

            thread_index++;
        }
    }
    closedir(d);
    return NULL;
}

void* handle_store(struct HandleArgs* args) {
    char* store_path = (char*) args->file_path;
    char category_paths[8][100] = {
        "Apparel", "Beauty", "Digital", "Food", "Home", "Market", "Sports", "Toys"
    };
    char file_path[MAX_PATH_LEN];
    pid_t category_pid;
    pid_t store_pid = getpid();

    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    pid_t category_pids[3] = {0};  // Array to store PIDs of the child processes

    for (int i = 0; i < 8; i++) {
        args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
        if (args->file_path == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        snprintf(args->file_path, MAX_PATH_LEN, "%s/%s", store_path, category_paths[i]);
        // Create a process for each store
        category_pid = fork();
        switch (category_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                // printf("PID %jd created child for %s PID: %jd\n", (intmax_t)store_pid, category_paths[i], (intmax_t)getpid());
                handle_category(args);
                exit(EXIT_SUCCESS);
            default:
                category_pids[i] = category_pid;
                // exit(EXIT_SUCCESS);
                break;
        }
    }
    // Wait for all store processes to finish
    // for (int i = 0; i < 3; i++) {
    //     wait(NULL);
    // }
    for (int i = 0; i < 3; i++) {
        int status;
        if (category_pids[i] != 0 && (category_pids[i], &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            // printf("Child process %jd (Store%d) terminated successfully.\n", 
            //        (intmax_t)category_pids[i], i + 1);
        } else {
            printf("Child process %jd (Store%d) terminated with an error.\n", 
                   (intmax_t)category_pids[i], i + 1);
        }
    }
    return NULL;
}

bool handle_all_stores(struct HandleArgs* args) {
    pthread_mutex_t dat_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t dat_mutex2 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t add_mutex = PTHREAD_MUTEX_INITIALIZER;
    char *parent_path = (char*)args->file_path;
    char *dat_path;
    pid_t store_pid;
    int bestCost = -1;
    bool hasWritten = false;
    struct Product* product;

    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    pid_t store_pids[3] = {0};  // Array to store PIDs of the child processes

    // Create child processes for each store
    for (int i = 0; i < 3; i++) {
        args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
        if (args->file_path == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        snprintf(args->file_path, MAX_PATH_LEN, "%s/Store%d", parent_path, i + 1);
        // Fork a child process for each store
        store_pid = fork();
        switch (store_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                // Child process
                pthread_mutex_lock(&add_mutex);
                args->storeIndex = i + 1;
                pthread_mutex_unlock(&add_mutex);
                handle_store(args);  // Process the store
                exit(EXIT_SUCCESS);  // Exit child process after completing the task
            default:
                // Parent process stores the child PID to wait for it later
                store_pids[i] = store_pid;
                // printf("PID %jd created child for Store%d PID:%jd\n", 
                //        (intmax_t)getppid(), i + 1, (intmax_t)store_pid);
                break;
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++) {
        int status;
        if (store_pids[i] != 0 && (store_pids[i], &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            // printf("Child process %jd (Store%d) terminated successfully.\n", 
            //        (intmax_t)store_pids[i], i + 1);
        } else {
            printf("Child process %jd (Store%d) terminated with an error.\n", 
                   (intmax_t)store_pids[i], i + 1);
        }
    }

    // Now that all child processes are done, you can safely read from the .dat files
    for (int i = 0; i < 3; i++) {
        dat_path = malloc(MAX_PATH_LEN * sizeof(char));
        if (dat_path == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }
        snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", i + 1);
        pthread_mutex_lock(&dat_mutex);
        FILE *file = fopen(dat_path, "rb");
        if (file != NULL) {
            fread(args, sizeof(struct HandleArgs), 1, file);
            fclose(file);
        } else {
            perror("Failed to read from file");
        }

        product = &args->product;
        if (product->cost > bestCost) {
            bestCost = product->cost;
            FILE *file2 = fopen("bestlist.dat", "ab");
            if (file2 != NULL) {
                fwrite(args, sizeof(struct HandleArgs), 1, file2);
                fclose(file2);
                hasWritten = true;
            } else {
                perror("Failed to write to file");
            }
        }
        pthread_mutex_unlock(&dat_mutex);
        free(dat_path);
    }
    return hasWritten;
}

// void handle_all_stores(struct HandleArgs* args) {
//     pthread_mutex_t dat_mutex = PTHREAD_MUTEX_INITIALIZER;
//     pthread_mutex_t dat_mutex2 = PTHREAD_MUTEX_INITIALIZER;
//     pthread_mutex_t add_mutex = PTHREAD_MUTEX_INITIALIZER;
//     char *parent_path = (char*)args->file_path;
//     char *dat_path;
//     pid_t store_pid;
//     int bestCost = -1;
//     struct Product* product;
//     char bestStorePath[MAX_PATH_LEN] = {0};  // To store the path of the best store

//     if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
//         perror("signal");
//         exit(EXIT_FAILURE);
//     }

//     pid_t store_pids[3] = {0};  // Array to store PIDs of the child processes

//     // Create child processes for each store
//     for (int i = 0; i < 3; i++) {
//         args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
//         if (args->file_path == NULL) {
//             perror("malloc failed");
//             exit(EXIT_FAILURE);
//         }
//         snprintf(args->file_path, MAX_PATH_LEN, "%s/Store%d", parent_path, i + 1);

//         // Fork a child process for each store
//         store_pid = fork();
//         switch (store_pid) {
//             case -1:
//                 perror("fork");
//                 exit(EXIT_FAILURE);
//             case 0:
//                 // Child process
//                 pthread_mutex_lock(&add_mutex);
//                 args->storeIndex = i + 1;
//                 pthread_mutex_unlock(&add_mutex);
//                 handle_store(args);  // Process the store
//                 exit(EXIT_SUCCESS);  // Exit child process after completing the task
//             default:
//                 // Parent process stores the child PID to wait for it later
//                 store_pids[i] = store_pid;
//                 break;
//         }
//     }

//     // Wait for all child processes to finish
//     for (int i = 0; i < 3; i++) {
//         int status;
//         if (store_pids[i] != 0 && (store_pids[i], &status, 0) == -1) {
//             perror("waitpid");
//             exit(EXIT_FAILURE);
//         }
//         if (WIFEXITED(status)) {
//             // Child process terminated successfully
//         } else {
//             printf("Child process %jd (Store%d) terminated with an error.\n", 
//                    (intmax_t)store_pids[i], i + 1);
//         }
//     }

//     // Now that all child processes are done, you can safely read from the .dat files
//     for (int i = 0; i < 3; i++) {
//         // Read from storepathX.dat to get the path of the store
//         char storepath_filename[MAX_PATH_LEN];
//         snprintf(storepath_filename, MAX_PATH_LEN, "storepath%d.dat", i + 1);
        
//         FILE *path_file = fopen(storepath_filename, "rb");
//         if (path_file != NULL) {
//             // Read the file path from the storepathX.dat file
//             char store_path[MAX_PATH_LEN];
//             fread(store_path, sizeof(char), MAX_PATH_LEN, path_file);
//             fclose(path_file);

//             // Now, read from the storeX.dat file to get the product cost
//             dat_path = malloc(MAX_PATH_LEN * sizeof(char));
//             if (dat_path == NULL) {
//                 perror("malloc failed");
//                 exit(EXIT_FAILURE);
//             }
//             snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", i + 1);
            
//             pthread_mutex_lock(&dat_mutex);
//             FILE *file = fopen(dat_path, "rb");
//             if (file != NULL) {
//                 fread(args, sizeof(struct HandleArgs), 1, file);
//                 fclose(file);

//                 product = &args->product;
//                 // Compare the cost with the bestCost
//                 if (product->cost > bestCost) {
//                     bestCost = product->cost;
//                     // Update the best store path
//                     snprintf(bestStorePath, MAX_PATH_LEN, "%s", store_path);
//                 }
//             } else {
//                 perror("Failed to read from store dat file");
//             }
//             pthread_mutex_unlock(&dat_mutex);
//             free(dat_path);
//         } else {
//             perror("Failed to read from storepath.dat");
//         }
//     }

//     // After determining the best store, write the best store path to bestlistpath.dat
//     if (bestCost != -1) {
//         FILE *bestlist_file = fopen("bestlistpath.dat", "wb");
//         if (bestlist_file != NULL) {
//             fwrite(bestStorePath, sizeof(char), strlen(bestStorePath) + 1, bestlist_file);  // Include null terminator
//             fclose(bestlist_file);
//         } else {
//             perror("Failed to write to bestlistpath.dat");
//         }
//     }
// }

void check_and_ask_to_buy(struct HandleArgs* args, struct User* user) {
    struct Product* product = &args->product;
    // printf("The Content On bestlist.dat\n");
    // printf("  Name: %s\n", args->product.name);
    // printf("  Price: %.2f\n", args->product.price);
    // printf("  Score: %.2f\n", args->product.score);
    // printf("  Entity: %d\n", args->product.entity);
    // printf("  Cost: %.2f\n", args->product.cost);
    // printf("  Date: %04d-%02d-%02d\n", args->product.date.year, args->product.date.month, args->product.date.day);
    // printf("  Time: %02d:%02d\n", args->product.time.hour, args->product.time.minutes);
    // printf("------------------------------------\n");
    if (user->priceThreshold != -1) {
        if (product->cost <= user->priceThreshold) {
            char ans;
            printf("Your price threshold allows you to buy this product. Do you want to buy it? (Y or N): ");
            scanf(" %c", &ans);  // Space before %c to consume any newline character
            if (ans == 'Y' || ans == 'y') {
                // Ask for a score between 1 and 5
                int score;
                do {
                    printf("Please rate the product (1-5): ");
                    scanf("%d", &score);
                } while (score < 1 || score > 5);  // Ensure the score is valid
                product->score = score;  // Set the new score
                // Get the current time
                time_t rawtime;
                struct tm * timeinfo;
                
                // Get the current system time
                time(&rawtime);
                
                // Convert the time to local time
                timeinfo = localtime(&rawtime);
                
                // Set the product's date and time to the current date and time
                product->date.year = 1900 + timeinfo->tm_year;  // tm_year is years since 1900
                product->date.month = timeinfo->tm_mon + 1;     // tm_mon is 0-based
                product->date.day = timeinfo->tm_mday;
                
                product->time.hour = timeinfo->tm_hour;
                product->time.minutes = timeinfo->tm_min;
                product->time.seconds = timeinfo->tm_sec;

                // Create a thread to update the product's file with the new score
                // pthread_t update_thread;

                // Create a struct to pass to the thread
                // struct HandleArgs* update_args = malloc(sizeof(struct HandleArgs));
                // memcpy(update_args, args, sizeof(struct HandleArgs));  // Copy the args
                // update_args->product = *product;  // Pass the updated product

                // pthread_create(&update_thread, NULL, update_product_file, update_args);
                // pthread_join(update_thread, NULL);  // Wait for the thread to complete
            } else {
                printf("You chose not to buy this product.\n");
            }
        } else {
            printf("You cannot buy this product, as it exceeds your price threshold.\n");
        }
    } else {
        printf("No price threshold set, unable to check.\n");
    }
}

void* update_product_file(void* args_void) {
    struct HandleArgs* args = (struct HandleArgs*)args_void;
    struct ThreadDetails* thread_details = &args->user_details;  // Pass user details (ThreadDetails)

    // Suppose the user has rated the product (score between 1-5)
    int user_score = 4;  // Example user score (you can prompt user for this)
    
    // Update the score in the ThreadDetails
    pthread_mutex_lock(&thread_details->mutex);  // Lock before updating
    thread_details->new_score = user_score;
    thread_details->updated = 1;  // Indicate that the thread can be woken up

    // Signal the sleeping thread to wake up and process the update
    pthread_cond_signal(&thread_details->cond_var);

    pthread_mutex_unlock(&thread_details->mutex);  // Unlock after update

    return NULL;
}

bool read_product_details(FILE* file, struct Product* product, struct Order* order) {
    char line[256];
    bool isNameFound = false;
    bool isEntityValid = false;
    char* namestr = NULL;
    int entity = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        if (strncmp(line, "Name:", 5) == 0) {
            namestr = line + 6;
            if (strcmp(namestr, order->name) == 0) {
                strcpy(product->name, namestr);
                isNameFound = true;
                break;
            }
        } else if (strncmp(line, "Entity:", 7) == 0) {
            entity = atoi(line + 8);
            if (isNameFound && order->quantity <= entity) {
                product->entity = entity;
                isEntityValid = true;
                break;
            }
        }
    }
    return isNameFound && isEntityValid;
}

// Function to read additional product info (price, score, last modified)
void read_additional_product_info(FILE* file, struct Product* product) {
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "Price:", 6) == 0) {
            product->price = atof(line + 7);
        } else if (strncmp(line, "Score:", 6) == 0) {
            product->score = atof(line + 7);
        } else if (strncmp(line, "Last Modified:", 14) == 0) {
            parse_date_time(line + 15, product);
        }
    }
}

// Function to parse date and time from the string
void parse_date_time(char* date_time_str, struct Product* product) {
    char* date_str = strtok(date_time_str, " ");
    char* time_str = strtok(NULL, " ");

    if (date_str && time_str) {
        int date_tokens_count = 0;
        char** date_tokens = get_slices_input(date_str, &date_tokens_count, "-");
        if (date_tokens_count == 3) {
            product->date.year = atoi(date_tokens[0]);
            product->date.month = atoi(date_tokens[1]);
            product->date.day = atoi(date_tokens[2]);
        }
        free(date_tokens);

        int time_tokens_count = 0;
        char** time_tokens = get_slices_input(time_str, &time_tokens_count, ":");
        if (time_tokens_count == 3) {
            product->time.hour = atoi(time_tokens[0]);
            product->time.minutes = atoi(time_tokens[1]);
            product->time.seconds = atoi(time_tokens[2]);
        }
        free(time_tokens);
    }
}

