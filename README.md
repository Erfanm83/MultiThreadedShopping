# MultiThreadedShopping
My OS Course Midterm Project

## Overview

The **Order Management System** is a robust C-based application designed to handle product management, user orders, and multi-threaded processing. It efficiently manages multiple users and stores, allowing for seamless order processing, product tracking, and user interactions. The system leverages multi-threading and multiprocessing to ensure optimal performance and scalability.

## Features

- **User Management**: Handle multiple users with unique usernames and order histories.
- **Product Management**: Manage a wide range of products with detailed attributes such as price, score, entity, and cost.
- **Multi-threading**: Utilize pthreads for concurrent processing of files and orders.
- **Process Handling**: Fork child processes to manage different stores and categories efficiently.
- **Logging**: Comprehensive logging of user actions and system events for auditing and debugging.
- **Discount System**: Apply discounts based on user eligibility and purchase history.
- **Best Cost Calculation**: Determine the best cost from available stores and prompt user for purchase decisions.
- **Thread Control**: Manage thread synchronization using mutexes and condition variables to ensure thread safety.

## Prerequisites

- **Operating System**: Unix/Linux
- **Compiler**: GCC
- **Libraries**:
  - pthread
  - Standard C libraries

## Installation

1. **Clone the Repository**

   ```bash
   git clone https://github.com/Erfanm83/MultiThreadedShopping.git
   cd MultiThreadedShopping
   ```

2. **Setup Environment**

- Ensure that you have the necessary permissions to execute scripts and compile the program.

    ```bash
    chmod +x setup.sh
    ```

3. **Run Setup Script**

- The `setup.sh` script cleans up any existing log and store files to ensure a fresh start.

    ```bash
    ./setup.sh
    ```

## Compilation

1. **Using Makefile**
   ```bash
   make
   ```
   This command will compile the `main.c` file and generate an executable named `main`.

2. **Manual Compilation**

- Alternatively, you can compile the program manually using `gcc`:

    ```bash
    gcc -o main main.c -g -pthread
    ```

## Usage

1. Run the Program

After compilation, execute the program using:

```bash
./main
```

2. Program Workflow

- User Input: Enter your username and order details.
- Order Processing: The system will process your orders across different stores.
- Best Cost Calculation: It will determine the best cost from available stores.
- Purchase Decision: You will be prompted to confirm your purchase based on the cost and any applicable discounts.
- Logging: All actions are logged for future reference.


## Makefile Commands

- Build the Project

```bash
make
```

- Clean Build Files

```bash
make clean 
```
This command removes all object files and the executable.

## Setup Script (`setup.sh`)
- The setup.sh script performs the following actions:

- Removes existing store files (`store1.txt`, `store2.txt`, `store3.txt`).
- Deletes all `.txt` files in the current directory.
- Navigates to the `./Dataset` directory and removes all `.log` files.
- Compiles the C program (optional steps are commented out).
- Runs the compiled program (optional steps are commented out).

### Usage
```bash
./setup.sh
```

## Project Structure
```bash
MultiThreadedShopping/
├── Dataset/
│   ├── *.log
├── store1.txt
├── store2.txt
├── store3.txt
├── main.c
├── Makefile
├── setup.sh
└── README.md
```

## Contributing
Contributions are welcome! Please follow these steps:
1. Fork the Repository

2. Create a Feature Branch
```bash
git checkout -b feature/YourFeature
```

3.Commit Your Changes
```bash
git commit -m "Add your message here"
```

4.Push to the Branch
```bash
git push origin feature/YourFeature
```

5.Open a Pull Request

## License
This project is licensed under the MIT License.

## Developed by:

- Erfan Mahmoudi, student number: 4011262474
- Seyed Alireza Hashemi, student number: 4011262258
