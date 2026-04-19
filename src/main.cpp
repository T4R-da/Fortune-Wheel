#include "functions.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Helper to show the actual name of what you are hovering over
std::string getSelectionText(int id) {
    if (id == 0) return "0 (Green)";
    if (id >= 1 && id <= 36) return "Number " + std::to_string(id);
    if (id == 38) return "Top Row (3,6,9...36)";
    if (id == 39) return "Middle Row (2,5,8...35)";
    if (id == 40) return "Bottom Row (1,4,7...34)";
    if (id == 41) return "1st 12";
    if (id == 42) return "2nd 12";
    if (id == 43) return "3rd 12";
    if (id >= 44 && id <= 49) {
        std::string names[] = {"1-18", "Even", "RED", "BLACK", "Odd", "19-36"};
        return names[id - 44];
    }
    return "None";
}

// Evaluate a single bet against the result pocket; returns winnings (0 if lost)
int evaluateBet(const BetEntry& b, const Pocket& res) {
    bool won = false;
    int mult = 0;
    int cur = b.cursor;

    if (cur <= 36) { if (res.number == cur) { won = true; mult = 36; } }
    else if (cur == 38 && res.number != 0 && res.number % 3 == 0) { won = true; mult = 3; } // top row:    3,6,9,...,36
    else if (cur == 39 && res.number != 0 && res.number % 3 == 2) { won = true; mult = 3; } // middle row: 2,5,8,...,35
    else if (cur == 40 && res.number != 0 && res.number % 3 == 1) { won = true; mult = 3; } // bottom row: 1,4,7,...,34
    else if (cur == 41 && res.number >= 1  && res.number <= 12)   { won = true; mult = 3; }
    else if (cur == 42 && res.number >= 13 && res.number <= 24)   { won = true; mult = 3; }
    else if (cur == 43 && res.number >= 25 && res.number <= 36)   { won = true; mult = 3; }
    else if (cur == 44 && res.number >= 1  && res.number <= 18)   { won = true; mult = 2; }
    else if (cur == 49 && res.number >= 19 && res.number <= 36)   { won = true; mult = 2; }
    else if (cur == 46 && res.getColor() == "Red")   { won = true; mult = 2; }
    else if (cur == 47 && res.getColor() == "Black") { won = true; mult = 2; }
    else if (cur == 45 && res.isEven()) { won = true; mult = 2; }
    else if (cur == 48 && res.isOdd())  { won = true; mult = 2; }

    return won ? b.amount * mult : 0;
}

// ── Grid navigation helpers ──────────────────────────────────────────────────
// Numbers 1-36 are arranged as a 12-col x 3-row grid: num = col*3 + row
// col: 0-11 (left to right), row: 1-3 (bottom to top on screen)

// Returns column (0-11) of a number in the grid
inline int gridCol(int num) { return (num - 1) / 3; }
// Returns row (1-3) of a number in the grid
inline int gridRow(int num) { return (num - 1) % 3 + 1; }
// Returns the number at a given col and row
inline int gridNum(int col, int row) { return col * 3 + row; }

// Move cursor LEFT within the number grid (or wrap to outside bets area)
int moveCursorLeft(int cursor) {
    if (cursor >= 1 && cursor <= 36) {
        int col = gridCol(cursor);
        int row = gridRow(cursor);
        if (col == 0) return 0; // wrap to green 0
        return gridNum(col - 1, row);
    }
    // Column bets (38-40) go left into the rightmost column (col 11)
    if (cursor == 38) return gridNum(11, 3);
    if (cursor == 39) return gridNum(11, 2);
    if (cursor == 40) return gridNum(11, 1);
    // Outside bets row: shift left
    if (cursor >= 41 && cursor <= 49) {
        return (cursor == 41) ? 40 : cursor - 1;
    }
    return cursor;
}

// Move cursor RIGHT within the number grid (or into column bets)
int moveCursorRight(int cursor) {
    if (cursor >= 1 && cursor <= 36) {
        int col = gridCol(cursor);
        int row = gridRow(cursor);
        if (col == 11) {
            // Jump to matching column bet
            if (row == 3) return 38;
            if (row == 2) return 39;
            return 40;
        }
        return gridNum(col + 1, row);
    }
    // Column bets go right into dozens row
    if (cursor == 38 || cursor == 39 || cursor == 40) return 41;
    // Outside bets row: shift right
    if (cursor >= 41 && cursor <= 48) return cursor + 1;
    if (cursor == 49) return 0; // wrap to green
    // Green 0: go right into grid col 0
    if (cursor == 0) return gridNum(0, 2); // middle row
    return cursor;
}

// Move cursor UP (row 1->2->3, i.e. bottom->top visually on screen bottom->top)
int moveCursorUp(int cursor) {
    if (cursor >= 1 && cursor <= 36) {
        int col = gridCol(cursor);
        int row = gridRow(cursor);
        if (row == 3) return 0; // top row goes to green 0
        return gridNum(col, row + 1);
    }
    if (cursor == 0) return cursor; // already at top
    // Column bets: up goes to 0
    if (cursor >= 38 && cursor <= 40) return 0;
    // Dozens row: up goes into bottom row of numbers
    if (cursor >= 41 && cursor <= 43) {
        int col = (cursor == 41) ? 3 : (cursor == 42) ? 7 : 11;
        return gridNum(col, 1);
    }
    // Outside bets: up goes to dozens
    if (cursor >= 44 && cursor <= 49) return 41 + ((cursor - 44) / 2);
    return cursor;
}

// Move cursor DOWN
int moveCursorDown(int cursor) {
    if (cursor >= 1 && cursor <= 36) {
        int col = gridCol(cursor);
        int row = gridRow(cursor);
        if (row == 1) {
            // Bottom row drops into dozens
            if (col < 4) return 41;
            if (col < 8) return 42;
            return 43;
        }
        return gridNum(col, row - 1);
    }
    if (cursor == 0) return gridNum(5, 3); // green drops into mid-top grid
    // Column bets drop to dozens
    if (cursor == 38) return 43;
    if (cursor == 39) return 42;
    if (cursor == 40) return 41;
    // Dozens drop to outside bets
    if (cursor == 41) return 44;
    if (cursor == 42) return 46;
    if (cursor == 43) return 48;
    // Outside bets: already at bottom
    if (cursor >= 44 && cursor <= 49) return cursor;
    return cursor;
}

int main() {
    hideCursor();
    Booting();

    ma_engine engine;
    ma_sound bgm;
    bool audio = false;

    if (ma_engine_init(NULL, &engine) == MA_SUCCESS) {
        if (ma_sound_init_from_file(&engine, "sound.wav", MA_SOUND_FLAG_STREAM, NULL, NULL, &bgm) == MA_SUCCESS) {
            ma_sound_set_looping(&bgm, true);
            ma_sound_start(&bgm);
            audio = true;
        }
    }

    RouletteWheel wheel;
    bool keepPlaying = true;

    while (keepPlaying) {
        int cursor = 0;
        std::vector<BetEntry> betQueue;   // multi-bet accumulator
        int totalWagered = 0;

        if (playerBalance <= 0) {
            playerBalance = 1000;
        }

        while (playerBalance > 0) {
            clearScreen();
            printTitle();
            printRouletteTable(cursor);

            // Status bar
            std::cout << YELLOW << "BALANCE: $" << playerBalance << RESET
                      << " | " << MAGENTA << "WASD / ARROWS: move" << RESET
                      << " | " << GREEN   << "ENTER: queue bet" << RESET
                      << " | " << YELLOW  << "SPACE: spin all" << RESET
                      << " | " << RED     << "BKSP: undo last" << RESET
                      << " | " << BOLD    << "'X': cash out" << RESET
                      << "          \n";
            std::cout << "Selection: " << BOLD << getSelectionText(cursor) << RESET
                      << "                    \n";

            // Bet queue panel
            printBetQueue(betQueue, totalWagered);

            int key = _getch();

            // ── Navigation (WASD + Arrow keys) ──────────────────────────────
            if (key == 224) {           // arrow prefix
                int arrow = _getch();
                if (arrow == 72) cursor = moveCursorUp(cursor);    // UP
                else if (arrow == 80) cursor = moveCursorDown(cursor); // DOWN
                else if (arrow == 75) cursor = moveCursorLeft(cursor); // LEFT
                else if (arrow == 77) cursor = moveCursorRight(cursor); // RIGHT
            }
            else if (key == 'w' || key == 'W') cursor = moveCursorUp(cursor);
            else if (key == 's' || key == 'S') cursor = moveCursorDown(cursor);
            else if (key == 'a' || key == 'A') cursor = moveCursorLeft(cursor);
            else if (key == 'd' || key == 'D') cursor = moveCursorRight(cursor);
            else if (key == 'x' || key == 'X') {
                keepPlaying = false;
                break;
            }
            // ── Undo last queued bet ─────────────────────────────────────────
            else if (key == 8) { // Backspace
                if (!betQueue.empty()) {
                    totalWagered -= betQueue.back().amount;
                    playerBalance += betQueue.back().amount;
                    betQueue.pop_back();
                }
                system("cls");
                continue;
            }
            // ── Queue a new bet on current selection ─────────────────────────
            else if (key == 13) { // Enter
                // Enforce max 18 unique straight-up number bets (1-36)
                if (cursor >= 1 && cursor <= 36) {
                    // Count how many distinct numbers already have a bet
                    bool alreadyBet[37] = {};
                    for (const auto& b : betQueue)
                        if (b.cursor >= 1 && b.cursor <= 36)
                            alreadyBet[b.cursor] = true;

                    int uniqueNumberBets = 0;
                    for (int i = 1; i <= 36; ++i)
                        if (alreadyBet[i]) ++uniqueNumberBets;

                    if (!alreadyBet[cursor] && uniqueNumberBets >= 18) {
                        // Flash warning only when trying to add a NEW number beyond 18
                        std::cout << RED << BOLD
                                  << "\n  Max 18 number bets reached! Remove a bet first."
                                  << RESET << "          \n";
                        sleepMs(1200);
                        system("cls");
                        continue;
                    }
                }

                // Show cursor so they can type
                HANDLE ch = GetStdHandle(STD_OUTPUT_HANDLE);
                CONSOLE_CURSOR_INFO ci; ci.dwSize = 10; ci.bVisible = TRUE;
                SetConsoleCursorInfo(ch, &ci);

                int availableBalance = playerBalance - totalWagered;
                std::cout << "\nBet amount for [" << getSelectionText(cursor)
                          << "] (available: $" << availableBalance << "): $";
                int bet = 0;
                if (!(std::cin >> bet)) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                } else {
                    std::cin.ignore(10000, '\n');
                    if (bet > 0 && bet <= availableBalance) {
                        totalWagered += bet;
                        playerBalance -= bet;
                        betQueue.push_back({ cursor, bet, getSelectionText(cursor) });
                    }
                }

                ci.bVisible = FALSE; SetConsoleCursorInfo(ch, &ci);
                system("cls");
                continue;
            }
            // ── Spin all queued bets ─────────────────────────────────────────
            else if (key == ' ') {
                if (betQueue.empty()) continue;

                if (audio) ma_engine_play_sound(&engine, "spin.wav", NULL);
                rollAnimation(wheel);
                const Pocket& res = wheel.spin();

                if (audio) ma_sound_stop(&bgm);

                // Evaluate each bet
                int totalWon = 0;
                bool anyWin = false;

                std::cout << "\n" << BOLD << "Result: " << res.number << " " << res.getColor() << RESET << "\n";
                std::cout << "─────────────────────────────────────\n";

                for (const auto& b : betQueue) {
                    int winnings = evaluateBet(b, res);
                    if (winnings > 0) {
                        anyWin = true;
                        totalWon += winnings;
                        std::cout << GREEN << " WIN " << winnings
                                  << " on " << b.label << RESET << "\n";
                    } else {
                        std::cout << RED << " LOSS " << b.amount
                                  << " on " << b.label << RESET << "\n";
                    }
                }

                std::cout << "─────────────────────────────────────\n";
                playerBalance += totalWon;

                if (anyWin) {
                    std::cout << GREEN << BOLD << "Net result: +"
                              << (totalWon - totalWagered) << "  Balance: $"
                              << playerBalance << RESET << "\n";
                    if (audio) { ma_engine_play_sound(&engine, "win.wav", NULL); sleepMs(3500); }
                } else {
                    std::cout << RED << BOLD << "Net result: -" << totalWagered
                              << "  Balance: $" << playerBalance << RESET << "\n";
                    if (audio) { ma_engine_play_sound(&engine, "loss.wav", NULL); sleepMs(4000); }
                }

                if (audio) ma_sound_start(&bgm);

                // Clear queue for next round
                betQueue.clear();
                totalWagered = 0;
                system("cls");
                continue;
            }
        }

        if (playerBalance <= 0 && keepPlaying) {
            if (audio) { ma_sound_stop(&bgm); ma_engine_play_sound(&engine, "loss.wav", NULL); sleepMs(4000); }
            system("cls");
            std::cout << RED << BOLD << "\nBANKRUPT! " << RESET << "You have run out of chips.\n";
            std::cout << "Press " << YELLOW << "'R'" << RESET << " to play again, or any other key to quit.\n";
            int restartKey = _getch();
            if (restartKey != 'r' && restartKey != 'R') {
                keepPlaying = false;
            } else {
                system("cls");
            }
        }
    }

    if (audio) { ma_sound_stop(&bgm); ma_engine_uninit(&engine); }
    return 0;
}