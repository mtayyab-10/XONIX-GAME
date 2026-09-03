# XONIX Game

A modern remake of the *classic Xonix arcade game, developed using **C++* and *SFML (Simple and Fast Multimedia Library)*.  
This project supports *single-player* and *two-player* modes, dynamic difficulty levels, real-time scoring, and more.

⚙ How to Build and Run on Linux

Follow the steps below to build and run the game on a Linux system:
(You Can also Run This Game In Windows On Visual Studio)
### Step 1: Update your package list
sudo apt update

### Step 2: Install required dependencies
sudo apt install cmake
 
sudo apt install build-essential

sudo apt install libsfml-dev

sudo apt install make   (if not already installed)

### Step 3: Clone the repository
git clone https://github.com/mtayyab-10/XONIX-Game.git
cd XONIX-Game

### Step 4: Create a build directory and compile the game
mkdir build

cd build

cmake ..

make

After successful compilation, an executable file named xonix will appear in the build folder.

To run the game:
./xonix

##### After the first build, you can simply run make again to recompile any changes.

## 🎮 Game Features
### 1. Basic Features (10 Marks)

Single       and Two Player Modes
 
Start Menu -  Start Game -  Select Level -  Scoreboard

End Menu (Properly end the game) -   Show final score (highlight if it’s a new high score) - Options: Restart, Main Menu, Exit Game

### 2. Difficulty & Enemy Count (20 Marks)

Easy – 2 enemies

Medium – 4 enemies

Hard – 6 enemies

Continuous Mode – Starts with 2 enemies, adds 2 more every 20 seconds indefinitely.

### 3. Movement Counter (5 Marks)

Displays the total number of moves made by the player.
(A move counts each time the player starts building tiles.)

### 4. Enemy Speed & Movement (20 Marks)

Tracks elapsed game time.

Every 20 seconds, enemy movement speed increases.

After 30 seconds, half the enemies switch to defined geometric movement patterns:

Zig-zag - Circular

### 5. Scoring & Reward System (10 Marks)

Capturing a tile = +1 point

Capturing >10 tiles in a single move = ×2 points

After 3 bonuses, threshold reduces to 5 tiles for ×2 points

#### Additional feature :
Upon reaching 50 points, player earns a power-up that freezes enemies for 3 seconds

Unused power-ups stack in the inventory.

### 6. Scoreboard (10 Marks)

File-based scoreboard.txt maintains top 5 high scores (sorted descending).

Stores score and time taken.

On game over, if the player’s score qualifies, the file updates automatically.

### 7. Two Player Mode (25 Marks)

Player 1 → Arrow Keys

Player 2 → W, A, S, D

Shared timer, but individual scores and power-ups.

### 🧠 Technologies Used

Language: C++

Library: SFML (Simple and Fast Multimedia Library)

Build System: CMake
