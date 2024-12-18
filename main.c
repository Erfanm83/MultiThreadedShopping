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


#define MAX_FILENAME 1000
#define MAX_MESSAGE 1000

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
    struct Product productlist[MAX_PRODUCTS];
    struct ThreadDetails user_details;
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
int get_user_details(struct User* user);
bool handle_all_stores(struct HandleArgs* args);
void* handle_store(struct HandleArgs* args);
void* handle_file(void* args);
// void check_and_ask_to_buy(struct HandleArgs* args, struct User* user);
void* update_product_file(void* args_void);
int read_product_details(char* file_path, struct Product* product, struct Order* order);
void parse_date_time(char* date_time_str, struct Product* product);
void read_additional_product_info(FILE* file, struct Product* product);

int create_file_log(char *name, int order);
int file_exists(const char *filename);
int order_Chandom(char *name, int kodom);
int Log(struct User* user);
void Write_in_log(char *message, char *filename);

// Main function
int main() {
    bool hasWritten = false;
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs userargs;
    struct User user;
    
    // do {
    // create process for user
    int orderNum = get_user_details(&user);
    printf("User Input Completed!\n");
    userargs.user = &user;
    userargs.file_path = "./Dataset";
    hasWritten = handle_all_stores(&userargs);
    // for (int i = 0; i < orderNum; i++) {
        
    // }

    // if (hasWritten) {
    //     // Lock the mutex before accessing the shared file
    //     pthread_mutex_lock(&file_mutex);

    //     // Open bestlist.dat to read the stored contents
    //     FILE *file2 = fopen("bestlist.dat", "rb");
    //     if (file2 != NULL) {
    //         int bestStore = -1;
    //         int bestCost = 0;
            
    //         // Read the bestStore and bestCost
    //         if (fread(&bestStore, sizeof(int), 1, file2) != 1) {
    //             perror("Failed to read bestStore from file");
    //             fclose(file2);
    //             pthread_mutex_unlock(&file_mutex); // Unlock before returning
    //             return -1;
    //         }
    //         if (fread(&bestCost, sizeof(int), 1, file2) != 1) {
    //             perror("Failed to read bestCost from file");
    //             fclose(file2);
    //             pthread_mutex_unlock(&file_mutex); // Unlock before returning
    //             return -1;
    //         }

    //         // Print the best store and cost
    //         printf("Best Store: %d\n", bestStore);
    //         printf("Best Cost: %d\n", bestCost);

    //         // Read the HandleArgs structure
    //         if (fread(&userargs, sizeof(struct HandleArgs), 1, file2) != 1) {
    //             perror("Failed to read HandleArgs structure from file");
    //             fclose(file2);
    //             pthread_mutex_unlock(&file_mutex); // Unlock before returning
    //             return -1;
    //         }

    //         // Print details from HandleArgs (excluding file_path for now)
    //         printf("HandleArgs Details:\n");
    //         printf("User Address: %p\n", userargs.user);  // For demonstration, you may print other fields as needed.

    //         // Read the file_path string (it’s stored as a null-terminated string)
    //         char filePath[256]; // Buffer to store file_path string
    //         if (fread(filePath, sizeof(char), 256, file2) != 1) {
    //             perror("Failed to read file_path from file");
    //             fclose(file2);
    //             pthread_mutex_unlock(&file_mutex); // Unlock before returning
    //             return -1;
    //         }
    //         printf("File Path: %s\n", filePath);

    //         fclose(file2);
    //     } else {
    //         perror("Failed to open bestlist.dat for reading");
    //     }

    //     // Unlock the mutex after accessing the shared file
    //     pthread_mutex_unlock(&file_mutex);
    // }
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

    Log(user);

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
    int answer = -1;
    int storeNumber;
    bool flag = false;

    pid_t tid = syscall(SYS_gettid);
    pid_t category_pid = getpid();
    char formatted_file_id[13];
    char filename[MAX_FILENAME];
    int order_chandommm = order_Chandom(args->user->username, 0);
    sprintf(filename, "./Dataset/%s_order%d.log", args->user->username, order_chandommm);
    char message[MAX_MESSAGE];
    sprintf(message, "PID %jd created thread for %s TID: %jd\n",
            (intmax_t)category_pid,
            formatted_file_id,
            (intmax_t)syscall(SYS_gettid));
    Write_in_log(message, filename);

    pthread_mutex_lock(&file_mutex);

    // printf("user->totalOrder %d\n",user->totalOrder);

    for (int i = 0; i < user->totalOrder; i++) {
        answer = read_product_details(file_path, product, &user->orderlist[i]);
        if (answer % 10 == 1) {
            printf("file_path %s \n", file_path);
            // Open file again to read additional details
            FILE* file2 = fopen(file_path, "r");
            if (file2 == NULL) {
                printf("Error opening file: %s\n", file_path);
                pthread_mutex_unlock(&file_mutex);
                pthread_exit(NULL);
            }

            read_additional_product_info(file2, product);
            fclose(file2);
            product->cost = product->score * product->price;

            memcpy(&args->productlist[i], product, sizeof(struct Product));
            storeNumber = answer / 10;

            
            // printf("dakhel if flag\n");
            
            pthread_mutex_lock(&dat_mutex);
            char* store_path = malloc(MAX_PATH_LEN * sizeof(char));
            if (store_path == NULL) {
                perror("malloc failed");
                pthread_exit(NULL);
            }
            snprintf(store_path, MAX_PATH_LEN, "store%d.txt" , storeNumber);
            FILE *file3 = fopen(store_path, "a+");
            if (file3 != NULL) {
                char line[256];
                rewind(file3);
                if (fgets(line, sizeof(line), file3) == NULL || strncmp(line, "Store", 5) != 0) {
                    fprintf(file3, "Store %d\n", storeNumber);
                    fprintf(file3, "username: %s\n", args->user->username);
                    fprintf(file3, "file path : %s\n", args->file_path);
                    fprintf(file3, "------------------------------------\n");
                }
                fprintf(file3, "productlist : \n");
                if (product->price != 0.0) {
                    fprintf(file3, "  Name: %s\n", product->name);
                    fprintf(file3, "  Price: %.2f\n", product->price);
                    fprintf(file3, "  Score: %.2f\n", product->score);
                    fprintf(file3, "  Entity: %d\n", product->entity);
                    fprintf(file3, "  Cost: %.2f\n", product->cost);
                    fprintf(file3, "  Date: %04d-%02d-%02d\n", product->date.year, product->date.month, product->date.day);
                    fprintf(file3, "  Time: %02d:%02d\n", product->time.hour, product->time.minutes);
                    fprintf(file3, "  file_path: %s\n", args->file_path);
                    fprintf(file3, "------------------------------------\n");
                }
                fclose(file3);
                pthread_mutex_unlock(&dat_mutex);
            } else {
                perror("Failed to write to file");
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&dat_mutex);
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
            memcpy(thread_args, args, sizeof(struct HandleArgs));
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

            // printf("args->storeIndex in handle_category: %d\n", args->storeIndex);
            // Create a thread to handle the file
            pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, thread_args);

            // sleep thread
            pthread_join(threads[thread_index], NULL);

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
        // printf("args->storeIndex in handle_store: %d\n", args->storeIndex);
        // Create a process for each store
        category_pid = fork();
        switch (category_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                char filename[MAX_FILENAME];
                int order_chandommm = order_Chandom(args->user->username,0);
                sprintf(filename, "./Dataset/%s_order%d.log", args->user->username, order_chandommm);
                char message[MAX_MESSAGE];
                sprintf(message, "PID %jd created child for %s PID: %jd\n", (intmax_t)store_pid, category_paths[i], (intmax_t)getpid());
                Write_in_log(message, filename);

                handle_category(args);
                exit(EXIT_SUCCESS);
            default:
                category_pids[i] = category_pid;
                // exit(EXIT_SUCCESS);
                break;
        }
    }
    // Wait for all store processes to finish
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
    int bestCost, bestStore;
    bool hasWritten = false;
    struct Product* product;
    struct Product *productlist[MAX_PRODUCTS] = {0};
    float costStores[3] = {0};

    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    pid_t store_pids[3] = {0};  // Array to store PIDs of the child processes

    char filename[MAX_FILENAME];
    int order_chandommm = order_Chandom(args->user->username,0);
    sprintf(filename, "./Dataset/%s_order%d.log", args->user->username, order_chandommm);
    char message[MAX_MESSAGE];
    sprintf(message, "%s created PID: %jd \n", args->user->username,
            (intmax_t)getppid());

    Write_in_log(message, filename);

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
                char filename[MAX_FILENAME];
                int order_chandommm = order_Chandom(args->user->username,0);
                sprintf(filename, "./Dataset/%s_order%d.log", args->user->username, order_chandommm);
                char message[MAX_MESSAGE];
                sprintf(message, "PID %jd created child for Store%d PID:%jd\n",
                       (intmax_t)getppid(), i + 1, (intmax_t)store_pid);

                Write_in_log(message, filename);
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
    // for (int i = 0; i < 3; i++) {
    //     // read store%d.dat file
    //     dat_path = malloc(MAX_PATH_LEN * sizeof(char));
    //     if (dat_path == NULL) {
    //         perror("malloc failed");
    //         exit(EXIT_FAILURE);
    //     }
    //     snprintf(dat_path, MAX_PATH_LEN, "store%d.dat", i + 1);
    //     pthread_mutex_lock(&dat_mutex);
    //     FILE *file = fopen(dat_path, "rb");
    //     if (file != NULL) {
    //         fread(productlist, sizeof(struct Product), MAX_PRODUCTS, file);
    //         fclose(file);
    //     } else {
    //         perror("Failed to read from file");
    //     }

    //     // read product objects of productlist
    //     for (int j = 0; j < args->user->totalOrder; j++) {
    //         productlist[j] = malloc(sizeof(struct Product));
    //         if (productlist[j] == NULL) {
    //             perror("malloc failed");
    //             exit(EXIT_FAILURE);
    //         }
    //         memcpy(product, productlist[j], sizeof(struct Product));  // Copy product data into allocated space

    //         costStores[i] += product->cost;
    //     }
    // }
    pthread_mutex_unlock(&dat_mutex);
    free(dat_path);
    return hasWritten;
}

// void check_and_ask_to_buy(struct HandleArgs* args, struct User* user) {
//     struct Product* product = &args->product;
//     // printf("The Content On bestlist.dat\n");
//     // printf("  Name: %s\n", args->product.name);
//     // printf("  Price: %.2f\n", args->product.price);
//     // printf("  Score: %.2f\n", args->product.score);
//     // printf("  Entity: %d\n", args->product.entity);
//     // printf("  Cost: %.2f\n", args->product.cost);
//     // printf("  Date: %04d-%02d-%02d\n", args->product.date.year, args->product.date.month, args->product.date.day);
//     // printf("  Time: %02d:%02d\n", args->product.time.hour, args->product.time.minutes);
//     // printf("------------------------------------\n");
//     if (user->priceThreshold != -1) {
//         if ( <= user->priceThreshold) {
//             char ans;
//             printf("Your price threshold allows you to buy this product. Do you want to buy it? (Y or N): ");
//             scanf(" %c", &ans);  // Space before %c to consume any newline character
//             if (ans == 'Y' || ans == 'y') {
//                 // Ask for a score between 1 and 5
//                 int score;
//                 do {
//                     printf("Please rate the product (1-5): ");
//                     scanf("%d", &score);
//                 } while (score < 1 || score > 5);  // Ensure the score is valid
//                 product->score = score;  // Set the new score
//                 // Get the current time
//                 time_t rawtime;
//                 struct tm * timeinfo;
                
//                 // Get the current system time
//                 time(&rawtime);
                
//                 // Convert the time to local time
//                 timeinfo = localtime(&rawtime);
                
//                 // Set the product's date and time to the current date and time
//                 product->date.year = 1900 + timeinfo->tm_year;  // tm_year is years since 1900
//                 product->date.month = timeinfo->tm_mon + 1;     // tm_mon is 0-based
//                 product->date.day = timeinfo->tm_mday;
                
//                 product->time.hour = timeinfo->tm_hour;
//                 product->time.minutes = timeinfo->tm_min;
//                 product->time.seconds = timeinfo->tm_sec;

//                 // Create a thread to update the product's file with the new score
//                 // pthread_t update_thread;

//                 // Create a struct to pass to the thread
//                 // struct HandleArgs* update_args = malloc(sizeof(struct HandleArgs));
//                 // memcpy(update_args, args, sizeof(struct HandleArgs));  // Copy the args
//                 // update_args->product = *product;  // Pass the updated product

//                 // pthread_create(&update_thread, NULL, update_product_file, update_args);
//                 // pthread_join(update_thread, NULL);  // Wait for the thread to complete
//             } else {
//                 printf("You chose not to buy this product.\n");
//             }
//         } else {
//             printf("You cannot buy this product, as it exceeds your price threshold.\n");
//         }
//     } else {
//         printf("No price threshold set, unable to check.\n");
//     }
// }

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

int read_product_details(char* file_path, struct Product* product, struct Order* order) {
    char line[256];
    bool isNameFound = false;
    bool isEntityValid = false;
    char* namestr = NULL;
    int entity = 0;
    int ans = 0;

    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_path);
        pthread_exit(NULL);
    }

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        if (strncmp(line, "Name:", 5) == 0) {
            namestr = line + 6;
            if (strcmp(namestr, order->name) == 0) {
                strcpy(product->name, order->name);
                isNameFound = true;
                // break;
            }
        }
        if (strncmp(line, "Entity:", 7) == 0) {
            entity = atoi(line + 8);
            if (isNameFound && order->quantity <= entity) {
                product->entity = entity;
                isEntityValid = true;
                break;
            }
        }
    }
    fclose(file);
    if (isNameFound && isEntityValid) {
        ans += 1;
        ans += 10 * ((int)file_path[15] - 48);
    }
    return ans;
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

int create_file_log(char *name, int order){
    char filename[200];
    sprintf(filename, "./Dataset/%s_order%d.log", name, order);
    FILE *logFile = fopen(filename, "a");

    if (logFile == NULL) {
        printf("خطا در باز کردن فایل.\n");
        return 1;
    }

    fclose(logFile);
    return 0;
}

int file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1; // فایل وجود دارد
    }
    return 0; // فایل وجود ندارد
}

int order_Chandom(char *name, int kodom){
    int order = 0;
    char filename[200];

    // بررسی نام فایلهای موجود در دایرکتوری ./Dataset/
    while (1) {
        sprintf(filename, "./Dataset/%s_order%d.log", name, order);
        if (!file_exists(filename)) {
            // اگر فایل وجود نداشته باشد، شماره سفارش پیدا شده است
            break;
        }
        order++;  // اگر فایل وجود داشت، شماره سفارش را افزایش بده
    }

    if(kodom == 0){
        order--;
        return order;
    }else if (kodom == 1)
    {
        return order;
    }

}

int Log(struct User* user){

    char *name = user->username;
    int Add_order = order_Chandom(name, 1);

    int check = create_file_log(name, Add_order);

}

void Write_in_log(char *message, char *filename){
    FILE *logFile = fopen(filename, "a");

    if (logFile == NULL) {
        printf("خطا در باز کردن فایل.\n");
        return ;
    }

    fprintf(logFile, "%s", message);

    fclose(logFile);

}
