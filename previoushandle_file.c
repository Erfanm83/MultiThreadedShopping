void* handle_file(void* args_void) {
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct HandleArgs* args = (struct HandleArgs*)args_void;
    struct Product* product = &args->product;
    struct User* user = args->user;
    char* file_path = args->file_path;

    pid_t tid = syscall(SYS_gettid);
    pid_t category_pid = getpid();

    // Create a structure to store thread details
    // struct ThreadDetails* thread_details = malloc(sizeof(struct ThreadDetails));
    // thread_details->tid = tid;  // Store the thread ID
    // thread_details->product = *product;    // Store product details
    // thread_details->new_score = -1;        // Initialize new_score with invalid value
    // thread_details->updated = 0;           // No update yet
    // pthread_cond_init(&thread_details->cond_var, NULL);
    // pthread_mutex_init(&thread_details->mutex, NULL);

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

    return NULL;
}