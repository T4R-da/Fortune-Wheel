#include "functions.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Helper to show the actual name of what you are hovering over
std::string getSelectionText(int id) {
    if (id == 0) return "0 (Green)";
    if (id >= 1 && id <= 36) return "Number " + std::to_string(id);
    if (id == 38) return "Column 3 (Top)";    // Top row (3,6,9...)
    if (id == 39) return "Column 2 (Middle)"; // Mid row (2,5,8...)
    if (id == 40) return "Column 1 (Bottom)"; // Bottom row (1,4,7...)
    if (id == 41) return "1st 12";
    if (id == 42) return "2nd 12";
    if (id == 43) return "3rd 12";
    if (id >= 44 && id <= 49) {
        std::string names[] = {"1-18", "Even", "RED", "BLACK", "Odd", "19-36"};
        return names[id - 44];
    }
    return "None";
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
        
        if (playerBalance <= 0) {
            playerBalance = 1000;
        }

        while (playerBalance > 0) { 
            clearScreen(); 
            printTitle(); 
            printRouletteTable(cursor); 
            
            // Added extra spaces at the end of these lines to overwrite any trailing letters from previous frames
            std::cout << YELLOW << "BALANCE: $" << playerBalance << RESET << " | " << MAGENTA << "NAVIGATE WITH ARROWS" << RESET << "                    \n";
            std::cout << "Current Selection: " << BOLD << getSelectionText(cursor) << RESET << " (Enter to Bet, 'X' to Cash Out)                    \n";

            int key = _getch();
            if (key == 224) { 
                key = _getch();
                if (key == 72) { // UP
                    cursor--;
                }
                else if (key == 80) { // DOWN
                    cursor++;
                }
                else if (key == 75) { // LEFT
                    cursor--;
                }
                else if (key == 77) { // RIGHT
                    if (cursor >= 1 && cursor <= 36) {
                        if (cursor % 3 == 0) cursor = 38;      
                        else if (cursor % 3 == 2) cursor = 39; 
                        else if (cursor % 3 == 1) cursor = 40; 
                    } else {
                        cursor++;
                    }
                }

                if (cursor < 0) cursor = 49;
                if (cursor > 49) cursor = 0;
                
                if (cursor == 37) {
                    if (key == 72 || key == 75) cursor = 36; 
                    else cursor = 38; 
                }
            } 
            else if (key == 'x' || key == 'X') {
                keepPlaying = false; 
                break;
            }
            else if (key == 13) { // ENTER
                std::cout << "\nEnter bet: $";
                int bet; 
                
                if (!(std::cin >> bet)) {
                    std::cin.clear(); 
                    std::cin.ignore(10000, '\n'); 
                    continue; 
                }
                
                std::cin.ignore(10000, '\n');

                if (bet <= 0 || bet > playerBalance) {
                    system("cls"); // Clear invalid bet attempts so they don't stack up
                    continue;
                }

                playerBalance -= bet;

                if (audio) ma_engine_play_sound(&engine, "spin.wav", NULL);
                rollAnimation(wheel); 
                const Pocket& res = wheel.spin(); 
                
                bool won = false;
                int mult = 0;

                if (cursor <= 36) { if(res.number == cursor) { won = true; mult = 36; } } 
                else if (cursor == 38 && res.number != 0 && res.number % 3 == 0) { won = true; mult = 3; }
                else if (cursor == 39 && res.number != 0 && res.number % 3 == 2) { won = true; mult = 3; }
                else if (cursor == 40 && res.number != 0 && res.number % 3 == 1) { won = true; mult = 3; }
                else if (cursor >= 41 && cursor <= 43) { /* Dozens Logic (Add this later!) */ } 
                else if (cursor == 46 && res.getColor() == "Red") { won = true; mult = 2; }
                else if (cursor == 47 && res.getColor() == "Black") { won = true; mult = 2; }
                else if (cursor == 45 && res.isEven()) { won = true; mult = 2; }
                else if (cursor == 48 && res.isOdd()) { won = true; mult = 2; }

                if (audio) ma_sound_stop(&bgm);

                if (won) {
                    int winAmt = bet * mult;
                    playerBalance += winAmt;
                    std::cout << GREEN << BOLD << "\nWINNER! Result: " << res.number << " " << res.getColor() << "\n";
                    std::cout << "You won: $" << winAmt << RESET << "\n";
                    // Shortened wait time from 3000ms to 1000ms
                    if (audio) { ma_engine_play_sound(&engine, "jackpot.wav", NULL); sleepMs(1000); }
                } else {
                    std::cout << RED << BOLD << "\nLOSS! Result: " << res.number << " " << res.getColor() << "\n";
                    std::cout << "You lost: -$" << bet << RESET << "\n";
                    // Shortened wait time from 2000ms to 800ms
                    if (audio) { ma_engine_play_sound(&engine, "fart.wav", NULL); sleepMs(800); }
                }

                if (audio) ma_sound_start(&bgm);

                // Fully wipes the screen so the spin animation doesn't linger at the bottom
                sleepMs(2000);
                system("cls"); 
            }
        }

        if (playerBalance <= 0) {
            system("cls");
            std::cout << RED << BOLD << "\nBANKRUPT! " << RESET << "You have run out of chips.\n";
            std::cout << "Press " << YELLOW << "'R'" << RESET << " to play again, or any other key to quit.\n";
            int restartKey = _getch();
            if (restartKey != 'r' && restartKey != 'R') {
                keepPlaying = false;
            } else {
                system("cls"); // Wipes the bankrupt screen before restarting
            }
        }
    }

    if (audio) { ma_sound_stop(&bgm); ma_engine_uninit(&engine); }
    return 0;
}