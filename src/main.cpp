#include "functions.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Helper to show the actual name of what you are hovering over
std::string getSelectionText(int id) {
    if (id == 0) return "0 (Green)";
    if (id >= 1 && id <= 36) return "Number " + std::to_string(id);
    if (id == 38) return "Column 3 (Top)";
    if (id == 39) return "Column 2 (Middle)";
    if (id == 40) return "Column 1 (Bottom)";
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
    else if (cur == 38 && res.number != 0 && res.number % 3 == 0) { won = true; mult = 3; }
    else if (cur == 39 && res.number != 0 && res.number % 3 == 2) { won = true; mult = 3; }
    else if (cur == 40 && res.number != 0 && res.number % 3 == 1) { won = true; mult = 3; }
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
            bool moved = false;
            int direction = 0; // -1 left/up, +1 right/down

            if (key == 224) {           // arrow prefix
                key = _getch();
                if      (key == 72) { direction = -1; moved = true; } // UP
                else if (key == 80) { direction =  1; moved = true; } // DOWN
                else if (key == 75) { direction = -1; moved = true; } // LEFT
                else if (key == 77) { direction =  1; moved = true; } // RIGHT (special)

                if (direction == 1 && key == 77) { // RIGHT arrow special column jump
                    if (cursor >= 1 && cursor <= 36) {
                        if      (cursor % 3 == 0) cursor = 38;
                        else if (cursor % 3 == 2) cursor = 39;
                        else if (cursor % 3 == 1) cursor = 40;
                        moved = false; // skip generic increment
                    }
                }
            } else if (key == 'w' || key == 'W') { direction = -1; moved = true; }
              else if (key == 's' || key == 'S') { direction =  1; moved = true; }
              else if (key == 'a' || key == 'A') { direction = -1; moved = true; }
              else if (key == 'd' || key == 'D') {
                // D also does the column jump like right arrow
                if (cursor >= 1 && cursor <= 36) {
                    if      (cursor % 3 == 0) cursor = 38;
                    else if (cursor % 3 == 2) cursor = 39;
                    else if (cursor % 3 == 1) cursor = 40;
                } else {
                    cursor++;
                }
            } else if (key == 'x' || key == 'X') {
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
                        std::cout << GREEN << "  WIN  $" << winnings
                                  << " on " << b.label << RESET << "\n";
                    } else {
                        std::cout << RED << "  LOSS $" << b.amount
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

            // Apply generic movement
            if (moved) {
                cursor += direction;
                if (cursor < 0)  cursor = 49;
                if (cursor > 49) cursor = 0;
                if (cursor == 37) {
                    cursor = (direction < 0) ? 36 : 38;
                }
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