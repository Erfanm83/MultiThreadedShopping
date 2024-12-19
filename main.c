#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>  // Added this header to use 'wait()'
#include <signal.h>
#include <sys/types.h>  // Required for `struct stat`
#include <sys/stat.h>   // Required for `stat`
#include <inttypes.h>
#include <sys/syscall.h>   // For syscall and gettid
#include <stdbool.h>
#include <time.h>


#define MAX_FILENAME 1000
#define MAX_MESSAGE 1000
#define MAX_THREADS 1024

#define MAX_PRODUCTS 100
#define MAX_USERS 1000
#define MAX_PATH_LEN 1024
#ifndef DT_REG
    #define DT_REG 8
#endif
#ifndef DT_DIR
    #define DT_DIR 3
#endif

// Global Variable
ThreadControl thread_controls[MAX_THREADS];
int thread_count = 0;  // To track the number of threads created
char input[100];

// Global mutex for managing thread controls
pthread_mutex_t thread_controls_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    char file_path[MAX_PATH_LEN];
};

typedef struct ThreadControl {
    pid_t tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadControl;

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
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
int get_user_details(struct User* user);
bool handle_all_stores(struct HandleArgs* args);
void* handle_store(struct HandleArgs* args);
void* handle_file(void* args);
int read_product_details(char* file_path, struct Product* product, struct Order* order);
void parse_date_time(char* date_time_str, struct Product* product);
void read_additional_product_info(FILE* file, struct Product* product);

int create_file_log(char *name, int order);
int file_exists(const char *filename);
int order_Chandom(char *name, int kodom);
int Log(struct User* user);
void Write_in_log(char *message, char *filename);

int file_exists(const char *path);
void wait_for_files();
void calculate_best_cost(struct HandleArgs* args);
void check_and_ask_to_buy(struct HandleArgs* args, struct User* user);
double process_bestlist(int orderNumber, struct HandleArgs* args);
void create_off(const char* username, int order_chandommm, int storeBought);
bool off_file_exists(const char* username, int order_chandommm);
void update_score_in_file(char *input_file);
void wake_thread_by_tid(pid_t target_tid);

// Main function
int main() {
    bool hasWritten = false;
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs userargs;
    struct User user;
    int i;

    // do {
    //     i = 0;
    pid_t user_pids[MAX_USERS] = {0}; 

    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // create process for user
    int orderNum = get_user_details(&user);
    printf("User Input Completed!\n");
    userargs.user = &user;
    userargs.file_path = "./Dataset";
    pid_t user_pid = fork();
    switch (user_pid) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            // Child process
            hasWritten = handle_all_stores(&userargs);
            exit(EXIT_SUCCESS);
        default:
            // Parent process stores the child PID to wait for it later
            user_pids[i] = user_pid;
            // i++;
            break;
    }

    // Wait for child processes
    int status;
    for (i = 0; i < MAX_USERS; i++) {
        if (user_pids[i] != 0) {
            if (waitpid(user_pids[i], &status, 0) == -1) {
                perror("waitpid");
            } else if (WIFEXITED(status)) {
                printf("Child process %jd terminated with status %d.\n", 
                    (intmax_t)user_pids[i], WEXITSTATUS(status));
            } else {
                printf("Child process %jd terminated abnormally.\n", (intmax_t)user_pids[i]);
            }
        }
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

    Log(user);

    user->totalOrder = count;
    return count;
}

void wake_thread_by_tid(pid_t target_tid) {
    pthread_mutex_lock(&thread_controls_mutex);

    for (int i = 0; i < thread_count; i++) {
        if (thread_controls[i].tid == target_tid) {
            // Wake up the thread with the target_tid
            pthread_mutex_lock(&thread_controls[i].mutex);
            pthread_cond_signal(&thread_controls[i].cond);  // Wake up the specific thread
            pthread_mutex_unlock(&thread_controls[i].mutex);
            break;
        }
    }

    pthread_mutex_unlock(&thread_controls_mutex);
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

    // Register the current thread in the thread_controls array
    pthread_mutex_lock(&thread_controls_mutex);
    thread_controls[thread_count].tid = tid;
    pthread_mutex_init(&thread_controls[thread_count].mutex, NULL);
    pthread_cond_init(&thread_controls[thread_count].cond, NULL);
    thread_count++;
    pthread_mutex_unlock(&thread_controls_mutex);

    pthread_mutex_lock(&file_mutex);
    for (int i = 0; i < user->totalOrder; i++) {
        answer = read_product_details(file_path, product, &user->orderlist[i]);
        if (answer % 10 == 1) {
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
                    fprintf(file3, "  tid: %ld\n", (intmax_t) tid);
                    fprintf(file3, "------------------------------------\n");
                }
                fclose(file3);
                pthread_mutex_unlock(&dat_mutex);
            } else {
                perror("Failed to write to file");
            }
            pthread_mutex_unlock(&dat_mutex);

            // Now, sleep this thread because it processed data
            pthread_mutex_lock(&thread_controls[thread_count - 1].mutex);
            pthread_cond_wait(&thread_controls[thread_count - 1].cond, &thread_controls[thread_count - 1].mutex);  // Wait to be woken up
            pthread_mutex_unlock(&thread_controls[thread_count - 1].mutex);

            break;
        }
    }
    pthread_mutex_unlock(&file_mutex);

    return NULL;
}

void* handle_category(struct HandleArgs* args) {
    char* file_path = args->file_path;

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
                return NULL;
            }

            thread_args->user = args->user;
            thread_args->product = args->product;
            // Allocate and copy the file path to thread_args
            thread_args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
            snprintf(thread_args->file_path, MAX_PATH_LEN, "%s/%s", file_path, dir->d_name);

            // Create a thread to handle the file
            pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, thread_args);

            // // sleep thread
            // pthread_join(threads[thread_index], NULL);

            // free(thread_args->file_path);
            // free(thread_args);
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
    pthread_mutex_t add_mutex = PTHREAD_MUTEX_INITIALIZER;
    char *parent_path = (char*)args->file_path;
    // pid_t store_pid;
    struct Product* product;
    struct Product *productlist[MAX_PRODUCTS] = {0};
    float costStores[3] = {0};
    bool hasWritten = false;

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
        pid_t store_pid = fork();
        switch (store_pid) {
            case -1:
                perror("fork");
                free(args->file_path);
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
                break;
        }
    }

    for (int i = 0; i < 3; i++) {
        int status;
        if (store_pids[i] != 0) {
            if (waitpid(store_pids[i], &status, 0) == -1) {
                perror("waitpid");
            } else if (WIFEXITED(status)) {
                printf("Store %d (PID %jd) terminated with status %d.\n", 
                       i + 1, (intmax_t)store_pids[i], WEXITSTATUS(status));
            } else {
                printf("Store %d (PID %jd) terminated abnormally.\n", 
                       i + 1, (intmax_t)store_pids[i]);
            }
        }
    }

    // Wait for all child processes to finish
    // for (int i = 0; i < 3; i++) {
    //     int status;
    //     if (store_pids[i] != 0 && (store_pids[i], &status, 0) == -1) {
    //         perror("waitpid");
    //         exit(EXIT_FAILURE);
    //     }
    //     else {
    //         printf("Child process %jd (Store%d) terminated with an error.\n", 
    //                (intmax_t)store_pids[i], i + 1);
    //     }
    // }

    // Wait for all required files to be created
    wait_for_files();

    // Once all files are created, calculate the best cost
    calculate_best_cost(args);

    check_and_ask_to_buy(args, args->user);
    
    return hasWritten;
}

// Function to check if a file exists
int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Function to wait for all required files to be created
void wait_for_files() {
    int all_files_exist = 0;

    while (!all_files_exist) {
        all_files_exist = 1; // Assume all files exist

        for (int i = 1; i <= 3; i++) {
            char store_path[MAX_PATH_LEN];
            snprintf(store_path, MAX_PATH_LEN, "store%d.txt", i);

            if (!file_exists(store_path)) {
                all_files_exist = 0; // At least one file is missing
                break;
            }
        }

        if (!all_files_exist) {
            printf("Waiting for all store files to be created...\n");
            sleep(1); // Wait for 1 second before checking again
        }
    }

    printf("All required store files are created.\n");
}

// Function to calculate the best cost
void calculate_best_cost(struct HandleArgs* args) {
    char store_path[MAX_PATH_LEN];
    char best_store_path[MAX_PATH_LEN];
    FILE *file;
    FILE *best_file;
    float price, score, cost;
    float best_cost = 0.0;
    char line[256];
    char bestpath[MAX_FILENAME];

    for (int i = 1; i <= 3; i++) {
        snprintf(store_path, MAX_PATH_LEN, "store%d.txt", i);

        file = fopen(store_path, "r");
        if (file == NULL) {
            perror("Failed to open store file");
            continue;
        }

        price = 0.0;
        score = 0.0;
        cost = 0.0;

        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "  Price:", 8) == 0) {
                sscanf(line, "  Price: %f", &price);
            } else if (strncmp(line, "  Score:", 8) == 0) {
                sscanf(line, "  Score: %f", &score);
            }
        }

        fclose(file);

        cost = price * score;
        if (cost > best_cost) {
            best_cost = cost;
            strncpy(best_store_path, store_path, MAX_PATH_LEN);
        }
    }

    if (best_cost > 0.0) {
        int order_chandommm = order_Chandom(args->user->username, 0);
        sprintf(bestpath, "%s_bestlist%d.txt", args->user->username, order_chandommm);
        file = fopen(best_store_path, "r");
        best_file = fopen(bestpath, "w");

        if (file == NULL || best_file == NULL) {
            perror("Failed to open file for best cost");
            if (file) fclose(file);
            if (best_file) fclose(best_file);
            return;
        }

        fprintf(best_file, "Best Cost: %.2f\n", best_cost);
        fprintf(best_file, "Details from %s:\n\n", best_store_path);

        while (fgets(line, sizeof(line), file)) {
            fprintf(best_file, "%s", line);
        }

        fclose(file);
        fclose(best_file);
    } else {
        printf("No valid store files found or costs calculated.\n");
    }
}

// Function to process bestlist file
double process_bestlist(int orderNumber, struct HandleArgs* args) {
    char bestlist_path[MAX_PATH_LEN];
    snprintf(bestlist_path, sizeof(bestlist_path), "%s_bestlist%d.txt", args->user->username, orderNumber);

    FILE *file = fopen(bestlist_path, "r");
    if (file == NULL) {
        perror("Failed to open bestlist file");
        return -1.0;
    }

    double best_cost = 0.0;
    char line[256];
    int found_best_cost = 0;
    struct Product *product = &args->productlist[MAX_PRODUCTS];
    int index = 0;

    // Read Best Cost and populate product list
    while (fgets(line, sizeof(line), file)) {
        if (!found_best_cost && strncmp(line, "Best Cost:", 10) == 0) {
            sscanf(line, "Best Cost: %lf", &best_cost);
            found_best_cost = 1;
        }

        if (strncmp(line, "Store", 5) == 0) {
            sscanf(line, "Store %d", &args->storeIndex);
        }

        if (strncmp(line, "  Name:", 7) == 0) {
            sscanf(line, "  Name: %s", product->name);

            // Read product details
            fgets(line, sizeof(line), file);
            sscanf(line, "  Price: %f", &product->price);

            fgets(line, sizeof(line), file);
            sscanf(line, "  Score: %f", &product->score);

            fgets(line, sizeof(line), file);
            sscanf(line, "  Entity: %d", &product->entity);

            fgets(line, sizeof(line), file);
            sscanf(line, "  Cost: %f", &product->cost);

            fgets(line, sizeof(line), file); // Date
            fgets(line, sizeof(line), file); // Time

            fgets(line, sizeof(line), file);
            sscanf(line, "  file_path: %s", product->file_path);

            memcpy(&args->productlist[index], product, sizeof(struct Product));
            index += 1;
        }
    }

    fclose(file);

    if (!found_best_cost) {
        printf("Failed to find Best Cost in %s\n", bestlist_path);
        return -1.0;
    }

    return best_cost;
}

// Main function to check, process, and update
void check_and_ask_to_buy(struct HandleArgs* args, struct User* user) {
    int order_chandommm = order_Chandom(user->username, 0);
    double final_cost = process_bestlist(order_chandommm, args);
    double effective_cost = final_cost;
    char* bestPath = malloc(MAX_PATH_LEN * sizeof(char));
    sprintf(bestPath, "%s_bestlist%d.txt", args->user->username, order_chandommm);

    // Check if off.txt exists and apply "off" percentage
    if (off_file_exists(user->username, order_chandommm)) {
        printf("Applying OFF discount of 10 percent\n");
        effective_cost = final_cost * 0.1;
    }

    if (effective_cost <= user->priceThreshold || user->priceThreshold == -1) {
        printf("Your price threshold allows you to buy this product. Final cost: %.2f\n", final_cost);

        char ans;
        printf("Do you want to buy this product? (Y or N): ");
        scanf(" %c", &ans);

        if (ans == 'Y' || ans == 'y') {
            // Call create_off to generate the off.txt file
            create_off(user->username, order_chandommm, args->storeIndex);

            pthread_t update_thread;
            pthread_create(&update_thread, NULL, (void* (*)(void*))update_score_in_file, bestPath);
            // sleep thread
            pthread_join(update_thread, NULL);

            printf("Purchase successful and all product files updated.\n");
        } else if (ans == 'N' || ans == 'n') {
            printf("You chose not to buy this product.\n");
        }
    } else {
        printf("You cannot buy this product. Cost exceeds your price threshold.\n");
    }
    free(args);
}

void create_off(const char* username, int order_chandommm, int storeBought) {
    char offPath[MAX_PATH_LEN];
    snprintf(offPath, MAX_PATH_LEN, "%s_off%d.txt", username, order_chandommm);

    FILE *off_file = fopen(offPath, "w");
    if (off_file != NULL) {
        fprintf(off_file, "*** User OFF Tag *** \n");
        fprintf(off_file, "  Username: %s\n", username);
        fprintf(off_file, "  Order: %d\n", order_chandommm);
        fprintf(off_file, "  Store: %d\n", storeBought);
        fprintf(off_file, "------------------------------------\n");
        fclose(off_file);
    } else {
        perror("Failed to write to file");
    }
}

bool off_file_exists(const char* username, int order_chandommm) {
    char offPath[MAX_PATH_LEN];
    snprintf(offPath, MAX_PATH_LEN, "%s_off%d.txt", username, order_chandommm);

    struct stat buffer;
    return (stat(offPath, &buffer) == 0);
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

void update_score_in_file(char *input_file) {
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    FILE *file = fopen(input_file, "r");
    if (!file) {
        printf("error to opening file in update score in file\n");
        return;
    }
    char line[512];
    char name[50], file_path[200];
    float price, score;
    int entity;
    int product_count = 0;
    // خواندن فایل ورودی
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Name:")) {
            sscanf(line, " Name: %49[^\n]", name);
        } else if (strstr(line, "Price:")) {
            sscanf(line, " Price: %f", &price);
        } else if (strstr(line, "Score:")) {
            sscanf(line, " Score: %f", &score);
        } else if (strstr(line, "Entity:")) {
            sscanf(line, " Entity: %d", &entity);
        } else if (strstr(line, "file_path:")) {
            sscanf(line, " file_path: %199[^\n]", file_path);
            float user_score;
            while (1) {
                // گرفتن امتیاز از کاربر
                printf("How would you rate the product %s with the price %.2f (between 0 and 5)?", name, price);
                scanf("%f", &user_score);
                if (user_score < 0.0 || user_score > 5.0) {
                    printf("Invalid score. Must be between 0 and 5.\n");
                    continue;
                }else{
                    break;
                }
            }
            // محاسبه امتیاز جدید
            float new_score = (score + user_score) / 2;
            FILE *product_file = fopen(file_path, "r+");
            if (!product_file) {
                printf("خطا در باز کردن فایل: %s\n", file_path);
                continue;
            }
            time_t now = time(NULL);
            struct tm *local = localtime(&now);
            char current_time[50];
            strftime(current_time, sizeof(current_time), "Last Modified: %Y-%m-%d %H:%M:%S\n", local);
            // بروزرسانی محتوا
            char buffer[1024];
            char new_content[1024] = "";
            int found_score = 0, found_last_modified = 0;

            pthread_mutex_lock(&file_mutex);

            while (fgets(buffer, sizeof(buffer), product_file)) {
                if (strstr(buffer, "Score:") && !found_score) {
                    sprintf(buffer, "Score: %.2f\n", new_score);
                    found_score = 1;
                } else if (strstr(buffer, "Last Modified:") && !found_last_modified) {
                    sprintf(buffer, "%s", current_time);
                    found_last_modified = 1;
                }
                strcat(new_content, buffer);
            }
            rewind(product_file);
            fprintf(product_file, "%s", new_content);
            fflush(product_file);
            fclose(product_file);

            pthread_mutex_unlock(&file_mutex);
            printf("Score and modification time were successfully updated.\n");
            product_count++;
        }
    }
    fclose(file);
    if (product_count == 0) {
        printf("No products found in the input file.\n");
    }
}
