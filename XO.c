#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#ifdef _WIN32
void setColor(int color) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, color);
}
void resetColor() {
    setColor(7);
}
#else
void setColor(int color) {
    if (color == 12) printf("\033[31m");
    else if (color == 9) printf("\033[34m");
    else printf("\033[0m");
}
void resetColor() {
    printf("\033[0m");
}
#endif

struct Move {
    int abs_row;
    int abs_col;
    char symbol;
};

struct Pole {
    char cells[40][40];
    int size;
    int display_size;
    int offset_row;
    int offset_col;
    int can_expand;
    struct Move last_move;
    struct Move last_human_move;
    struct Move last_bot_move;
    int moves_count;
    int isSmallBoard;
};

struct Igrok {
    char symbol;
    int isBot;
    int difficulty;
    char name[20];
    int move_count;
};

struct GameSettings {
    int sizeChoice;
    int gameMode;
    int botDifficulty1;
    int botDifficulty2;
    char igrokSymbol;
};

int checkThreat(struct Pole* pole, char symbol, int threatLength);
void initEmptyPole(struct Pole* pole, int size);
void setViewToFitAllMoves(struct Pole* pole);
void makeBotMoveEasy(struct Pole* pole, char botSymbol, int* out_row, int* out_col);
void makeBotMoveHard(struct Pole* pole, char botSymbol, int* out_row, int* out_col);
void printPole(struct Pole* pole);
int checkWin(struct Pole* pole, char symbol);
int isBoardFull(struct Pole* pole);
void continueGame(struct Pole* pole, struct Igrok p1, struct Igrok p2, char nextSymbol, int isBotVsBot, char* winnerNameBuffer);
void getGameSettings(struct GameSettings* settings, int mode);
void logGameResult(int gameMode, const char* winnerName, int isDraw, int botDiff1, int botDiff2, int sizeChoice, int p1Moves, int p2Moves, const char* p1Name, const char* p2Name);
int showMainMenu();
void showGameHistory();

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void initEmptyPole(struct Pole* pole, int size) {
    pole->size = size;
    pole->isSmallBoard = (size == 3);
    pole->can_expand = !pole->isSmallBoard;
    pole->moves_count = 0;
    pole->offset_row = 0;
    pole->offset_col = 0;
    pole->display_size = pole->isSmallBoard ? size : 5;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            pole->cells[i][j] = ' ';
        }
    }

    pole->last_move.abs_row = -1;
    pole->last_move.abs_col = -1;
    pole->last_move.symbol = ' ';
    pole->last_human_move.abs_row = -1;
    pole->last_human_move.abs_col = -1;
    pole->last_human_move.symbol = ' ';
    pole->last_bot_move.abs_row = -1;
    pole->last_bot_move.abs_col = -1;
    pole->last_bot_move.symbol = ' ';
}

void setViewToFitAllMoves(struct Pole* pole) {
    if (pole->isSmallBoard) {
        pole->display_size = pole->size;
        pole->offset_row = 0;
        pole->offset_col = 0;
        return;
    }

    if (pole->moves_count == 0) {
        pole->offset_row = 0;
        pole->offset_col = 0;
        pole->display_size = 5;
        return;
    }

    int min_row = pole->size, max_row = -1;
    int min_col = pole->size, max_col = -1;
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] != ' ') {
                if (i < min_row) min_row = i;
                if (i > max_row) max_row = i;
                if (j < min_col) min_col = j;
                if (j > max_col) max_col = j;
            }
        }
    }

    if (pole->last_move.abs_row != -1 && pole->last_move.abs_col != -1) {
        if (pole->last_move.abs_row < min_row) min_row = pole->last_move.abs_row;
        if (pole->last_move.abs_row > max_row) max_row = pole->last_move.abs_row;
        if (pole->last_move.abs_col < min_col) min_col = pole->last_move.abs_col;
        if (pole->last_move.abs_col > max_col) max_col = pole->last_move.abs_col;
    }

    int margin = 2;
    min_row = (min_row - margin < 0) ? 0 : min_row - margin;
    max_row = (max_row + margin >= pole->size) ? pole->size - 1 : max_row + margin;
    min_col = (min_col - margin < 0) ? 0 : min_col - margin;
    max_col = (max_col + margin >= pole->size) ? pole->size - 1 : max_col + margin;

    pole->offset_row = min_row;
    pole->offset_col = min_col;
    int row_span = max_row - min_row + 1;
    int col_span = max_col - min_col + 1;
    pole->display_size = (row_span > col_span) ? row_span : col_span;
    if (pole->display_size > pole->size) pole->display_size = pole->size;

    if (pole->offset_row + pole->display_size > pole->size) {
        pole->offset_row = pole->size - pole->display_size;
    }
    if (pole->offset_col + pole->display_size > pole->size) {
        pole->offset_col = pole->size - pole->display_size;
    }
}

void printPole(struct Pole* pole) {
    int size = pole->isSmallBoard ? pole->size : pole->display_size;
    printf("\n    ");
    for (int j = 0; j < size; j++) {
        int global_col = pole->offset_col + j + 1;
        printf("%3d ", global_col);
    }
    printf("\n    ");
    for (int j = 0; j < size; j++) {
        printf("----");
    }
    printf("-\n");

    for (int i = 0; i < size; i++) {
        int global_row = pole->offset_row + i + 1;
        printf("%3d|", global_row);
        for (int j = 0; j < size; j++) {
            int abs_i = pole->offset_row + i;
            int abs_j = pole->offset_col + j;
            char cell = ' ';
            if (abs_i >= 0 && abs_i < pole->size && abs_j >= 0 && abs_j < pole->size) {
                cell = pole->cells[abs_i][abs_j];
            }
            int is_last = (pole->last_move.abs_row == abs_i && pole->last_move.abs_col == abs_j);
            if (is_last) {
                setColor(cell == 'X' ? 12 : 9);
            }
            printf(" %c ", cell);
            if (is_last) {
                resetColor();
            }
            if (j < size - 1) printf("|");
        }
        printf("|\n");
        if (i < size - 1) {
            printf("    ");
            for (int j = 0; j < size; j++) {
                printf("----");
            }
            printf("-\n");
        }
    }
    printf("\n");
}

void printLastMoves(struct Pole* pole, struct Igrok p1, struct Igrok p2) {
    if (pole->isSmallBoard) return;
    printf("\n");
    struct Igrok humanPlayer = p1.isBot ? p2 : p1;
    struct Igrok botPlayer = p1.isBot ? p1 : p2;
    if (pole->last_human_move.abs_row != -1) {
        int row = pole->last_human_move.abs_row + 1;
        int col = pole->last_human_move.abs_col + 1;
        char symbol = pole->last_human_move.symbol;
        const char* playerName = (symbol == p1.symbol) ? p1.name : p2.name;
        printf("Последний ход человека: %s сходил(а) на (%d, %d)\n", playerName, row, col);
    }
    if (pole->last_bot_move.abs_row != -1) {
        int row = pole->last_bot_move.abs_row + 1;
        int col = pole->last_bot_move.abs_col + 1;
        char symbol = pole->last_bot_move.symbol;
        const char* playerName = (symbol == p1.symbol) ? p1.name : p2.name;
        printf("Последний ход бота: %s сходил(а) на (%d, %d)\n", playerName, row, col);
    }
    printf("\n");
}

int checkWin(struct Pole* pole, char symbol) {
    int winLength = (pole->size == 3) ? 3 : (pole->size == 4) ? 4 : 5;
    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == symbol) {
                for (int dir = 0; dir < 4; dir++) {
                    int count = 1;
                    int x = j + dx[dir];
                    int y = i + dy[dir];
                    while (x >= 0 && x < pole->size && y >= 0 && y < pole->size && pole->cells[y][x] == symbol) {
                        count++;
                        x += dx[dir];
                        y += dy[dir];
                    }
                    if (count >= winLength) return 1;
                }
            }
        }
    }
    return 0;
}

int isBoardFull(struct Pole* pole) {
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') return 0;
        }
    }
    return 1;
}

int countNeighbors(struct Pole* pole, int row, int col, char botSymbol, char playerSymbol) {
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            int r = row + i;
            int c = col + j;
            if (r >= 0 && r < pole->size && c >= 0 && c < pole->size) {
                if (pole->cells[r][c] == botSymbol) count += 2;
                else if (pole->cells[r][c] == playerSymbol) count += 1;
            }
        }
    }
    return count;
}

int evaluateLine(struct Pole* pole, int row, int col, char symbol, int dx, int dy) {
    int count = 1;
    int x = col + dx;
    int y = row + dy;
    while (x >= 0 && x < pole->size && y >= 0 && y < pole->size && pole->cells[y][x] == symbol) {
        count++;
        x += dx;
        y += dy;
    }
    x = col - dx;
    y = row - dy;
    while (x >= 0 && x < pole->size && y >= 0 && y < pole->size && pole->cells[y][x] == symbol) {
        count++;
        x -= dx;
        y -= dy;
    }
    return count;
}

void evaluatePotentialMoves(struct Pole* pole, char botSymbol, int* bestRow, int* bestCol) {
    char playerSymbol = (botSymbol == 'X') ? 'O' : 'X';
    int maxScore = -1;
    int winLength = (pole->size == 3) ? 3 : (pole->size == 4) ? 4 : 5;
    int threatLength1 = (pole->size == 3) ? 2 : (pole->size == 4) ? 3 : 4;
    int threatLength2 = (pole->size == 3) ? 1 : (pole->size == 4) ? 2 : 3;

    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                int score = 0;
                pole->cells[i][j] = botSymbol;
                if (checkWin(pole, botSymbol)) score += 10000;
                int dx_dirs[] = { 1, 0, 1, 1 };
                int dy_dirs[] = { 0, 1, 1, -1 };
                for (int dir = 0; dir < 4; dir++) {
                    int botLine = evaluateLine(pole, i, j, botSymbol, dx_dirs[dir], dy_dirs[dir]);
                    if (botLine >= winLength) score += 5000;
                    else if (botLine == winLength - 1) score += 1000;
                    else if (botLine == winLength - 2) score += 200;
                    else if (botLine > 1) score += 50;
                }
                pole->cells[i][j] = ' ';
                pole->cells[i][j] = playerSymbol;
                if (checkWin(pole, playerSymbol)) score += 9000;
                else if (checkThreat(pole, playerSymbol, threatLength1)) score += 4000;
                else if (checkThreat(pole, playerSymbol, threatLength2)) score += 1000;
                for (int dir = 0; dir < 4; dir++) {
                    int playerLine = evaluateLine(pole, i, j, playerSymbol, dx_dirs[dir], dy_dirs[dir]);
                    if (playerLine >= winLength) score += 4500;
                    else if (playerLine == winLength - 1) score += 800;
                    else if (playerLine == winLength - 2) score += 150;
                    else if (playerLine > 1) score += 40;
                }
                pole->cells[i][j] = ' ';
                score += countNeighbors(pole, i, j, botSymbol, playerSymbol) * 10;
                int centerDist = abs(i - pole->size / 2) + abs(j - pole->size / 2);
                score -= centerDist;
                if (score > maxScore) {
                    maxScore = score;
                    *bestRow = i;
                    *bestCol = j;
                }
            }
        }
    }
}

int checkThreat(struct Pole* pole, char symbol, int threatLength) {
    int dx[] = { 1, 0, 1, 1 };
    int dy[] = { 0, 1, 1, -1 };
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = symbol;
                int isThreat = 0;
                for (int dir = 0; dir < 4; dir++) {
                    int count = 0;
                    int x = j, y = i;
                    while (x >= 0 && x < pole->size && y >= 0 && y < pole->size && pole->cells[y][x] == symbol) {
                        count++;
                        x += dx[dir];
                        y += dy[dir];
                    }
                    x = j - dx[dir];
                    y = i - dy[dir];
                    while (x >= 0 && x < pole->size && y >= 0 && y < pole->size && pole->cells[y][x] == symbol) {
                        count++;
                        x -= dx[dir];
                        y -= dy[dir];
                    }
                    if (count >= threatLength) {
                        isThreat = 1;
                        break;
                    }
                }
                pole->cells[i][j] = ' ';
                if (isThreat) return 1;
            }
        }
    }
    return 0;
}

void makeBotMoveEasy(struct Pole* pole, char botSymbol, int* out_row, int* out_col) {
    char playerSymbol = (botSymbol == 'X') ? 'O' : 'X';
    int threatLength = (pole->size == 3) ? 2 : (pole->size == 4) ? 3 : 4;

    if (pole->isSmallBoard) {
        
        int corners[][2] = { {0, 0}, {0, pole->size - 1}, {pole->size - 1, 0}, {pole->size - 1, pole->size - 1} };
        for (int k = 0; k < 4; k++) {
            int i = corners[k][0];
            int j = corners[k][1];
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                pole->last_move.abs_row = i;
                pole->last_move.abs_col = j;
                pole->last_move.symbol = botSymbol;
                pole->last_bot_move.abs_row = i;
                pole->last_bot_move.abs_col = j;
                pole->last_bot_move.symbol = botSymbol;
                pole->moves_count++;
                *out_row = i;
                *out_col = j;
                return;
            }
        }
        for (int i = 0; i < pole->size; i++) {
            for (int j = 0; j < pole->size; j++) {
                if (pole->cells[i][j] == ' ') {
                    pole->cells[i][j] = botSymbol;
                    if (checkWin(pole, botSymbol)) {
                        pole->last_move.abs_row = i;
                        pole->last_move.abs_col = j;
                        pole->last_move.symbol = botSymbol;
                        pole->last_bot_move.abs_row = i;
                        pole->last_bot_move.abs_col = j;
                        pole->last_bot_move.symbol = botSymbol;
                        pole->moves_count++;
                        *out_row = i;
                        *out_col = j;
                        return;
                    }
                    pole->cells[i][j] = ' ';
                }
            }
        }
        
        int sides[][2] = { {0, pole->size / 2}, {pole->size / 2, 0}, {pole->size / 2, pole->size - 1}, {pole->size - 1, pole->size / 2} };
        for (int k = 0; k < 4; k++) {
            int i = sides[k][0];
            int j = sides[k][1];
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                pole->last_move.abs_row = i;
                pole->last_move.abs_col = j;
                pole->last_move.symbol = botSymbol;
                pole->last_bot_move.abs_row = i;
                pole->last_bot_move.abs_col = j;
                pole->last_bot_move.symbol = botSymbol;
                pole->moves_count++;
                *out_row = i;
                *out_col = j;
                return;
            }
        }
        int center = pole->size/2;
        if (pole->cells[center][center] == ' ') {
            pole->cells[center][center] = botSymbol;
            pole->last_move.abs_row = center;
            pole->last_move.abs_col = center;
            pole->last_move.symbol = botSymbol;
            pole->last_bot_move.abs_row = center;
            pole->last_bot_move.abs_col = center;
            pole->last_bot_move.symbol = botSymbol;
            pole->moves_count++;
            *out_row = center;
            *out_col = center;
            return;
        }
        for (int i = 0; i < pole->size; i++) {
            for (int j = 0; j < pole->size; j++) {
                if (pole->cells[i][j] == ' ') {
                    pole->cells[i][j] = botSymbol;
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
            }
        }
        return;
    }

    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                if (checkWin(pole, botSymbol)) {
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
                pole->cells[i][j] = ' ';
            }
        }
    }
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = playerSymbol;
                if (checkWin(pole, playerSymbol)) {
                    pole->cells[i][j] = botSymbol;
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
                pole->cells[i][j] = ' ';
            }
        }
    }

    int nearMoves[1600][2];
    int nearCount = 0;
    int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    
    if (nearCount > 0) {
        int index = 0;
        int ni = nearMoves[index][0];
        int nj = nearMoves[index][1];
        pole->cells[ni][nj] = botSymbol;
        pole->last_move.abs_row = ni;
        pole->last_move.abs_col = nj;
        pole->last_move.symbol = botSymbol;
        pole->last_bot_move.abs_row = ni;
        pole->last_bot_move.abs_col = nj;
        pole->last_bot_move.symbol = botSymbol;
        pole->moves_count++;
        *out_row = ni;
        *out_col = nj;
        return;
    }

    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] != ' ') {
                for (int k = 0; k < 8; k++) {
                    int ni = i + dy[k];
                    int nj = j + dx[k];
                    if (ni >= 0 && ni < pole->size && nj >= 0 && nj < pole->size && pole->cells[ni][nj] == ' ') {
                        nearMoves[nearCount][0] = ni;
                        nearMoves[nearCount][1] = nj;
                        nearCount++;
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                pole->last_move.abs_row = i;
                pole->last_move.abs_col = j;
                pole->last_move.symbol = botSymbol;
                pole->last_bot_move.abs_row = i;
                pole->last_bot_move.abs_col = j;
                pole->last_bot_move.symbol = botSymbol;
                pole->moves_count++;
                *out_row = i;
                *out_col = j;
                return;
            }
        }
    }
}

void makeBotMoveHard(struct Pole* pole, char botSymbol, int* out_row, int* out_col) {
    char playerSymbol = (botSymbol == 'X') ? 'O' : 'X';
    int center = pole->size / 2;

    if (pole->isSmallBoard) {
        for (int i = 0; i < pole->size; i++) {
            for (int j = 0; j < pole->size; j++) {
                if (pole->cells[i][j] == ' ') {
                    pole->cells[i][j] = botSymbol;
                    if (checkWin(pole, botSymbol)) {
                        pole->last_move.abs_row = i;
                        pole->last_move.abs_col = j;
                        pole->last_move.symbol = botSymbol;
                        pole->last_bot_move.abs_row = i;
                        pole->last_bot_move.abs_col = j;
                        pole->last_bot_move.symbol = botSymbol;
                        pole->moves_count++;
                        *out_row = i;
                        *out_col = j;
                        return;
                    }
                    pole->cells[i][j] = ' ';
                }
            }
        }
        for (int i = 0; i < pole->size; i++) {
            for (int j = 0; j < pole->size; j++) {
                if (pole->cells[i][j] == ' ') {
                    pole->cells[i][j] = playerSymbol;
                    if (checkWin(pole, playerSymbol)) {
                        pole->cells[i][j] = botSymbol;
                        pole->last_move.abs_row = i;
                        pole->last_move.abs_col = j;
                        pole->last_move.symbol = botSymbol;
                        pole->last_bot_move.abs_row = i;
                        pole->last_bot_move.abs_col = j;
                        pole->last_bot_move.symbol = botSymbol;
                        pole->moves_count++;
                        *out_row = i;
                        *out_col = j;
                        return;
                    }
                    pole->cells[i][j] = ' ';
                }
            }
        }
        if (pole->cells[center][center] == ' ') {
            pole->cells[center][center] = botSymbol;
            pole->last_move.abs_row = center;
            pole->last_move.abs_col = center;
            pole->last_move.symbol = botSymbol;
            pole->last_bot_move.abs_row = center;
            pole->last_bot_move.abs_col = center;
            pole->last_bot_move.symbol = botSymbol;
            pole->moves_count++;
            *out_row = center;
            *out_col = center;
            return;
        }
        int corners[][2] = { {0, 0}, {0, pole->size - 1}, {pole->size - 1, 0}, {pole->size - 1, pole->size - 1} };
        int available_corners = 0;
        for (int k = 0; k < 4; k++) {
            if (pole->cells[corners[k][0]][corners[k][1]] == ' ') available_corners++;
        }
        if (available_corners > 0) {
            int rand_corner = rand() % available_corners;
            int found = 0;
            for (int k = 0; k < 4; k++) {
                if (pole->cells[corners[k][0]][corners[k][1]] == ' ') {
                    if (found == rand_corner) {
                        pole->cells[corners[k][0]][corners[k][1]] = botSymbol;
                        pole->last_move.abs_row = corners[k][0];
                        pole->last_move.abs_col = corners[k][1];
                        pole->last_move.symbol = botSymbol;
                        pole->last_bot_move.abs_row = corners[k][0];
                        pole->last_bot_move.abs_col = corners[k][1];
                        pole->last_bot_move.symbol = botSymbol;
                        pole->moves_count++;
                        *out_row = corners[k][0];
                        *out_col = corners[k][1];
                        return;
                    }
                    found++;
                }
            }
        }
        int sides[][2] = { {0, pole->size / 2}, {pole->size / 2, 0}, {pole->size / 2, pole->size - 1}, {pole->size - 1, pole->size / 2} };
        int available_sides = 0;
        for (int k = 0; k < 4; k++) {
            if (pole->cells[sides[k][0]][sides[k][1]] == ' ') available_sides++;
        }
        if (available_sides > 0) {
            int rand_side = rand() % available_sides;
            int found = 0;
            for (int k = 0; k < 4; k++) {
                if (pole->cells[sides[k][0]][sides[k][1]] == ' ') {
                    if (found == rand_side) {
                        pole->cells[sides[k][0]][sides[k][1]] = botSymbol;
                        pole->last_move.abs_row = sides[k][0];
                        pole->last_move.abs_col = sides[k][1];
                        pole->last_move.symbol = botSymbol;
                        pole->last_bot_move.abs_row = sides[k][0];
                        pole->last_bot_move.abs_col = sides[k][1];
                        pole->last_bot_move.symbol = botSymbol;
                        pole->moves_count++;
                        *out_row = sides[k][0];
                        *out_col = sides[k][1];
                        return;
                    }
                    found++;
                }
            }
        }
        for (int i = 0; i < pole->size; i++) {
            for (int j = 0; j < pole->size; j++) {
                if (pole->cells[i][j] == ' ') {
                    pole->cells[i][j] = botSymbol;
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
            }
        }
        return;
    }

    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                if (checkWin(pole, botSymbol)) {
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
                pole->cells[i][j] = ' ';
            }
        }
    }
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = playerSymbol;
                if (checkWin(pole, playerSymbol)) {
                    pole->cells[i][j] = botSymbol;
                    pole->last_move.abs_row = i;
                    pole->last_move.abs_col = j;
                    pole->last_move.symbol = botSymbol;
                    pole->last_bot_move.abs_row = i;
                    pole->last_bot_move.abs_col = j;
                    pole->last_bot_move.symbol = botSymbol;
                    pole->moves_count++;
                    *out_row = i;
                    *out_col = j;
                    return;
                }
                pole->cells[i][j] = ' ';
            }
        }
    }
    if (pole->cells[center][center] == ' ') {
        pole->cells[center][center] = botSymbol;
        pole->last_move.abs_row = center;
        pole->last_move.abs_col = center;
        pole->last_move.symbol = botSymbol;
        pole->last_bot_move.abs_row = center;
        pole->last_bot_move.abs_col = center;
        pole->last_bot_move.symbol = botSymbol;
        pole->moves_count++;
        *out_row = center;
        *out_col = center;
        return;
    }
    int bestRow = -1, bestCol = -1;
    evaluatePotentialMoves(pole, botSymbol, &bestRow, &bestCol);
    if (bestRow != -1 && bestCol != -1) {
        pole->cells[bestRow][bestCol] = botSymbol;
        pole->last_move.abs_row = bestRow;
        pole->last_move.abs_col = bestCol;
        pole->last_move.symbol = botSymbol;
        pole->last_bot_move.abs_row = bestRow;
        pole->last_bot_move.abs_col = bestCol;
        pole->last_bot_move.symbol = botSymbol;
        pole->moves_count++;
        *out_row = bestRow;
        *out_col = bestCol;
        return;
    }
    for (int i = 0; i < pole->size; i++) {
        for (int j = 0; j < pole->size; j++) {
            if (pole->cells[i][j] == ' ') {
                pole->cells[i][j] = botSymbol;
                pole->last_move.abs_row = i;
                pole->last_move.abs_col = j;
                pole->last_move.symbol = botSymbol;
                pole->last_bot_move.abs_row = i;
                pole->last_bot_move.abs_col = j;
                pole->last_bot_move.symbol = botSymbol;
                pole->moves_count++;
                *out_row = i;
                *out_col = j;
                return;
            }
        }
    }
}

void continueGame(struct Pole* pole, struct Igrok p1, struct Igrok p2, char nextSymbol, int isBotVsBot, char* winnerNameBuffer) {
    struct Igrok players[2] = { p1, p2 };
    int currentPlayer = (nextSymbol == p1.symbol) ? 0 : 1;
    while (1) {
        clearScreen();
        printPole(pole);
        if (!pole->isSmallBoard) {
            printLastMoves(pole, p1, p2);
        }
        struct Igrok cur = players[currentPlayer];
        if (!cur.isBot) {
            int row, col;
            while (1) {
                printf("Ваш ход (%c). введите координаты (строка и столбец от 1 до %d): ", cur.symbol, pole->size);
                if (scanf("%d %d", &row, &col) != 2) {
                    printf("Ошибка ввода. попробуйте снова.\n");
                    while (getchar() != '\n');
                    continue;
                }
                while (getchar() != '\n');
                if (row >= 1 && row <= pole->size && col >= 1 && col <= pole->size) {
                    int abs_row = row - 1;
                    int abs_col = col - 1;
                    if (pole->cells[abs_row][abs_col] == ' ') {
                        pole->cells[abs_row][abs_col] = cur.symbol;
                        pole->last_move.abs_row = abs_row;
                        pole->last_move.abs_col = abs_col;
                        pole->last_move.symbol = cur.symbol;
                        pole->last_human_move.abs_row = abs_row;
                        pole->last_human_move.abs_col = abs_col;
                        pole->last_human_move.symbol = cur.symbol;
                        pole->moves_count++;
                        players[currentPlayer].move_count++;
                        if (!pole->isSmallBoard) {
                            setViewToFitAllMoves(pole);
                        }
                        break;
                    }
                    else {
                        printf("Эта клетка уже занята. попробуйте снова.\n");
                    }
                }
                else {
                    printf("Неверные координаты. введите числа от 1 до %d.\n", pole->size);
                }
            }
        }
        else {
            int bot_abs_row, bot_abs_col;
            if (cur.difficulty == 1)
                makeBotMoveEasy(pole, cur.symbol, &bot_abs_row, &bot_abs_col);
            else
                makeBotMoveHard(pole, cur.symbol, &bot_abs_row, &bot_abs_col);
            players[currentPlayer].move_count++;
            if (!pole->isSmallBoard) {
                setViewToFitAllMoves(pole);
            }
        }

//#ifdef _WIN32
//        Sleep(3000);
//#else
//        sleep(3);
//#endif

        clearScreen();
        printPole(pole);
        if (!pole->isSmallBoard) {
            printLastMoves(pole, p1, p2);
        }
        if (checkWin(pole, cur.symbol)) {
            printf("%s победил(а)!\n", cur.name);
            strcpy(winnerNameBuffer, cur.name);
            return;
        }
        if (isBoardFull(pole)) {
            printf("Ничья!\n");
            strcpy(winnerNameBuffer, "Ничья");
            return;
        }
        currentPlayer = 1 - currentPlayer;
    }
}

void getGameSettings(struct GameSettings* settings, int mode) {
    settings->gameMode = mode;
    printf("Введите размер игрового поля (от 3 до 40): ");
    int size;
    while (1) {
        if (scanf("%d", &size) != 1 || size < 3 || size > 40) {
            printf("Неверный размер. Введите число от 3 до 40: ");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        break;
    }
    settings->sizeChoice = size;

    if (mode == 2) {
        printf("Выберите уровень сложности бота:\n");
        printf("1 - простой\n");
        printf("2 - сложный\n");
        scanf("%d", &settings->botDifficulty1);
        printf("За кого вы хотите играть?\n");
        printf("1 - x (ходит первым)\n");
        printf("2 - o (ходит вторым)\n");
        int choice;
        scanf("%d", &choice);
        settings->igrokSymbol = (choice == 1) ? 'X' : 'O';
    }
    else if (mode == 3) {
        printf("Выберите вариант ботов:\n");
        printf("1 - простой vs простой\n");
        printf("2 - сложный vs сложный\n");
        printf("3 - сложный vs простой\n");
        printf("4 - простой vs сложный\n");
        int choice;
        scanf("%d", &choice);
        if (choice == 1) {
            settings->botDifficulty1 = 1;
            settings->botDifficulty2 = 1;
        }
        else if (choice == 2) {
            settings->botDifficulty1 = 2;
            settings->botDifficulty2 = 2;
        }
        else if (choice == 3) {
            settings->botDifficulty1 = 2;
            settings->botDifficulty2 = 1;
        }
        else if (choice == 4) {
            settings->botDifficulty1 = 1;
            settings->botDifficulty2 = 2;
        }
        else {
            settings->botDifficulty1 = 2;
            settings->botDifficulty2 = 1;
        }
    }
}

void logGameResult(int gameMode, const char* winnerName, int isDraw, int botDiff1, int botDiff2, int sizeChoice, int p1Moves, int p2Moves, const char* p1Name, const char* p2Name) {
    FILE* file = fopen("game_results.txt", "a");
    if (!file) {
        printf("Не удалось открыть файл для записи результатов.\n");
        return;
    }
    time_t now = time(NULL);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(file, "Дата и время: %s\n", timeStr);
    fprintf(file, "Размер поля: %dx%d\n", sizeChoice, sizeChoice);
    if (gameMode == 1) {
        fprintf(file, "Режим: человек vs человек\n");
    }
    else if (gameMode == 2) {
        const char* diff = (botDiff1 == 1) ? "простой" : "сложный";
        fprintf(file, "Режим: человек vs бот (уровень %s)\n", diff);
    }
    else if (gameMode == 3) {
        const char* d1 = (botDiff1 == 1) ? "простой" : "сложный";
        const char* d2 = (botDiff2 == 1) ? "простой" : "сложный";
        fprintf(file, "Режим: бот (%s) vs бот (%s)\n", d1, d2);
    }
    fprintf(file, "Ходов %s: %d\n", p1Name, p1Moves);
    fprintf(file, "Ходов %s: %d\n", p2Name, p2Moves);
    if (isDraw) {
        fprintf(file, "Результат: ничья\n");
    }
    else {
        fprintf(file, "Победитель: %s\n", winnerName);
    }
    fprintf(file, "----------------------------------------\n");
    fclose(file);
}

int showMainMenu() {
    int choice;
    printf("\nМеню\n");
    printf("1 - Человек против человека\n");
    printf("2 - Человек против бота\n");
    printf("3 - Бот против бота\n");
    printf("4 - Показать историю игр\n");
    printf("0 - Выход\n");
    printf("Выберите действие: ");
    scanf("%d", &choice);
    return choice;
}

void showGameHistory() {
    FILE* file = fopen("game_results.txt", "r");
    if (!file) {
        printf("\nИстория игр пуста. Нет сохранённых результатов.\n");
        return;
    }
    printf("\nРезультаты\n");
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);
    }
    fclose(file);
    printf("\n");
}

int main() {
    setlocale(LC_ALL, "russian");
    srand(0);
    while (1) {
        int menuChoice = showMainMenu();
        if (menuChoice == 0) {
            break;
        }
        else if (menuChoice == 4) {
            showGameHistory();
            printf("Нажмите enter, чтобы вернуться в меню...");
            while (getchar() != '\n');
            getchar();
            continue;
        }
        else if (menuChoice < 1 || menuChoice > 3) {
            printf("Неверный выбор. попробуйте снова.\n");
            continue;
        }

        struct GameSettings settings;
        getGameSettings(&settings, menuChoice);
        struct Pole pole;
        initEmptyPole(&pole, settings.sizeChoice);
        setViewToFitAllMoves(&pole);

        if (settings.gameMode == 1) {
            struct Igrok igroki[2] = {
                {'X', 0, 0, "Игрок 1", 0},
                {'O', 0, 0, "Игрок 2", 0}
            };
            char winnerName[50];
            continueGame(&pole, igroki[0], igroki[1], 'X', 0, winnerName);
            int isDraw = (strcmp(winnerName, "Ничья") == 0);
            logGameResult(settings.gameMode, winnerName, isDraw, settings.botDifficulty1, settings.botDifficulty2, settings.sizeChoice, igroki[0].move_count, igroki[1].move_count, igroki[0].name, igroki[1].name);
        }
        else if (settings.gameMode == 2) {
            struct Igrok chel, bot;
            strcpy(chel.name, "Вы");
            strcpy(bot.name, "Бот");
            chel.symbol = settings.igrokSymbol;
            bot.symbol = (settings.igrokSymbol == 'X') ? 'O' : 'X';
            chel.isBot = 0;
            bot.isBot = 1;
            bot.difficulty = settings.botDifficulty1;
            chel.move_count = 0;
            bot.move_count = 0;

            if (bot.symbol == 'X') {
                int center = pole.size / 2;
                pole.cells[center][center] = bot.symbol;
                pole.last_move.abs_row = center;
                pole.last_move.abs_col = center;
                pole.last_move.symbol = bot.symbol;
                pole.last_bot_move.abs_row = center;
                pole.last_bot_move.abs_col = center;
                pole.last_bot_move.symbol = bot.symbol;
                pole.moves_count++;
                bot.move_count++;
                printf("%s сходил(а) на (%d, %d)\n", bot.name, center + 1, center + 1);

#ifdef _WIN32
                Sleep(3000);
#else
                sleep(3);
#endif

                setViewToFitAllMoves(&pole);
                char winnerName[50];
                continueGame(&pole, chel, bot, chel.symbol, 0, winnerName);
                int isDraw = (strcmp(winnerName, "Ничья") == 0);
                logGameResult(settings.gameMode, winnerName, isDraw, settings.botDifficulty1, settings.botDifficulty2, settings.sizeChoice, chel.move_count, bot.move_count, chel.name, bot.name);

            }
            else {
                int row, col;
                while (1) {
                    printf("Ваш ход (%c). Введите координаты (строка и столбец от 1 до %d): ", chel.symbol, pole.size);
                    if (scanf("%d %d", &row, &col) != 2) {
                        printf("Ошибка ввода. Попробуйте снова.\n");
                        while (getchar() != '\n');
                        continue;
                    }
                    while (getchar() != '\n');
                    if (row >= 1 && row <= pole.size && col >= 1 && col <= pole.size) {
                        int abs_row = row - 1;
                        int abs_col = col - 1;
                        if (pole.cells[abs_row][abs_col] == ' ') {
                            pole.cells[abs_row][abs_col] = chel.symbol;
                            pole.last_move.abs_row = abs_row;
                            pole.last_move.abs_col = abs_col;
                            pole.last_move.symbol = chel.symbol;
                            pole.last_human_move.abs_row = abs_row;
                            pole.last_human_move.abs_col = abs_col;
                            pole.last_human_move.symbol = chel.symbol;
                            pole.moves_count++;
                            chel.move_count++;
                            printf("%s сходил(а) на (%d, %d)\n", chel.name, row, col);
                            setViewToFitAllMoves(&pole);
                            char winnerName[50];
                            continueGame(&pole, chel, bot, bot.symbol, 0, winnerName);
                            int isDraw = (strcmp(winnerName, "Ничья") == 0);
                            logGameResult(settings.gameMode, winnerName, isDraw, settings.botDifficulty1, settings.botDifficulty2, settings.sizeChoice, chel.move_count, bot.move_count, chel.name, bot.name);
                            break;
                        }
                        else {
                            printf("Эта клетка уже занята. Попробуйте снова.\n");
                        }
                    }
                    else {
                        printf("Координаты должны быть от 1 до %d.\n", pole.size);
                    }
                }
            }
        }
        else if (settings.gameMode == 3) {
            struct Igrok bot1 = { 'X', 1, settings.botDifficulty1, "бот 1", 0 };
            struct Igrok bot2 = { 'O', 1, settings.botDifficulty2, "бот 2", 0 };
            int center = pole.size / 2;
            pole.cells[center][center] = bot1.symbol;
            pole.last_move.abs_row = center;
            pole.last_move.abs_col = center;
            pole.last_move.symbol = bot1.symbol;
            pole.last_bot_move.abs_row = center;
            pole.last_bot_move.abs_col = center;
            pole.last_bot_move.symbol = bot1.symbol;
            pole.moves_count++;
            bot1.move_count++;
            printf("%s сходил(а) на (%d, %d)\n", bot1.name, center + 1, center + 1);

#ifdef _WIN32
            Sleep(3000);
#else
            sleep(3);
#endif

            setViewToFitAllMoves(&pole);
            char winnerName[50];
            continueGame(&pole, bot1, bot2, bot2.symbol, 1, winnerName);
            int isDraw = (strcmp(winnerName, "Ничья") == 0);
            logGameResult(settings.gameMode, winnerName, isDraw, settings.botDifficulty1, settings.botDifficulty2, settings.sizeChoice, bot1.move_count, bot2.move_count, bot1.name, bot2.name);
        }
        printf("Хотите сыграть ещё раз? (y/n): ");
        char playAgain;
        scanf(" %c", &playAgain);
        if (playAgain != 'y' && playAgain != 'Y') {
            break;
        }
    }
    printf("Спасибо за игру!\n");
    return 0;
}