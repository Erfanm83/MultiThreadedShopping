#!/bin/bash

# Create main project directory
mkdir ShoppingSystemProject
cd ShoppingSystemProject

# Create src, include, and logs directories
mkdir src include logs

# Create source files in the src directory
touch src/main.c
touch src/process_store.c
touch src/product_threads.c
touch src/synchronization.c
touch src/ipc.c
touch src/utils.c

# Create header files in the include directory
touch include/shopping.h
touch include/store.h
touch include/product_thread.h
touch include/synchronization.h
touch include/ipc.h

# Create log files in the logs directory
touch logs/user_order.log
touch logs/store_operations.log
touch logs/product_threads.log
touch logs/synchronization.log

# Create Makefile, README.md, .gitignore
touch Makefile
touch README.md
touch .gitignore

# Done
echo "Project structure created successfully!"

