# TicTakToe
A console-based Tic-Tac-Toe game for variable board sizes with bot AI, written in C. Includes two versions: the original (XO.c) and an optimized variant (XOopt.c) with algorithmic improvements for benchmarking bot performance.
## Files
 
```
XO.c      — full game with menu: human vs human, human vs bot, bot vs bot
XOopt.c     — optimized bot-vs-bot benchmark version
```
 
---
 
## Features
 
- Board sizes from 3×3 up to 40×40 (XO.c) / unlimited (XOopt.c, heap-allocated)
- Win condition: 3 in a row on 3×3, 4 on 4×4, 5 on larger boards
- Two bot difficulty levels:
  - **Easy** — random moves near existing pieces
  - **Hard** — threat detection (blocks opponent's 4-in-a-row, builds own lines)
- Scrolling viewport for large boards — always shows the active play area
- Last move highlighted in color (X = red, O = blue)
- Game history saved to `game_results.txt`
- Cross-platform: Windows (WinAPI colors) and Linux/macOS (ANSI escape codes)
## Game Modes (XO.c)
 
| Option | Mode |
|---|---|
| 1 | Human vs Human |
| 2 | Human vs Bot |
| 3 | Bot vs Bot |
| 4 | Show game history |
 
## Optimizations in XOopt.c
 
`XOopt.c` is a stripped-down bot-vs-bot version used to measure AI performance. It introduces two algorithmic optimizations over the original:
 
1. **Active zone pruning** — the bot only considers cells within radius 2 of existing pieces, drastically reducing the search space on large boards.
2. **Incremental win check** — instead of scanning the entire board, win detection only checks lines passing through the last move.
3. **Memory optimization** — board uses heap-allocated `char**` instead of a fixed `char[40][40]` array; row initialization uses `memset` instead of a loop; win length is cached at init time.
After each game, `XOopt.c` prints total game time and average time per bot move.
 
## Build
 
**Linux / macOS:**
```bash
# Original
gcc -Wall -Wextra -o XO XO.c
 
# Optimized benchmark
gcc -Wall -Wextra -O2 -o XOopt XOopt.c
```
 
**Windows (MSVC):**
```bat
cl /W4 /O2 XO.c /Fe:XO.exe
cl /W4 /O2 XOopt.c /Fe:XOopt.exe
```
 
**Windows (MinGW):**
```bash
gcc -Wall -Wextra -O2 -o XO.exe XO.c
gcc -Wall -Wextra -O2 -o XOopt.exe XOopt.c
```
 
## Game History
 
Results are appended to `game_results.txt` in the working directory after each game. Each entry includes date/time, board size, game mode, bot difficulty, move counts, and the winner.
 
