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

// Global Variable
char input[100];

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
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", file_name);
        pthread_exit(NULL);
    } else {
        memset(product, 0, sizeof(struct Product));  // Clear product struct
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;  // Remove newline character
            // Debug: Print the line being processed
            // printf("Processing line: %s\n", line);
            // Process "Name:" line
            if (strncmp(line, "Name:", 5) == 0) {
                namestr = line + 6;  // Skip the "Name: " part
                for (int i = 0; i < MAX_PRODUCTS; i++) {
                    if (strcmp(namestr, user->orderlist[i].name) == 0) {
                        // We found the product name in the user's order list
                        strcpy(product->name, namestr);
                        isNameFound = true;
                        break;
                    }
                }
            }
            // Process "Entity:" line
            else if (strncmp(line, "Entity:", 7) == 0) {
                entity = atoi(line + 8);  // Extract the entity value
                for (int i = 0; i < MAX_PRODUCTS; i++) {
                    if (isNameFound && user->orderlist[i].quantity <= entity) {
                        product->entity = entity;
                        isEntityValid = true;
                        break;
                    }
                }
            }
        }
        fclose(file);
        char line2[256];
        FILE *file = fopen(file_name, "r");
        if (file == NULL) {
            printf("Error opening file: %s\n", file_name);
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

        // Debug: Print the user's orderlist to check if it's populated
        // printf("User's Order List:\n");
        // for (int i = 0; i < MAX_PRODUCTS; i++) {
        //     if (user->orderlist[i].quantity > 0) {
        //         printf("Item: %s, Quantity: %d\n", user->orderlist[i].name, user->orderlist[i].quantity);
        //     }
        // }
    }

    return NULL;
}


// void* handle_file(void* args_void) {
//     struct HandleArgs* args = (struct HandleArgs*)args_void;
//     struct User* user = args->user;
//     char* file_name = args->file_path;
//     struct Product* product = &args->product;

//     pid_t tid = syscall(SYS_gettid);
//     pid_t category_pid = getpid();

//     char formatted_file_id[13];
//     snprintf(formatted_file_id, sizeof(formatted_file_id), "%06dID", tid);

//     printf("PID %jd created thread for %s TID: %jd\n",
//             (intmax_t)category_pid, 
//             formatted_file_id, 
//             (intmax_t)syscall(SYS_gettid));

//     char* namestr, entity, orderIndex;
//     bool isName = false;
//     bool isEntity = false;
//     char line[256];
//     FILE *file = fopen(file_name, "r");
//     if (file == NULL) {
//         printf("Error opening file: %s\n", file_name);
//         pthread_exit(NULL);
//     } else {
//         memset(product, 0, sizeof(struct Product));  // Clear product struct
        
//         while (fgets(line, sizeof(line), file)) {
//             line[strcspn(line, "\n")] = 0;
//             if (strncmp(line, "Name:", 5) == 0) {
//                 strcpy(namestr, line + 6);
//                 for (int i = 0; i < MAX_PRODUCTS; i++) {
//                     if (strcmp(namestr, user->orderlist[i].name) == 0) {
//                         strcpy(product->name, line + 6);
//                         orderIndex = i;
//                         isName = true;
//                     }
//                 }
//             } else if (strcmp(line, "Entity:", 7) == 0) {
//                 strcpy(entity, line + 8);
//                 if ((int)entity >= user->orderlist[orderIndex].quantity) {
//                     product->entity = atoi(line + 8);
//                     isEntity = true;
//                 }
//             }
//         }

//         if (isName == true && isEntity == true) {
//             while (fgets(line, sizeof(line), file)) {
//                 line[strcspn(line, "\n")] = 0;
//                 if (strncmp(line, "Price:", 6) == 0) {
//                     product->price = atof(line + 7);
//                 } else if (strncmp(line, "Score:", 6) == 0) {
//                     product->score = atof(line + 7);
//                 } else if (strncmp(line, "Last Modified:", 14) == 0) {
//                     char *date_time_str = line + 15;
//                     char *date_str = strtok(date_time_str, " ");
//                     char *time_str = strtok(NULL, " ");

//                     if (date_str && time_str) {
//                         int date_tokens_count = 0;
//                         char **date_tokens = get_slices_input(date_str, &date_tokens_count, "-");
//                         if (date_tokens_count == 3) {
//                             product->date.year = atoi(date_tokens[0]);
//                             product->date.month = atoi(date_tokens[1]);
//                             product->date.day = atoi(date_tokens[2]);
//                         }
//                         free(date_tokens);

//                         int time_tokens_count = 0;
//                         char **time_tokens = get_slices_input(time_str, &time_tokens_count, ":");
//                         if (time_tokens_count == 3) {
//                             product->time.hour = atoi(time_tokens[0]);
//                             product->time.minutes = atoi(time_tokens[1]);
//                             product->time.seconds = atoi(time_tokens[2]);
//                         }
//                         free(time_tokens);
//                     }
//                 }
//             }
//         }

//         fclose(file);
        
//         printf("The Product Found:\n");
//         product->cost = product->price * product->score;

//         printf("Name : %s\n", product->name);
//         printf("Price : %.2lf\n", product->price);
//         printf("Score : %lf\n", product->score);
//         printf("Entity : %d\n", product->entity);
//         printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
//         printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
//         printf("Cost : %.2lf\n", product->cost);
//         printf("-----------------------------------------\n");

//         printf("User's Order List:\n");
//         for (int j = 0; j < MAX_PRODUCTS; j++) {
//             if (user->orderlist[j].quantity > 0) {
//                 printf("Item: %s, Quantity: %d\n", user->orderlist[j].name, user->orderlist[j].quantity);
//             }
//         }
//     }
//     return NULL;
// }

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
    // Create a thread for each file in the category
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_DIR) {
            args->file_path = malloc(MAX_PATH_LEN * sizeof(char));
            snprintf(args->file_path, MAX_PATH_LEN, "%s/%s", file_path, dir->d_name);
            // printf("args->file_path : %s\n" , args->file_path);
            // Create a new thread to process the file
            int x = pthread_create(&threads[thread_index], NULL, (void* (*)(void*))handle_file, args);
            if (x != 0) {
                perror("Error creating thread");
                free(args);
                free(file_path);
                closedir(d);
                return NULL;
            }

            thread_index++;
        }
    }
    closedir(d);

    // Wait for all threads to finish
    for (int i = 0; i < file_count; i++) {
        pthread_join(threads[i], NULL);
    }
    // struct Product* product = &args->product;
    // printf("GORGALI  2\n");
    // product->cost = product->price * product->score;

    // // Print the product info (for debugging)
    // printf("Name : %s\n", product->name);
    // printf("Price : %.2lf\n", product->price);
    // printf("Score : %lf\n", product->score);
    // printf("Entity : %d\n", product->entity);
    // printf("Date : %d-%d-%d\n", product->date.year, product->date.month, product->date.day);
    // printf("Time : %d:%d:%d\n", product->time.hour, product->time.minutes, product->time.seconds);
    // printf("Cost : %.2lf\n", product->cost);
    // printf("-----------------------------------------\n");
    // // Debug: Print the user's orderlist to check if it's populated
    // printf("User's Order List:\n");
    // for (int i = 0; i < MAX_PRODUCTS; i++) {
    //     if (args->user->orderlist[i].quantity > 0) {
    //         printf("Item: %s, Quantity: %d\n", args->user->orderlist[i].name, args->user->orderlist[i].quantity);
    //     }
    // }

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
    for (int i = 0; i < 1; i++) {
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

