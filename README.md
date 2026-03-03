# Terminal Frog Game

A command-line implementation of a classic Frogger-style game written entirely in C. Developed as part of a Basics of Programming course, this project demonstrates foundational programming concepts such as real-time game loops, terminal I/O, and file handling.

## Tech Stack

* **Language:** C
* **Environment:** Terminal / Command Line Interface (CLI)
* **Build Tool:** Makefile

## Core Features

* **Terminal Gameplay:** Fully playable game rendered directly within the console environment using standard C libraries.
* **File-based Configuration:** Custom game dimensions, mechanics, and ASCII art models are loaded dynamically from external text files (`game_settings.txt`, `models.txt`, `art_file.txt`).
* **State Management:** Handles game state updates, collision detection, and rendering sequences efficiently.

## The "Development Archive" (My Learning Process)

If you browse the repository, you will notice a `development_archive` folder containing sequentially named `.c` files. This represents my manual approach to "version control" before I fully integrated Git into my workflow! I decided to keep it in the repository as a transparent look at my iterative development process and how my coding practices have evolved since the beginning of my studies.

## How to Build and Run

### 1. Compile the Project
Use the provided `Makefile` to compile the source code:
```bash
make
```

### 2. Run the Game
```bash
./main
```
*(Note: Ensure your terminal window is appropriately sized to display the custom ASCII game board correctly).*