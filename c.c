#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PRODUCTS 100

// Function to split input by a custom delimiter and return a limited number of slices
char** get_slices_input(char *inputstr, int num_slices, char *splitter) {
    // Duplicate the input string to prevent modifying the original string
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

    int token_count = 0;  // Initialize token count
    char *token = strtok(str, splitter);

    // Loop through all tokens and store them in the array
    while (token != NULL && (num_slices == 0 || token_count < num_slices)) {
        tokens[token_count] = strdup(token);
        if (tokens[token_count] == NULL) {
            perror("Failed to duplicate token");
            free(str);
            return NULL;
        }
        token_count++;
        token = strtok(NULL, splitter);
    }

    // If num_slices is 0, split till the end
    if (num_slices == 0) {
        num_slices = token_count;
    }

    // Clean up and return the result
    free(str);  // Free the duplicated input string

    return tokens;
}

int main() {
    char input[] = "./Dataset/Store1/Home/45.txt/2.txt/...";
    int num_slices = 4;
    char splitter[] = "/";

    char **result = get_slices_input(input, num_slices, splitter);

    if (result != NULL) {
        for (int i = 0; i < num_slices; i++) {
            printf("Slice %d: %s\n", i, result[i]);
            free(result[i]);  // Free each slice
        }
        free(result);  // Free the tokens array
    }

    return 0;
}
