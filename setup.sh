#!/bin/bash

# Remove the specified files
rm -f store1.txt
rm -f store2.txt
rm -f store3.txt

rm -v *.txt

cd ./Dataset
rm -v *.log
cd ..

# Compile the C program
# gcc -o main main.c

# Run the compiled program
# ./main
