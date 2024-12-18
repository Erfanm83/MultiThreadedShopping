
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