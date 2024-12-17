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
};

struct HandleArgs {
    struct User* user;
    char* file_path;
    struct Product productList[MAX_PRODUCTS];
    int storeIndex;
    struct Product product;
    struct ThreadDetails user_details;
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
void get_user_details(struct User* user);
void handle_all_stores(struct HandleArgs* args);
void* handle_store(struct HandleArgs* args);
void* handle_file(void* args);
void check_and_ask_to_buy(struct HandleArgs* args, struct User* user);
void* update_product_file(void* args_void);

// Main function
int main() {
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct User user = {.priceThreshold = -1};  // Declare a global 'user' variable
    // struct ThreadDetails* thread_details = malloc(sizeof(struct ThreadDetails));
    // thread_details->tid = tid;  // Store the thread ID
    // thread_details->product = *product;    // Store product details
    // thread_details->new_score = -1;        // Initialize new_score with invalid value
    // thread_details->updated = 0;           // No update yet
    // pthread_cond_init(&thread_details->cond_var, NULL);
    // pthread_mutex_init(&thread_details->mutex, NULL);
    // do {
    // create process for user
    get_user_details(&user);

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
    struct HandleArgs args;
    args.user = &user;
    args.file_path = "./Dataset";
    handle_all_stores(&args);

    // sleep(5);
    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen("bestlist.dat", "rb");
    if (file)
    {
        fread(&args, sizeof(struct HandleArgs), 1, file);
        fclose(file);
        struct Product* product = &args.product;
        printf("The Content On bestlist.dat\n");
        printf("  Name: %s\n", args.product.name);
        printf("  Price: %.2f\n", args.product.price);
        printf("  Score: %.2f\n", args.product.score);
        printf("  Entity: %d\n", args.product.entity);
        printf("  Cost: %.2f\n", args.product.cost);
        printf("  Date: %04d-%02d-%02d\n", args.product.date.year, args.product.date.month, args.product.date.day);
        printf("  Time: %02d:%02d\n", args.product.time.hour, args.product.time.minutes);
        printf("------------------------------------\n");
        check_and_ask_to_buy(&args, &user);
        
    } else {
        printf("Error finding file");
    }
    pthread_mutex_unlock(&file_mutex);

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

void get_user_details(struct User* user) {
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
}

void* handle_file(void* args_void) {
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs* args = (struct HandleArgs*)args_void;
    struct Product* product = &args->product;
    struct User* user = args->user;
    char* file_path = args->file_path;

    pid_t tid = syscall(SYS_gettid);
    pid_t category_pid = getpid();

    // Create a structure to store thread details
    struct ThreadDetails* thread_details = malloc(sizeof(struct ThreadDetails));
    thread_details->tid = tid;  // Store the thread ID
    thread_details->product = *product;    // Store product details
    thread_details->new_score = -1;        // Initialize new_score with invalid value
    thread_details->updated = 0;           // No update yet
    pthread_cond_init(&thread_details->cond_var, NULL);
    pthread_mutex_init(&thread_details->mutex, NULL);

    char* namestr = NULL;
    int entity = 0;
    bool isNameFound = false;
    bool isEntityValid = false;
    bool isProductFound = false;
    char line[256];

    // Lock the file handling section to ensure exclusive access
    pthread_mutex_lock(&file_mutex);
    int orderIndex;

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_path);
        pthread_mutex_unlock(&file_mutex);
        pthread_exit(NULL);
    } else {
        memset(product, 0, sizeof(struct Product));
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            if (strncmp(line, "Name:", 5) == 0) {
                namestr = line + 6;  // Skip the "Name: " part
                for (int i = 0; i < MAX_PRODUCTS; i++) {
                    if (strcmp(namestr, user->orderlist[i].name) == 0) {
                        strcpy(product->name, namestr);
                        isNameFound = true;
                        orderIndex = i;
                        break;
                    }
                }
            }
            else if (strncmp(line, "Entity:", 7) == 0) {
                entity = atoi(line + 8);
                if (isNameFound && user->orderlist[orderIndex].quantity <= entity) {
                    product->entity = entity;
                    isEntityValid = true;
                    break;
                }
            }
        }
        fclose(file);

        char line2[256];
        FILE *file2 = fopen(file_path, "r");
        if (file2 == NULL) {
            printf("Error opening file: %s\n", file_path);
            pthread_mutex_unlock(&file_mutex);
            pthread_exit(NULL);
        } else {
            if (isNameFound && isEntityValid && !isProductFound) {
                while (fgets(line2, sizeof(line2), file2)) {
                    line2[strcspn(line2, "\n")] = 0;
                    if (strncmp(line2, "Price:", 6) == 0) {
                        product->price = atof(line2 + 7);
                    } else if (strncmp(line2, "Score:", 6) == 0) {
                        product->score = atof(line2 + 7);
                    } else if (strncmp(line2, "Last Modified:", 14) == 0) {
                        char *date_time_str = line2 + 15;
                        char *date_str = strtok(date_time_str, " ");
                        char *time_str = strtok(NULL, " ");

                        if (date_str && time_str) {
                            int date_tokens_count = 0;
                            char **date_tokens = get_slices_input(date_str, &date_tokens_count, "-");
                            if (date_tokens_count == 3) {
                                product->date.year = atoi(date_tokens[0]);
                                product->date.month = atoi(date_tokens[1]);
                                product->date.day = atoi(date_tokens[2]);
                            }
                            free(date_tokens);

                            int time_tokens_count = 0;
                            char **time_tokens = get_slices_input(time_str, &time_tokens_count, ":");
                            if (time_tokens_count == 3) {
                                product->time.hour = atoi(time_tokens[0]);
                                product->time.minutes = atoi(time_tokens[1]);
                                product->time.seconds = atoi(time_tokens[2]);
                            }
                            free(time_tokens);
                        }
                    }
                }
                isProductFound = true;
            }
        }
        fclose(file2);

        // If a valid product was found, print its details
        if (isProductFound) {
            product->cost = product->score*product->price;
        }
    }
    pthread_mutex_unlock(&file_mutex);

    // printf("thread_details->updated %d\n" , thread_details->updated);

    // Sleep this thread (waiting for update)
    while (!thread_details->updated) {
        printf("Waiting for GORGALII");
        pthread_cond_wait(&thread_details->cond_var, &thread_details->mutex);
    }

    // Lock the mutex before doing file operations
    pthread_mutex_lock(&thread_details->mutex);

    // Once woken up, update the product score and last-modified
    if (thread_details->new_score != -1) {
        // Calculate the new score
        product->score = (product->score + thread_details->new_score) / 2.0;
        // Update the last-modified date/time
        product->date.year = thread_details->product.date.year;
        product->date.month = thread_details->product.date.month;
        product->date.day = thread_details->product.date.day;

        product->time.hour = thread_details->product.time.hour;  // Example modification (you can use current time here)
        product->time.minutes = thread_details->product.time.minutes;
        product->time.seconds = thread_details->product.time.seconds;
        
        // Write the updated product info to the file
        FILE *file = fopen(file_path, "r+");
        if (file != NULL) {
            char line[256];
            int found_product = 0;
            while (fgets(line, sizeof(line), file)) {
                line[strcspn(line, "\n")] = 0;  // Remove newline character

                // If product found, update score and last-modified
                if (strncmp(line, "Name:", 5) == 0) {
                    char* product_name = line + 6;
                    if (strcmp(product_name, product->name) == 0) {
                        found_product = 1;
                        fseek(file, -strlen(line), SEEK_CUR);  // Move to the product's position
                        fprintf(file, "Name: %s\n", product->name);
                        fprintf(file, "Price: %.2f\n", product->price);
                        fprintf(file, "Score: %.2f\n", product->score);
                        fprintf(file, "Entity: %d\n", product->entity);
                        fprintf(file, "Last Modified: %d-%02d-%02d %02d:%02d:%02d\n",
                                product->date.year, product->date.month, product->date.day,
                                product->time.hour, product->time.minutes, product->time.seconds);
                        fflush(file);  // Make sure the data is written immediately
                        break;
                    }
                }
            }
            fclose(file);
        }
    }

    // Cleanup and unlock mutex
    pthread_mutex_unlock(&thread_details->mutex);

    // Free the thread details structure after use
    free(thread_details);

    return NULL;
}


// void* handle_file(void* args_void) {
//     pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
//     struct HandleArgs* args = (struct HandleArgs*)args_void;
//     struct User* user = args->user;
//     char* file_name = args->file_path;
//     struct Product* product = &args->product;

//     pid_t tid = syscall(SYS_gettid);
//     pid_t category_pid = getpid();

//     // char formatted_file_id[13];
//     // snprintf(formatted_file_id, sizeof(formatted_file_id), "%06dID", tid);

//     // printf("PID %jd created thread for %s TID: %jd\n",
//     //         (intmax_t)category_pid, 
//     //         formatted_file_id, 
//     //         (intmax_t)syscall(SYS_gettid));

//     char* namestr = NULL;
//     int entity = 0;
//     bool isNameFound = false;
//     bool isEntityValid = false;
//     bool isProductFound = false;
//     char line[256];

//     // Lock the file handling section to ensure exclusive access
//     pthread_mutex_lock(&file_mutex);
//     int orderIndex;

//     FILE *file = fopen(file_name, "r");
//     if (file == NULL) {
//         printf("Error opening file: %s\n", file_name);
//         pthread_mutex_unlock(&file_mutex);
//         pthread_exit(NULL);
//     } else {
//         memset(product, 0, sizeof(struct Product));
//         while (fgets(line, sizeof(line), file)) {
//             line[strcspn(line, "\n")] = 0;
//             if (strncmp(line, "Name:", 5) == 0) {
//                 namestr = line + 6;  // Skip the "Name: " part
//                 for (int i = 0; i < MAX_PRODUCTS; i++) {
//                     if (strcmp(namestr, user->orderlist[i].name) == 0) {
//                         strcpy(product->name, namestr);
//                         isNameFound = true;
//                         orderIndex = i;
//                         break;
//                     }
//                 }
//             }
//             else if (strncmp(line, "Entity:", 7) == 0) {
//                 entity = atoi(line + 8);
//                 if (isNameFound && user->orderlist[orderIndex].quantity <= entity) {
//                     product->entity = entity;
//                     isEntityValid = true;
//                     break;
//                 }
//             }
//         }
//         fclose(file);

//         char line2[256];
//         FILE *file2 = fopen(file_name, "r");
//         if (file2 == NULL) {
//             printf("Error opening file: %s\n", file_name);
//             pthread_mutex_unlock(&file_mutex);
//             pthread_exit(NULL);
//         } else {
//             if (isNameFound && isEntityValid && !isProductFound) {
//                 while (fgets(line2, sizeof(line2), file2)) {
//                     line2[strcspn(line2, "\n")] = 0;
//                     if (strncmp(line2, "Price:", 6) == 0) {
//                         product->price = atof(line2 + 7);
//                     } else if (strncmp(line2, "Score:", 6) == 0) {
//                         product->score = atof(line2 + 7);
//                     } else if (strncmp(line2, "Last Modified:", 14) == 0) {
//                         char *date_time_str = line2 + 15;
//                         char *date_str = strtok(date_time_str, " ");
//                         char *time_str = strtok(NULL, " ");

//                         if (date_str && time_str) {
//                             int date_tokens_count = 0;
//                             char **date_tokens = get_slices_input(date_str, &date_tokens_count, "-");
//                             if (date_tokens_count == 3) {
//                                 product->date.year = atoi(date_tokens[0]);
//                                 product->date.month = atoi(date_tokens[1]);
//                                 product->date.day = atoi(date_tokens[2]);
//                             }
//                             free(date_tokens);

//                             int time_tokens_count = 0;
//                             char **time_tokens = get_slices_input(time_str, &time_tokens_count, ":");
//                             if (time_tokens_count == 3) {
//                                 product->time.hour = atoi(time_tokens[0]);
//                                 product->time.minutes = atoi(time_tokens[1]);
//                                 product->time.seconds = atoi(time_tokens[2]);
//                             }
//                             free(time_tokens);
//                         }
//                     }
//                 }
//                 isProductFound = true;
//             }
//         }
//         fclose(file2);

//         // If a valid product was found, print its details
//         if (isProductFound) {
//             product->cost = product->score*product->price;
//             // printf("Founded Product :\n");
//             // printf("Name : %s\n", product->name);
//             // printf("Price : %.2f\n", product->price);
//             // printf("Score : %.2f\n", product->score);
//             // printf("Entity : %d\n", product->entity);
//             // printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
//             // printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
//             // printf("Cost : %.2f\n", product->cost);
//             // printf("-----------------------------------------\n");
//         }
//     }
//     pthread_mutex_unlock(&file_mutex);
//     return NULL;
// }

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

            // Allocate and copy the file path to thread_args
            thread_args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
            snprintf(thread_args->file_path, MAX_PATH_LEN, "%s/%s", file_path, dir->d_name);

            // Create a thread to handle the file
            pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, thread_args);
            pthread_join(threads[thread_index], NULL);

            struct Product* product = &thread_args->product;
            if (product->cost != 0) {
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
                thread_args->storeIndex = args->storeIndex;
                dat_path = malloc(MAX_PATH_LEN * sizeof(char));
                if (dat_path == NULL) {
                    perror("malloc failed");
                    exit(EXIT_FAILURE);
                }
                snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", args->storeIndex);
                // printf("dat_path at category: %s\n", dat_path);

                // Lock the mutex before writing to the file
                pthread_mutex_lock(&dat_mutex);

                FILE *file = fopen(dat_path, "wb");
                if (file != NULL) {
                    fwrite(thread_args, sizeof(struct HandleArgs), 1, file);
                    fclose(file);
                } else {
                    perror("Failed to write to file");
                }

                // Unlock the mutex after writing
                pthread_mutex_unlock(&dat_mutex);
            }

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

    for (int i = 0; i < 1; i++) {
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
                exit(EXIT_SUCCESS);
                break;
        }
    }
    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
    // // Read the modified args from the file
    // FILE *file = fopen("modified_args.dat", "rb");
    // if (file != NULL) {
    //     fread(args, sizeof(struct HandleArgs), 1, file);
    //     fclose(file);
    // } else {
    //     perror("Failed to read from file");
    // }

    // struct Product* product = &args->product;
    // printf("In handle_store after update:\n");
    // printf("Name: %s\n", product->name);
    // printf("Price: %.2lf\n", product->price);
    // printf("Cost: %.2lf\n", product->cost);

    return NULL;
}

void handle_all_stores(struct HandleArgs* args) {
    pthread_mutex_t dat_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t dat_mutex2 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t add_mutex = PTHREAD_MUTEX_INITIALIZER;
    char *parent_path = (char*)args->file_path;
    char *dat_path;
    pid_t store_pid;
    int bestCost = -1;
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
        // storeIdx = args->storeIndex;

        if (product->cost > bestCost) {
            bestCost = product->cost;
            FILE *file2 = fopen("bestlist.dat", "wb");
            if (file2 != NULL) {
                fwrite(args, sizeof(struct HandleArgs), 1, file);
                fclose(file2);
            } else {
                perror("Failed to write to file");
            }
        }
        pthread_mutex_unlock(&dat_mutex);
        free(dat_path);
    }
    
}

void check_and_ask_to_buy(struct HandleArgs* args, struct User* user) {
    printf("price Threshold : %lf\n" , user->priceThreshold);
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
                pthread_t update_thread;

                // Create a struct to pass to the thread
                struct HandleArgs* update_args = malloc(sizeof(struct HandleArgs));
                memcpy(update_args, args, sizeof(struct HandleArgs));  // Copy the args
                update_args->product = *product;  // Pass the updated product

                pthread_create(&update_thread, NULL, update_product_file, update_args);
                pthread_join(update_thread, NULL);  // Wait for the thread to complete
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

