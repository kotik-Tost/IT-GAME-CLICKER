#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip> 
#include <ctime>   
#include <cstdlib> 

// Cross-platform key reading block
#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <sys/select.h> 

    int _getch() {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }

    int _kbhit() {
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        struct timeval tv = {0, 0};
        fd_set rdfs;
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        int ret = select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ret > 0;
    }
#endif

const std::string SAVE_FILE = "save.click";
const std::string ENCRYPTION_KEY = "NetClickerSecretKey2026!"; 

struct Upgrade {
    std::string icon;
    std::string name_en;
    std::string name_ru;
    long long cost;
    int cpc; 
    int cps; 
    int count;
};

struct Achievement {
    std::string title_en;
    std::string desc_en;
    std::string title_ru;
    std::string desc_ru;
    bool isUnlocked;
};

struct Map {
    std::string name_en;
    std::string name_ru;
    long long cost;
    double multiplier;
    bool isBought;
};

std::string encryptDecrypt(const std::string& data, const std::string& key) {
    std::string output = data;
    for (size_t i = 0; i < data.size(); ++i) {
        output[i] = data[i] ^ key[i % key.size()];
    }
    return output;
}

void saveGame(long long clicks, int cpc, int cps, const std::vector<Upgrade>& upgrades, 
              const std::vector<Achievement>& achievements, bool promo1, bool promo2, 
              int currentMapIdx, long long lastWheelTime) {
    std::stringstream ss;
    ss << clicks << "\n" << cpc << "\n" << cps << "\n";
    for (const auto& up : upgrades) {
        ss << up.count << "\n" << up.cost << "\n";
    }
    for (const auto& ach : achievements) {
        ss << ach.isUnlocked << "\n";
    }
    ss << promo1 << "\n" << promo2 << "\n";
    ss << currentMapIdx << "\n" << lastWheelTime << "\n"; 

    std::string encryptedData = encryptDecrypt(ss.str(), ENCRYPTION_KEY);
    std::ofstream saveFile(SAVE_FILE, std::ios::binary);
    if (saveFile.is_open()) {
        saveFile.write(encryptedData.data(), encryptedData.size());
        saveFile.close();
    }
}

void loadGame(long long& clicks, int& cpc, int& cps, std::vector<Upgrade>& upgrades, 
              std::vector<Achievement>& achievements, bool& promo1, bool& promo2, 
              int& currentMapIdx, long long& lastWheelTime, std::vector<Map>& maps) {
    std::ifstream saveFile(SAVE_FILE, std::ios::binary | std::ios::ate);
    if (saveFile.is_open()) {
        std::streamsize size = saveFile.tellg();
        saveFile.seekg(0, std::ios::beg);
        
        std::string encryptedData(size, ' ');
        if (saveFile.read(&encryptedData[size == 0 ? 0 : size - size], size)) { 
            std::string decryptedData = encryptDecrypt(encryptedData, ENCRYPTION_KEY);
            std::stringstream ss(decryptedData);
            
            ss >> clicks >> cpc >> cps;
            for (auto& up : upgrades) {
                ss >> up.count >> up.cost;
            }
            for (auto& ach : achievements) {
                ss >> ach.isUnlocked;
            }
            ss >> promo1 >> promo2;
            if (ss >> currentMapIdx >> lastWheelTime) {
                for (int i = 0; i <= currentMapIdx && i < maps.size(); ++i) {
                    maps[i].isBought = true;
                }
            }
        }
        saveFile.close();
    }
}

std::string getCurrentTime() {
    std::time_t now = std::time(nullptr);
    std::tm localTime;
#if defined(_MSC_VER)
    localtime_s(&localTime, &now); 
#else
    localTime = *std::localtime(&now); 
#endif
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << localTime.tm_hour << ":"
       << std::setw(2) << localTime.tm_min << ":" << std::setw(2) << localTime.tm_sec;
    return ss.str();
}

void enterPromoCode(long long& clicks, bool& promo1Used, bool& promo2Used, int lang) {
    std::system("cls || clear");
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "        🎟️ PROMO CODE ACTIVATION 🎟️     " << std::endl;
    else std::cout << "        🎟️ АКТИВАЦИЯ ПРОМОКОДА 🎟️       " << std::endl;
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "  Enter promo code: ";
    else std::cout << "  Введите промокод: ";
    
    std::string promo;
    std::cin >> promo;

    if (promo == "NET2026") { // Обновили имя промокода на безопасное
        if (!promo1Used) { 
            clicks += 5000; promo1Used = true; 
            if (lang == 1) std::cout << "\n  🎉 Success! Rewarded: +5,000 clicks!" << std::endl;
            else std::cout << "\n  🎉 Успешно! Начислено: +5000 кликов!" << std::endl;
        } else {
            if (lang == 1) std::cout << "\n  ❌ Already activated!" << std::endl;
            else std::cout << "\n  ❌ Код уже активирован!" << std::endl;
        }
    } 
    else if (promo == "CLICK") {
        if (!promo2Used) { 
            clicks += 10000; promo2Used = true; 
            if (lang == 1) std::cout << "\n  🎉 Success! Rewarded: +10,000 clicks!" << std::endl;
            else std::cout << "\n  🎉 Успешно! Начислено: +10 000 кликов!" << std::endl;
        } else {
            if (lang == 1) std::cout << "\n  ❌ Already activated!" << std::endl;
            else std::cout << "\n  ❌ Код уже активирован!" << std::endl;
        }
    } 
    else {
        if (lang == 1) std::cout << "\n  ❌ Invalid promo code!" << std::endl;
        else std::cout << "\n  ❌ Неверный промокод!" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    _getch(); 
}

void spinWheel(long long& clicks, long long& lastWheelTime, int lang) {
    std::system("cls || clear");
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "        🎡 WHEEL OF FORTUNE 🎡          " << std::endl;
    else std::cout << "        🎡 КОЛЕСО ФОРТУНЫ 🎡           " << std::endl;
    std::cout << "========================================" << std::endl;

    long long nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long timeLeft = 600 - (nowSeconds - lastWheelTime);

    if (timeLeft > 0) {
        if (lang == 1) {
            std::cout << "  ❌ Recharging! Please wait: " << timeLeft / 60 << "m " << timeLeft % 60 << "s" << std::endl;
        } else {
            std::cout << "  ❌ Перезарядка! Подождите: " << timeLeft / 60 << " мин. " << timeLeft % 60 << " сек." << std::endl;
        }
    } else {
        if (lang == 1) std::cout << "  ✨ Spinning... ✨\n" << std::endl;
        else std::cout << "  ✨ Колесо крутится... ✨\n" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        int chance = std::rand() % 100 + 1;
        long long prize = 0;

        if (chance == 1) { 
            prize = 10000;
            std::cout << "  🔥 🏆 JACKPOT!!! 🏆 🔥" << std::endl;
        } else { 
            prize = 100 + (std::rand() % 4901); 
            if (lang == 1) std::cout << "  🎉 You won a standard prize!" << std::endl;
            else std::cout << "  🎉 Вы выиграли обычный приз!" << std::endl;
        }
        clicks += prize;
        lastWheelTime = nowSeconds;
        if (lang == 1) std::cout << "  💰 Reward: +" << prize << " clicks!" << std::endl;
        else std::cout << "  💰 Награда: +" << prize << " кликов!" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    _getch();
}

void manageMaps(long long& clicks, int& currentMapIdx, std::vector<Map>& maps, int lang) {
    std::system("cls || clear");
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "          🗺️ LOCATION MAPS 🗺️          " << std::endl;
    else std::cout << "          🗺️ КАРТЫ И ЛОКАЦИИ 🗺️         " << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (lang == 1) std::cout << "  Current Map: " << maps[currentMapIdx].name_en << " (Multiplier: x" << maps[currentMapIdx].multiplier << ")" << std::endl;
    else std::cout << "  Текущая карта: " << maps[currentMapIdx].name_ru << " (Множитель: x" << maps[currentMapIdx].multiplier << ")" << std::endl;
    std::cout << "========================================" << std::endl;

    int nextMapIdx = currentMapIdx + 1;
    if (nextMapIdx < maps.size()) {
        if (lang == 1) {
            std::cout << "  Next Location: " << maps[nextMapIdx].name_en << "\n"
                      << "  Price: " << maps[nextMapIdx].cost << " clicks\n"
                      << "  Bonus: Increases multiplier to x" << maps[nextMapIdx].multiplier << "\n"
                      << "========================================" << std::endl;
            std::cout << "  Press [Y] to unlock this map location" << std::endl;
        } else {
            std::cout << "  Следующая локация: " << maps[nextMapIdx].name_ru << "\n"
                      << "  Цена: " << maps[nextMapIdx].cost << " кликов\n"
                      << "  Бонус: Увеличит множитель до x" << maps[nextMapIdx].multiplier << "\n"
                      << "========================================" << std::endl;
            std::cout << "  Нажмите [Y], чтобы купить карту локации" << std::endl;
        }
        
        int choice = _getch();
        if (choice == 'y' || choice == 'Y' || choice == 169 || choice == 233 || choice == -87 || choice == -119) {
            if (clicks >= maps[nextMapIdx].cost) {
                clicks -= maps[nextMapIdx].cost; 
                currentMapIdx = nextMapIdx;
                maps[nextMapIdx].isBought = true;
                if (lang == 1) std::cout << "\n  🎉 Map unlocked: " << maps[nextMapIdx].name_en << "!" << std::endl;
                else std::cout << "\n  🎉 Вы открыли карту: " << maps[nextMapIdx].name_ru << "!" << std::endl;
            } else {
                if (lang == 1) std::cout << "\n  ❌ Not enough clicks!" << std::endl;
                else std::cout << "\n  ❌ Недостаточно кликов!" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        if (lang == 1) std::cout << "  🏆 All maps unlocked! 🏆" << std::endl;
        else std::cout << "  🏆 Вы открыли абсолютно ВСЕ КАРТЫ! 🏆" << std::endl;
        _getch();
    }
}


void drawInterface(long long clicks, int cpc, int cps, const std::vector<Upgrade>& upgrades, 
                   const std::vector<Achievement>& achievements, const std::vector<Map>& maps, int currentMapIdx, int lang) {
    std::system("cls || clear"); 
    std::cout << "========================================" << std::endl;
    std::cout << "            🌐 NET CLICKER 🌐            " << std::endl;
    if (lang == 1) {
        std::cout << "        🕒 Time: " << getCurrentTime() << std::endl; 
        std::cout << "        🗺️ Map: " << maps[currentMapIdx].name_en << " (x" << maps[currentMapIdx].multiplier << ")" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  💰 Total Clicks: " << clicks << std::endl;
        std::cout << "  ⚡ Click Power (CPC): +" << static_cast<int>(cpc * maps[currentMapIdx].multiplier) << " (Base: " << cpc << ")" << std::endl;
        std::cout << "  ⏱️ Passive Income (CPS): +" << static_cast<int>(cps * maps[currentMapIdx].multiplier) << " /sec" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  [SPACE] - Click! 🖱️     | [P] - Promo Code 🎟️" << std::endl;
        std::cout << "  [F]     - Wheel of Fortune 🎡 | [M] - Buy MAP 🗺️" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "             UPGRADE SHOP:              " << std::endl;
    } else {
        std::cout << "        🕒 Ваше время: " << getCurrentTime() << std::endl; 
        std::cout << "        🗺️ Локация: " << maps[currentMapIdx].name_ru << " (x" << maps[currentMapIdx].multiplier << ")" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  💰 Баланс кликов: " << clicks << std::endl;
        std::cout << "  ⚡ Сила клика: +" << static_cast<int>(cpc * maps[currentMapIdx].multiplier) << " (Базовая: " << cpc << ")" << std::endl;
        std::cout << "  ⏱️ Пассивный доход: +" << static_cast<int>(cps * maps[currentMapIdx].multiplier) << " /сек" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  [ПРОБЕЛ] - Кликнуть! 🖱️  | [P] - Промокод 🎟️" << std::endl;
        std::cout << "  [F]      - Колесо Фортуны 🎡 | [M] - Купить КАРТУ 🗺️" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "          МАГАЗИН УЛУЧШЕНИЙ:            " << std::endl;
    }
    
    for (size_t i = 0; i < upgrades.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << upgrades[i].icon << " " << (lang == 1 ? upgrades[i].name_en : upgrades[i].name_ru) 
                  << " (" << (lang == 1 ? "Owned: " : "Куплено: ") << upgrades[i].count << ")\n"
                  << "      " << (lang == 1 ? "Price: " : "Цена: ") << upgrades[i].cost << " | ";
        if (upgrades[i].cpc > 0) std::cout << "+" << upgrades[i].cpc << (lang == 1 ? " to base CPC" : " к базовому клику");
        if (upgrades[i].cps > 0) std::cout << "+" << upgrades[i].cps << (lang == 1 ? " to base CPS" : " в базовую сек");
        std::cout << "\n";
    }
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "             ACHIEVEMENTS:              " << std::endl;
    else std::cout << "             ДОСТИЖЕНИЯ:                " << std::endl;
    
    for (const auto& ach : achievements) {
        if (lang == 1) {
            std::cout << "  " << (ach.isUnlocked ? "🏆 [UNLOCKED] " : "🔒 [LOCKED]   ") << ach.title_en << " - " << ach.desc_en << std::endl;
        } else {
            std::cout << "  " << (ach.isUnlocked ? "🏆 [ОТКРЫТО] " : "🔒 [ЗАКРЫТО] ") << ach.title_ru << " — " << ach.desc_ru << std::endl;
        }
    }
    std::cout << "========================================" << std::endl;
    if (lang == 1) std::cout << "  Press [ESC] to exit the game ❌" << std::endl;
    else std::cout << "  Нажмите [ESC], чтобы выйти из игры ❌" << std::endl;
    std::cout << "========================================" << std::endl;
}

bool checkAchievements(long long clicks, const std::vector<Upgrade>& upgrades, std::vector<Achievement>& achievements) {
    bool updated = false;
    if (achievements.size() > 0 && !achievements[0].isUnlocked && clicks >= 100) { achievements[0].isUnlocked = true; updated = true; }
    if (achievements.size() > 1 && !achievements[1].isUnlocked && clicks >= 500) { achievements[1].isUnlocked = true; updated = true; }
    if (achievements.size() > 2 && !achievements[2].isUnlocked && upgrades.size() > 5 && upgrades[5].count > 0) { achievements[2].isUnlocked = true; updated = true; }
    return updated;
}

int main() {
    std::srand(std::time(nullptr)); 
#if defined(_WIN32) || defined(_WIN64)
    std::system("chcp 65001 > nul"); 
#endif

    int language = 0;
    std::system("cls || clear");
    std::cout << "========================================" << std::endl;
    std::cout << "    SELECT LANGUAGE / УСТАНОВИТЕ ЯЗЫК     " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  1. English" << std::endl;
    std::cout << "  2. Русский" << std::endl;
    std::cout << "========================================" << std::endl;
    while (language != 1 && language != 2) {
        int ch = _getch();
        if (ch == '1') language = 1;
        if (ch == '2') language = 2;
    }

    long long googleClicks = 0;
    int totalClicksPerClick = 1;
    int totalClicksPerSecond = 0;
    
    bool google2026Used = false;
    bool clickUsed = false;
    int currentMapIdx = 0; 
    long long lastWheelTime = 0; 

    std::vector<Upgrade> upgrades = {
        {"🔍", "Net Search Bot", "Поисковый Бот Сети", 15, 0, 1, 0},
        {"🌐", "Browser Extension", "Расширение Браузера", 100, 0, 5, 0},
        {"📁", "Cloud Drive", "Облачное Хранилище", 500, 0, 20, 0},
        {"🤖", "System Auto-Tap", "Системный Авто-Кликер", 3000, 0, 90, 0},
        {"📺", "Premium Stream Click", "Премиум Стрим Клик", 10000, 25, 0, 0},
        {"🔬", "Quantum Core PC", "Квантовый Суперкомпьютер", 50000, 0, 500, 0}
    };

    std::vector<Achievement> achievements = {
        {"First Steps", "Reach 100 total clicks", "Первые шаги", "Набрать 100 кликов", false},
        {"Click Tycoon", "Reach 500 total clicks", "Клик-Магнат", "Набрать 500 кликов", false},
        {"Quantum Leap", "Purchase at least one Quantum Core PC", "Квантовый скачок", "Купить Квантовый Компьютер", false}
    };

    std::vector<Map> maps = {
        {"🏠 Starting Garage Office", "🏠 Стартовый Офис Гараж", 0, 1.0, true},
        {"🏢 Regional Data Center", "🏢 Региональный Дата-Центр", 2000, 1.5, false},
        {"🇺🇸 Global Tech Valley", "🇺🇸 Мировая Техно-Долина", 15000, 2.5, false},
        {"🚀 Net Orbital Station", "🚀 Орбитальная Станция Google", 100000, 5.0, false}
    };

    loadGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime, maps);

    auto lastTick = std::chrono::steady_clock::now();
    bool shouldRedraw = true;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTick).count() >= 1) {
            googleClicks += static_cast<int>(totalClicksPerSecond * maps[currentMapIdx].multiplier);
            lastTick = now;
            checkAchievements(googleClicks, upgrades, achievements);
            shouldRedraw = true;
            saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
        }

        if (shouldRedraw) {
            drawInterface(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, maps, currentMapIdx, language);
            shouldRedraw = false;
        }

        if (_kbhit()) {
            int ch = _getch();
            if (ch == 27) break; // ESC
            
            if (ch == 32) { // SPACE
                googleClicks += static_cast<int>(totalClicksPerClick * maps[currentMapIdx].multiplier);
                checkAchievements(googleClicks, upgrades, achievements);
                shouldRedraw = true;
                saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
            }
            if (ch == 'p' || ch == 'P' || ch == 167 || ch == 231 || ch == -120 || ch == -88) { // Promo Codes
                enterPromoCode(googleClicks, google2026Used, clickUsed, language);
                checkAchievements(googleClicks, upgrades, achievements);
                shouldRedraw = true;
                saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
            }
            if (ch == 'f' || ch == 'F' || ch == 164 || ch == 224 || ch == -128 || ch == -96) { // Wheel of Fortune
                spinWheel(googleClicks, lastWheelTime, language);
                checkAchievements(googleClicks, upgrades, achievements);
                shouldRedraw = true;
                saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
            }
            if (ch == 'm' || ch == 'M' || ch == 172 || ch == 236 || ch == -114 || ch == -82) { // Maps Shop
                manageMaps(googleClicks, currentMapIdx, maps, language);
                shouldRedraw = true;
                saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
            }
            if (ch >= '1' && ch <= '6') { // Upgrades Shop
                size_t idx = ch - '1';
                if (googleClicks >= upgrades[idx].cost) {
                    googleClicks -= upgrades[idx].cost;
                    upgrades[idx].count++;
                    totalClicksPerClick += upgrades[idx].cpc;
                    totalClicksPerSecond += upgrades[idx].cps;
                    upgrades[idx].cost = static_cast<long long>(upgrades[idx].cost * 1.5);
                    checkAchievements(googleClicks, upgrades, achievements);
                    saveGame(googleClicks, totalClicksPerClick, totalClicksPerSecond, upgrades, achievements, google2026Used, clickUsed, currentMapIdx, lastWheelTime);
                }
                shouldRedraw = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::system("cls || clear");
    if (language == 1) std::cout << "Game successfully saved to save.click! Exiting..." << std::endl;
    else std::cout << "Игра успешно сохранена в файл save.click! Выход..." << std::endl;
    return 0;
}
