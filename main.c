#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_PRODUCTS 100

// Global Variable
char input[100];
struct OrderList {
    char name[50];
    int quantity;
};

// Function to split input by space and return an array of strings (tokens)
char** get_slices_input(char *inputstr, int *num_tokens) {
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
    char *token = strtok(str, " ");  // Split by space

    // Loop through all tokens and store them in the array
    while (token != NULL) {
        tokens[*num_tokens] = strdup(token);
        if (tokens[*num_tokens] == NULL) {
            perror("Failed to duplicate token");
            free(str);
            return NULL;
        }
        (*num_tokens)++;
        token = strtok(NULL, " ");
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
            printf("Price threshold:");

            fgets(input, sizeof(input), stdin);
            if (strcmp(input, "\n") == 0) {
                break;
            }
            *priceThreshold = strtof(input, NULL);
            break;
        }

        // Get the tokens from the input
        char **tokens = get_slices_input(input, &num_tokens);

        char *combinedstr = strcpy(item, tokens[0]);
        for (int i = 1; i < num_tokens - 1; i++) {
            strcat(combinedstr, " "); 
            strcat(combinedstr, tokens[i]); 
        }

        number = atoi(tokens[num_tokens - 1]);
        printf("Item: %s, Quantity: %d\n", combinedstr, number);

        // Use count to track the index for orderlist
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

// Main function
int main() {
    // Declare variables
    int usernumber = 0;
    char Username[50];
    float priceThreshold = -1;
    struct OrderList orderlist[100];

    // Call function to get user input
    get_user_input(Username, &usernumber, &priceThreshold, orderlist);

    // After gathering input, you can continue further tasks
    printf("User Input Completed!\n");
    printf("Number of Order: %d\n", usernumber);
    printf("Price threshold: %lf\n", priceThreshold);

    for (int i = 0; i < usernumber; i++) {
        printf("Name: %s, Quantity: %d\n", orderlist[i].name, orderlist[i].quantity);
    }

    // Create Thread (if needed)

    return 0;
}
