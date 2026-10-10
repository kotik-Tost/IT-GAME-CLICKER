// ============================================================
//  NET CLICKER - Windows Edition (Full, with AntiCheat)
//  Compile:
//    g++ -std=c++17 -O2 -static -o NetClicker.exe main.cpp
// ============================================================

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
#include <random>
#include <cctype>
#include <deque>
#include <cmath>

#include <windows.h>
#include <conio.h>

// ============================================================
//  PLATFORM (Windows)
// ============================================================
namespace platform {

inline int getch_impl() { return _getch(); }
inline bool kbhit_impl() { return _kbhit() != 0; }

inline void clear_screen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = {0, 0};
    if (hConsole == INVALID_HANDLE_VALUE) return;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterA(hConsole, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hConsole, homeCoords);
}

inline void setup_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

inline std::string get_save_path() {
    const char* appdata = std::getenv("APPDATA");
    std::string base = appdata ? appdata : ".";
    std::string dir = base + "\\NetClicker";
    CreateDirectoryA(dir.c_str(), nullptr);
    return dir + "\\save.click";
}

} // namespace platform

// ============================================================
//  CRYPTO
// ============================================================
namespace crypto {

inline const std::string& get_key() {
    static const std::string KEY = "NetClickerSecretKey2026!xZ9qLm3P";
    return KEY;
}

inline std::string xor_cipher(const std::string& data, const std::string& key) {
    std::string out = data;
    for (size_t i = 0; i < data.size(); ++i)
        out[i] = static_cast<char>(data[i] ^ key[i % key.size()]);
    return out;
}

inline const char* b64_chars() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

inline std::string base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64_chars()[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64_chars()[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

inline std::string base64_decode(const std::string& in) {
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[b64_chars()[i]] = i;
    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

inline std::string encrypt_data(const std::string& data) {
    return base64_encode(xor_cipher(data, get_key()));
}

inline std::string decrypt_data(const std::string& data) {
    return xor_cipher(base64_decode(data), get_key());
}

} // namespace crypto

// ============================================================
//  GAME STRUCTS
// ============================================================
struct Upgrade {
    std::string icon;
    std::string name_en, name_ru;
    long long cost;
    int cpc, cps, count;
};

struct Achievement {
    std::string title_en, desc_en;
    std::string title_ru, desc_ru;
    bool isUnlocked;
};

struct Map {
    std::string name_en, name_ru;
    long long cost;
    double multiplier;
    bool isBought;
};

// ============================================================
//  ANTI-CHEAT
// ============================================================
struct AntiCheat {
    bool spaceHeld = false;
    std::deque<long long> clickTimes;
    double suspicion = 0.0;
    int strikes = 0;
    long long blockUntilMs = 0;
    bool punished = false;
    long long punishedUntilMs = 0;
};

inline long long now_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

inline long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline std::string get_current_time() {
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << localTime.tm_hour << ":"
       << std::setw(2) << localTime.tm_min << ":" << std::setw(2) << localTime.tm_sec;
    return ss.str();
}

inline long long safe_add(long long a, long long b) {
    constexpr long long MAX = 9'000'000'000'000'000'000LL;
    if (a > MAX - b) return MAX;
    return a + b;
}

// ============================================================
//  I18N
// ============================================================
struct Strings {
    std::string title, select_lang, time, map, balance, click_power, passive;
    std::string controls1, controls2, shop, achievements, owned, price;
    std::string base_cpc, base_cps, unlocked, locked, press_esc;
    std::string promo_title, promo_enter, promo_ok, promo_used, promo_bad;
    std::string wheel_title, wheel_wait, wheel_spin, wheel_jackpot;
    std::string wheel_normal, wheel_reward;
    std::string maps_title, maps_current, maps_next, maps_buy;
    std::string maps_unlocked, maps_all, maps_no_money;
    std::string not_enough, saved_exit;
    std::string autoclicker_detected, suspicious, clicks_blocked;
};

inline Strings get_strings(int lang) {
    Strings s;
    if (lang == 1) {
        s = {
            "NET CLICKER", "SELECT LANGUAGE / УСТАНОВИТЕ ЯЗЫК",
            "Time", "Map", "Total Clicks", "Click Power (CPC)", "Passive Income (CPS)",
            "[SPACE] - Click!        | [P] - Promo Code",
            "[F] - Wheel of Fortune  | [M] - Buy MAP",
            "UPGRADE SHOP", "ACHIEVEMENTS", "Owned", "Price",
            "to base CPC", "to base CPS", "UNLOCKED", "LOCKED", "Press [ESC] to exit",
            "PROMO CODE ACTIVATION", "Enter promo code", "Success! Rewarded",
            "Already activated!", "Invalid promo code!",
            "WHEEL OF FORTUNE", "Recharging! Please wait", "Spinning...",
            "JACKPOT!!!", "You won a standard prize!", "Reward",
            "LOCATION MAPS", "Current Map", "Next Location",
            "Press [Y] to unlock this map location", "Map unlocked",
            "All maps unlocked!", "Not enough clicks!",
            "Not enough clicks!", "Game saved! Exiting...",
            "AUTO-CLICKER DETECTED! Blocked",
            "Suspicious activity",
            "Clicks blocked"
        };
    } else {
        s = {
            "NET CLICKER", "УСТАНОВИТЕ ЯЗЫК / SELECT LANGUAGE",
            "Ваше время", "Локация", "Баланс кликов", "Сила клика", "Пассивный доход",
            "[ПРОБЕЛ] - Кликнуть!   | [P] - Промокод",
            "[F] - Колесо Фортуны   | [M] - Купить КАРТУ",
            "МАГАЗИН УЛУЧШЕНИЙ", "ДОСТИЖЕНИЯ", "Куплено", "Цена",
            "к базовому клику", "в базовую сек", "ОТКРЫТО", "ЗАКРЫТО",
            "Нажмите [ESC] для выхода",
            "АКТИВАЦИЯ ПРОМОКОДА", "Введите промокод", "Успешно! Начислено",
            "Код уже активирован!", "Неверный промокод!",
            "КОЛЕСО ФОРТУНЫ", "Перезарядка! Подождите", "Колесо крутится...",
            "ДЖЕКПОТ!!!", "Вы выиграли обычный приз!", "Награда",
            "КАРТЫ И ЛОКАЦИИ", "Текущая карта", "Следующая локация",
            "Нажмите [Y], чтобы купить карту локации", "Вы открыли карту",
            "Вы открыли ВСЕ КАРТЫ!", "Недостаточно кликов!",
            "Недостаточно кликов!", "Игра сохранена! Выход...",
            "ОБНАРУЖЕН АВТОКЛИКЕР! Блокировка",
            "Подозрительная активность",
            "Клики заблокированы"
        };
    }
    return s;
}

// ============================================================
//  SAVE / LOAD
// ============================================================
void save_game(long long clicks, int cpc, int cps,
               const std::vector<Upgrade>& upgrades,
               const std::vector<Achievement>& achievements,
               bool promo1, bool promo2,
               int currentMapIdx, long long lastWheelTime) {
    std::stringstream ss;
    ss << clicks << "\n" << cpc << "\n" << cps << "\n";
    for (const auto& up : upgrades) ss << up.count << "\n" << up.cost << "\n";
    for (const auto& ach : achievements) ss << ach.isUnlocked << "\n";
    ss << promo1 << "\n" << promo2 << "\n";
    ss << currentMapIdx << "\n" << lastWheelTime << "\n";

    std::string encrypted = crypto::encrypt_data(ss.str());
    std::ofstream f(platform::get_save_path(), std::ios::binary | std::ios::trunc);
    if (f.is_open()) f.write(encrypted.data(), (std::streamsize)encrypted.size());
}

void load_game(long long& clicks, int& cpc, int& cps,
               std::vector<Upgrade>& upgrades,
               std::vector<Achievement>& achievements,
               bool& promo1, bool& promo2,
               int& currentMapIdx, long long& lastWheelTime,
               std::vector<Map>& maps) {
    std::ifstream f(platform::get_save_path(), std::ios::binary | std::ios::ate);
    if (!f.is_open()) return;

    std::streamsize size = f.tellg();
    if (size <= 0) { f.close(); return; }

    f.seekg(0, std::ios::beg);
    std::string buffer((size_t)size, '\0');
    if (!f.read(buffer.data(), size)) { f.close(); return; }
    f.close();

    std::string decrypted = crypto::decrypt_data(buffer);
    std::stringstream ss(decrypted);

    ss >> clicks >> cpc >> cps;
    for (auto& up : upgrades) ss >> up.count >> up.cost;
    for (auto& ach : achievements) ss >> ach.isUnlocked;
    ss >> promo1 >> promo2;
    if (ss >> currentMapIdx >> lastWheelTime) {
        if (currentMapIdx < 0) currentMapIdx = 0;
        if (currentMapIdx >= (int)maps.size()) currentMapIdx = (int)maps.size() - 1;
        for (int i = 0; i <= currentMapIdx; ++i) maps[i].isBought = true;
    }
}

// ============================================================
//  PROMOCODES
// ============================================================
static const std::string PROMO1 = "NET2026";
static const std::string PROMO2 = "CLICK";

void enter_promo(long long& clicks, bool& p1, bool& p2, const Strings& S) {
    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "        🎟️ " << S.promo_title << " 🎟️\n";
    std::cout << "========================================\n";
    std::cout << "  " << S.promo_enter << ": ";

    std::string promo;
    std::cin >> promo;

    if (promo == PROMO1) {
        if (!p1) {
            clicks = safe_add(clicks, 5000); p1 = true;
            std::cout << "\n  🎉 " << S.promo_ok << ": +5,000!\n";
        } else std::cout << "\n  ❌ " << S.promo_used << "\n";
    } else if (promo == PROMO2) {
        if (!p2) {
            clicks = safe_add(clicks, 10000); p2 = true;
            std::cout << "\n  🎉 " << S.promo_ok << ": +10,000!\n";
        } else std::cout << "\n  ❌ " << S.promo_used << "\n";
    } else {
        std::cout << "\n  ❌ " << S.promo_bad << "\n";
    }
    std::cout << "========================================\n";
    platform::getch_impl();
}

// ============================================================
//  WHEEL OF FORTUNE
// ============================================================
void spin_wheel(long long& clicks, long long& lastWheelTime, const Strings& S) {
    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "        🎡 " << S.wheel_title << " 🎡\n";
    std::cout << "========================================\n";

    long long now = now_seconds();
    long long timeLeft = (lastWheelTime == 0) ? 0 : (600 - (now - lastWheelTime));

    if (timeLeft > 0) {
        std::cout << "  ❌ " << S.wheel_wait << ": "
                  << timeLeft / 60 << "m " << timeLeft % 60 << "s\n";
    } else {
        std::cout << "  ✨ " << S.wheel_spin << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));

        static std::mt19937 rng((unsigned)std::time(nullptr));
        std::uniform_int_distribution<int> chanceDist(1, 100);
        int chance = chanceDist(rng);
        long long prize = 0;

        if (chance == 1) {
            prize = 10000;
            std::cout << "  🔥 🏆 " << S.wheel_jackpot << " 🏆 🔥\n";
        } else {
            std::uniform_int_distribution<long long> prizeDist(100, 5000);
            prize = prizeDist(rng);
            std::cout << "  🎉 " << S.wheel_normal << "\n";
        }
        clicks = safe_add(clicks, prize);
        lastWheelTime = now;
        std::cout << "  💰 " << S.wheel_reward << ": +" << prize << "\n";
    }
    std::cout << "========================================\n";
    platform::getch_impl();
}

// ============================================================
//  MAPS
// ============================================================
void manage_maps(long long& clicks, int& currentMapIdx, std::vector<Map>& maps,
                 const Strings& S, int lang) {
    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "        🗺️ " << S.maps_title << " 🗺️\n";
    std::cout << "========================================\n";
    std::cout << "  " << S.maps_current << ": "
              << (lang == 1 ? maps[currentMapIdx].name_en : maps[currentMapIdx].name_ru)
              << " (x" << maps[currentMapIdx].multiplier << ")\n";
    std::cout << "========================================\n";

    int nextIdx = currentMapIdx + 1;
    if (nextIdx < (int)maps.size()) {
        std::cout << "  " << S.maps_next << ": "
                  << (lang == 1 ? maps[nextIdx].name_en : maps[nextIdx].name_ru) << "\n"
                  << "  " << S.price << ": " << maps[nextIdx].cost << "\n"
                  << "  Bonus: x" << maps[nextIdx].multiplier << "\n";
        std::cout << "========================================\n";
        std::cout << "  " << S.maps_buy << "\n";

        int choice = platform::getch_impl();
        bool is_yes = (choice == 'y' || choice == 'Y');
        if (!is_yes && (choice == 0xD0 || choice == -48)) {
            int second = platform::getch_impl();
            if (second == 0xBD || second == -67 || second == 0x9D || second == -99)
                is_yes = true;
        }

        if (is_yes) {
            if (clicks >= maps[nextIdx].cost) {
                clicks -= maps[nextIdx].cost;
                currentMapIdx = nextIdx;
                maps[nextIdx].isBought = true;
                std::cout << "\n  🎉 " << S.maps_unlocked << ": "
                          << (lang == 1 ? maps[nextIdx].name_en : maps[nextIdx].name_ru) << "!\n";
            } else {
                std::cout << "\n  ❌ " << S.maps_no_money << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::cout << "  🏆 " << S.maps_all << " 🏆\n";
        platform::getch_impl();
    }
}

// ============================================================
//  UI
// ============================================================
void draw_interface(long long clicks, int cpc, int cps,
                    const std::vector<Upgrade>& upgrades,
                    const std::vector<Achievement>& achievements,
                    const std::vector<Map>& maps, int currentMapIdx,
                    int lang, const Strings& S,
                    const AntiCheat& antiCheat) {
    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "            🌐 " << S.title << " 🌐\n";
    std::cout << "        🕒 " << S.time << ": " << get_current_time() << "\n";
    std::cout << "        🗺️ " << S.map << ": "
              << (lang == 1 ? maps[currentMapIdx].name_en : maps[currentMapIdx].name_ru)
              << " (x" << maps[currentMapIdx].multiplier << ")\n";
    std::cout << "========================================\n";
    std::cout << "  💰 " << S.balance << ": " << clicks << "\n";
    std::cout << "  ⚡ " << S.click_power << ": +"
              << (long long)(cpc * maps[currentMapIdx].multiplier)
              << " (Base: " << cpc << ")\n";
    std::cout << "  ⏱️ " << S.passive << ": +"
              << (long long)(cps * maps[currentMapIdx].multiplier) << " /sec\n";
    std::cout << "========================================\n";
    std::cout << "  " << S.controls1 << "\n";
    std::cout << "  " << S.controls2 << "\n";
    std::cout << "========================================\n";
    std::cout << "          " << S.shop << ":\n";

    for (size_t i = 0; i < upgrades.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << upgrades[i].icon << " "
                  << (lang == 1 ? upgrades[i].name_en : upgrades[i].name_ru)
                  << " (" << S.owned << ": " << upgrades[i].count << ")\n"
                  << "      " << S.price << ": " << upgrades[i].cost << " | ";
        if (upgrades[i].cpc > 0) std::cout << "+" << upgrades[i].cpc << " " << S.base_cpc;
        if (upgrades[i].cps > 0) std::cout << "+" << upgrades[i].cps << " " << S.base_cps;
        std::cout << "\n";
    }
    std::cout << "========================================\n";
    std::cout << "          " << S.achievements << ":\n";
    for (const auto& ach : achievements) {
        std::cout << "  "
                  << (ach.isUnlocked ? "🏆 [" + S.unlocked + "] " : "🔒 [" + S.locked + "] ")
                  << (lang == 1 ? ach.title_en : ach.title_ru) << " - "
                  << (lang == 1 ? ach.desc_en : ach.desc_ru) << "\n";
    }

    // Индикатор античита
    long long t = now_ms();
    if (antiCheat.punished && t < antiCheat.punishedUntilMs) {
        long long left = (antiCheat.punishedUntilMs - t) / 1000;
        std::cout << "========================================\n";
        std::cout << "  🚫 " << S.autoclicker_detected << ": " << left << "s\n";
        std::cout << "========================================\n";
    } else if (antiCheat.suspicion > 30.0) {
        std::cout << "========================================\n";
        std::cout << "  ⚠️  " << S.suspicious << ": "
                  << (int)antiCheat.suspicion << "%\n";
        std::cout << "========================================\n";
    }

    std::cout << "========================================\n";
    std::cout << "  " << S.press_esc << "\n";
    std::cout << "========================================\n";
}

// ============================================================
//  ACHIEVEMENTS
// ============================================================
bool check_achievements(long long clicks, const std::vector<Upgrade>& upgrades,
                        std::vector<Achievement>& achievements) {
    bool updated = false;
    if (achievements.size() > 0 && !achievements[0].isUnlocked && clicks >= 100) {
        achievements[0].isUnlocked = true; updated = true;
    }
    if (achievements.size() > 1 && !achievements[1].isUnlocked && clicks >= 500) {
        achievements[1].isUnlocked = true; updated = true;
    }
    if (achievements.size() > 2 && !achievements[2].isUnlocked &&
        upgrades.size() > 5 && upgrades[5].count > 0) {
        achievements[2].isUnlocked = true; updated = true;
    }
    return updated;
}

// ============================================================
//  CLICK HANDLER (с античитом)
// ============================================================
void handle_click(long long& clicks, int cpc, const std::vector<Map>& maps,
                  int currentMapIdx, std::vector<Upgrade>& upgrades,
                  std::vector<Achievement>& achievements,
                  AntiCheat& ac, bool& shouldRedraw, bool& dirty) {
    if (ac.spaceHeld) return; // защита от зажатия
    ac.spaceHeld = true;

    long long t = now_ms();

    // Если действует блокировка
    if (t < ac.blockUntilMs) {
        shouldRedraw = true;
        return;
    }

    // Записываем клик
    ac.clickTimes.push_back(t);

    // Убираем старые (старше 1 сек)
    while (!ac.clickTimes.empty() && t - ac.clickTimes.front() > 1000) {
        ac.clickTimes.pop_front();
    }

    int cps_instant = (int)ac.clickTimes.size();
    bool suspicious = false;

    // 1. Слишком высокий CPS
    if (cps_instant > 15) suspicious = true;

    // 2. Проверка на "идеальный ритм" (боты дают ровные интервалы)
    if (ac.clickTimes.size() >= 5) {
        std::vector<long long> diffs;
        for (size_t i = 1; i < ac.clickTimes.size(); ++i)
            diffs.push_back(ac.clickTimes[i] - ac.clickTimes[i - 1]);

        double mean = 0;
        for (auto d : diffs) mean += (double)d;
        mean /= (double)diffs.size();

        double variance = 0;
        for (auto d : diffs) variance += (d - mean) * (d - mean);
        variance /= (double)diffs.size();
        double stdev = std::sqrt(variance);

        // Ровный ритм < 5 мс — точно бот
        if (stdev < 5.0 && mean < 200.0) suspicious = true;
    }

    if (suspicious) {
        ac.suspicion += 30.0;
        ac.strikes++;
    } else {
        ac.suspicion -= 2.0;
        if (ac.suspicion < 0) ac.suspicion = 0;
    }

    // Наказание
    if (ac.suspicion >= 100.0) {
        ac.punished = true;
        ac.punishedUntilMs = t + 5000;
        ac.blockUntilMs = ac.punishedUntilMs;
        ac.suspicion = 50.0;
        ac.clickTimes.clear();
        shouldRedraw = true;
        return;
    }

    // Если наказание активно — не начисляем
    if (ac.punished && t < ac.punishedUntilMs) {
        shouldRedraw = true;
        return;
    }
    if (ac.punished && t >= ac.punishedUntilMs) ac.punished = false;

    // Начисляем клик с jitter-ом (античит для ботов)
    static std::mt19937 rngC((unsigned)std::time(nullptr));
    std::uniform_int_distribution<int> jitter(-1, 1);
    long long base = (long long)(cpc * maps[currentMapIdx].multiplier);
    long long gain = base + jitter(rngC);
    if (gain < 1) gain = 1;

    clicks = safe_add(clicks, gain);
    check_achievements(clicks, upgrades, achievements);
    shouldRedraw = true;
    dirty = true;
}

// ============================================================
//  MAIN
// ============================================================
int main() {
    platform::setup_console();
    std::srand((unsigned)std::time(nullptr));

    int language = 0;
    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "    SELECT LANGUAGE / УСТАНОВИТЕ ЯЗЫК\n";
    std::cout << "========================================\n";
    std::cout << "  1. English\n";
    std::cout << "  2. Русский\n";
    std::cout << "========================================\n";
    while (language != 1 && language != 2) {
        int ch = platform::getch_impl();
        if (ch == '1') language = 1;
        if (ch == '2') language = 2;
    }
    Strings S = get_strings(language);

    long long clicks = 0;
    int cpc = 1;
    int cps = 0;
    bool promo1 = false, promo2 = false;
    int currentMapIdx = 0;
    long long lastWheelTime = 0;

    std::vector<Upgrade> upgrades = {
        {"🔍", "Net Search Bot",        "Поисковый Бот Сети",        15,     0,  1,   0},
        {"🌐", "Browser Extension",     "Расширение Браузера",       100,    0,  5,   0},
        {"📁", "Cloud Drive",           "Облачное Хранилище",        500,    0,  20,  0},
        {"🤖", "System Auto-Tap",       "Системный Авто-Кликер",     3000,   0,  90,  0},
        {"📺", "Premium Stream Click",  "Премиум Стрим Клик",        10000,  25, 0,   0},
        {"🔬", "Quantum Core PC",       "Квантовый Суперкомпьютер",  50000,  0,  500, 0}
    };

    std::vector<Achievement> achievements = {
        {"First Steps",  "Reach 100 total clicks",                "Первые шаги",     "Набрать 100 кликов",             false},
        {"Click Tycoon", "Reach 500 total clicks",                "Клик-Магнат",     "Набрать 500 кликов",             false},
        {"Quantum Leap", "Purchase at least one Quantum Core PC", "Квантовый скачок","Купить Квантовый Компьютер",     false}
    };

    std::vector<Map> maps = {
        {"🏠 Starting Garage Office", "🏠 Стартовый Офис Гараж",     0,      1.0, true},
        {"🏢 Regional Data Center",   "🏢 Региональный Дата-Центр", 2000,   1.5, false},
        {"🇺🇸 Global Tech Valley",    "🇺🇸 Мировая Техно-Долина",   15000,  2.5, false},
        {"🚀 Net Orbital Station",    "🚀 Орбитальная Станция",     100000, 5.0, false}
    };

    load_game(clicks, cpc, cps, upgrades, achievements, promo1, promo2,
              currentMapIdx, lastWheelTime, maps);

    AntiCheat antiCheat;

    auto lastTick = std::chrono::steady_clock::now();
    auto lastSave = std::chrono::steady_clock::now();
    bool shouldRedraw = true;
    bool dirty = false;

    while (true) {
        auto now = std::chrono::steady_clock::now();

        // Пассивный доход раз в секунду
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTick).count() >= 1) {
            clicks = safe_add(clicks, (long long)(cps * maps[currentMapIdx].multiplier));
            lastTick = now;
            check_achievements(clicks, upgrades, achievements);
            shouldRedraw = true;
            dirty = true;
        }

        // Автосохранение раз в 10 секунд
        if (dirty && std::chrono::duration_cast<std::chrono::seconds>(now - lastSave).count() >= 10) {
            save_game(clicks, cpc, cps, upgrades, achievements, promo1, promo2,
                      currentMapIdx, lastWheelTime);
            lastSave = now;
            dirty = false;
        }

        // Сброс зажатия пробела (отслеживаем физическое состояние клавиши)
        if (antiCheat.spaceHeld) {
            if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) {
                antiCheat.spaceHeld = false;
            }
        }

        if (shouldRedraw) {
            draw_interface(clicks, cpc, cps, upgrades, achievements,
                           maps, currentMapIdx, language, S, antiCheat);
            shouldRedraw = false;
        }

        if (platform::kbhit_impl()) {
            int ch = platform::getch_impl();
            if (ch == 27) break; // ESC

            if (ch == 32) { // SPACE
                handle_click(clicks, cpc, maps, currentMapIdx,
                             upgrades, achievements, antiCheat,
                             shouldRedraw, dirty);
            }
            if (ch == 'p' || ch == 'P') {
                enter_promo(clicks, promo1, promo2, S);
                check_achievements(clicks, upgrades, achievements);
                shouldRedraw = true;
                dirty = true;
            }
            if (ch == 'f' || ch == 'F') {
                spin_wheel(clicks, lastWheelTime, S);
                check_achievements(clicks, upgrades, achievements);
                shouldRedraw = true;
                dirty = true;
            }
            if (ch == 'm' || ch == 'M') {
                manage_maps(clicks, currentMapIdx, maps, S, language);
                shouldRedraw = true;
                dirty = true;
            }
            if (ch >= '1' && ch <= '6') {
                size_t idx = ch - '1';
                if (idx < upgrades.size() && clicks >= upgrades[idx].cost) {
                    clicks -= upgrades[idx].cost;
                    upgrades[idx].count++;
                    cpc += upgrades[idx].cpc;
                    cps += upgrades[idx].cps;
                    upgrades[idx].cost = (long long)(upgrades[idx].cost * 1.5);
                    check_achievements(clicks, upgrades, achievements);
                    dirty == true;
                }
                shouldRedraw = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Финальное сохранение
    save_game(clicks, cpc, cps, upgrades, achievements, promo1, promo2,
              currentMapIdx, lastWheelTime);

    platform::clear_screen();
    std::cout << "========================================\n";
    std::cout << "  " << S.saved_exit << "\n";
    std::cout << "========================================\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}