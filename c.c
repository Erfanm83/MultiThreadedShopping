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
#include <semaphore.h>

#define MAX_PRODUCTS 100
#define MAX_PATH_LEN 512
#ifndef DT_REG
    #define DT_REG 8
#endif
#ifndef DT_DIR
    #define DT_DIR 3
#endif
#define NORMAL_COLOR  "\x1B[0m"
#define GREEN  "\x1B[32m"
#define BLUE  "\x1B[34m"
#define MAX_FILES 2000

// Global Variable
char input[100];
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; 

typedef struct FileSemaphore
{
    char file_path[MAX_PATH_LEN];
    sem_t semaphore;
} FileSemaphore;

FileSemaphore file_semaphores[MAX_FILES];
int semaphore_count = 0;
pthread_mutex_t semaphore_mutex = PTHREAD_MUTEX_INITIALIZER;


void initialize_file_semaphore(const char* file_path) {
    pthread_mutex_lock(&semaphore_mutex);
    strncpy(file_semaphores[semaphore_count].file_path, file_path, MAX_PATH_LEN);
    sem_init(&file_semaphores[semaphore_count].semaphore, 0, 1);
    semaphore_count++;
    pthread_mutex_unlock(&semaphore_mutex);
}

sem_t* get_file_semaphore(const char* file_path) {
    for (int i = 0; i < semaphore_count; i++) {
        if (strcmp(file_semaphores[i].file_path, file_path) == 0) {
            return &file_semaphores[i].semaphore;
        }
    }
    return NULL;
}

void initialize_semaphores_for_directory(const char* directory_path) {
    DIR* dir = opendir(directory_path);
    if (dir == NULL) {
        perror("Error opening directory in semaphore");
        return;
    }

    struct dirent* entry;
    char path[MAX_PATH_LEN];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, MAX_PATH_LEN, "%s/%s", directory_path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            initialize_semaphores_for_directory(path);
        } else if (entry->d_type == DT_REG && strstr(entry->d_name, ".txt")) {
            initialize_file_semaphore(path);
        }
    }

    closedir(dir);
}



struct Order {
    char name[50];
    int quantity;
};

struct User {
    char username[50];
    struct Order orderlist[MAX_PRODUCTS];
    float priceThreshold;
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
    float cost;  // cost is initialized to -1 later in the function, not in the struct definition
    struct Date date;
    struct Time time;
};

struct HandleArgs {
    struct User* user;
    char* file_path;
    struct Product product;
};

// Prototype
char** get_slices_input(char *inputstr, int *num_tokens , char *splitter);
void get_user_details(struct User* user);
void handle_all_stores(struct HandleArgs* args);
void* handle_store(struct HandleArgs* args);
void* handle_file(void* args);

// Main function
int main() {
    const char* root_directory = "./Dataset";
    initialize_semaphores_for_directory(root_directory);

    struct User user = {0};  // Declare a global 'user' variable
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
    // } while(1);


    for (int i = 0; i < semaphore_count; i++) {
        sem_destroy(&file_semaphores[i].semaphore);
    }
    return 0;
}

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
    struct HandleArgs* args = (struct HandleArgs*)args_void;
    struct User* user = args->user;
    char* file_name = args->file_path;

    sem_t* semaphore = get_file_semaphore(file_name);

    if (semaphore == NULL) {
        fprintf(stderr, "No semaphore found for file: %s\n", file_name);
        pthread_exit(NULL);
    }

    sem_wait(semaphore);

    printf("Thread %lu is in semaphore file: %s\n", pthread_self(), file_name);

    struct Product* product = &args->product;

    pid_t tid = syscall(SYS_gettid);
    pid_t category_pid = getpid();

    char formatted_file_id[13];
    snprintf(formatted_file_id, sizeof(formatted_file_id), "%06dID", tid);

    printf("PID %jd created thread for %s TID: %jd\n",
            (intmax_t)category_pid, 
            formatted_file_id, 
            (intmax_t)syscall(SYS_gettid));

    char* namestr = NULL;
    int entity = 0;
    bool isNameFound = false;
    bool isEntityValid = false;
    bool isProductFound = false;
    char line[256];
    // Lock the file handling section to ensure exclusive access
    pthread_mutex_lock(&file_mutex);
    int orderIndex;
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_name);
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
        FILE *file = fopen(file_name, "r");
        if (file == NULL) {
            printf("Error opening file: %s\n", file_name);
            pthread_mutex_unlock(&file_mutex);
            pthread_exit(NULL);
        } else {
            if (isNameFound && isEntityValid && !isProductFound) {
                while (fgets(line2, sizeof(line2), file)) {
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
        fclose(file);

        sem_post(semaphore);

        // If a valid product was found, print its details
        if (isProductFound) {
            product->cost = product->score*product->price;
            printf("Founded Product :\n");
            printf("Name : %s\n", product->name);
            printf("Price : %.2f\n", product->price);
            printf("Score : %.2f\n", product->score);
            printf("Entity : %d\n", product->entity);
            printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
            printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
            printf("Cost : %.2f\n", product->cost);
            printf("-----------------------------------------\n");
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
    pid_t category_pid = getpid();
    d = opendir(args->file_path);
    if (d == NULL) {
        perror("Failed to reopen category directory");
        return NULL;
    }
    // Create a thread for each file in the category
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_DIR) {
            // Allocate memory for args for each thread to ensure thread-safety
            struct HandleArgs* thread_args = malloc(sizeof(struct HandleArgs));
            if (thread_args == NULL) {
                perror("Failed to allocate memory for thread_args");
                closedir(d);
                return NULL;
            }

            // Copy data to thread_args to avoid overwriting the shared args
            thread_args->user = args->user;
            thread_args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
            snprintf(thread_args->file_path, MAX_PATH_LEN, "%s/%s", file_path, dir->d_name);

            pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, thread_args);
            pthread_join(threads[thread_index], NULL);

            struct Product* product = &thread_args->product;
            if (product->cost != 0) {
                // Print the product info (for debugging)
                printf("Name : %s\n", product->name);
                printf("Price : %.2lf\n", product->price);
                printf("Score : %lf\n", product->score);
                printf("Entity : %d\n", product->entity);
                printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
                printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
                printf("Cost : %.2lf\n", product->cost);
                printf("-----------------------------------------\n");
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

    for (int i = 0; i < 8; i++) {
        args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
        snprintf(args->file_path, MAX_PATH_LEN, "%s/%s", store_path, category_paths[i]);
        // Create a process for each store
        category_pid = fork();
        switch (category_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                printf("PID %jd created child for %s PID: %jd\n",(intmax_t)store_pid, category_paths[i], (intmax_t)getpid());
                handle_category(args);
                exit(EXIT_SUCCESS);
            default:
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

void handle_all_stores(struct HandleArgs* args) {
    char * parent_path = (char*)args->file_path;
    pid_t store_pid;
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
               perror("signal");
               exit(EXIT_FAILURE);
           }
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
        args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
        snprintf(args->file_path, MAX_PATH_LEN, "%s/Store%d", parent_path, i + 1);

        // Create a process for each store
        pid_t parent_pid = getppid();
        store_pid = fork();
        switch (store_pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                handle_store(args);
                exit(EXIT_SUCCESS);
            default:
                printf("PID %jd created child for Store%d PID:%jd\n", (intmax_t)parent_pid, i + 1, (intmax_t)store_pid);
                // exit(EXIT_SUCCESS);
                break;
           }
    }

    // Wait for all store processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
}














