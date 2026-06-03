// ============================================================
//  SOLO LEVELING QUEST SYSTEM  —  main.cpp
//  Full interactive demo: menu-driven console game loop
// ============================================================

#include "QuestSystem.h"
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>

using namespace SoloLeveling;

// ─── Helpers ───────────────────────────────────────────────

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void pause(int ms = 600) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void waitForEnter() {
    std::cout << "\n  \033[2mPress ENTER to continue...\033[0m";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

int getChoice(int min, int max) {
    int choice;
    while (true) {
        std::cout << "\n  \033[36m> \033[0m";
        if (std::cin >> choice && choice >= min && choice <= max) {
            std::cin.ignore();
            return choice;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  \033[31mInvalid choice. Enter " << min << "-" << max << ".\033[0m\n";
    }
}

// ─── Splash / Title Screen ─────────────────────────────────

void showSplash() {
    clearScreen();
    std::cout << "\033[35m\033[1m\n\n";
    std::cout << "        ███████╗ ██████╗ ██╗      ██████╗     \n";
    std::cout << "        ██╔════╝██╔═══██╗██║     ██╔═══██╗    \n";
    std::cout << "        ███████╗██║   ██║██║     ██║   ██║    \n";
    std::cout << "        ╚════██║██║   ██║██║     ██║   ██║    \n";
    std::cout << "        ███████║╚██████╔╝███████╗╚██████╔╝    \n";
    std::cout << "        ╚══════╝ ╚═════╝ ╚══════╝ ╚═════╝     \n\n";
    std::cout << "        \033[36mL E V E L I N G   S Y S T E M\033[0m\n";
    std::cout << "\033[35m\n";
    std::cout << "        ┌─────────────────────────────────────┐\n";
    std::cout << "        │         QUEST ENGINE  v1.0          │\n";
    std::cout << "        │    A Solo Leveling Game Prototype   │\n";
    std::cout << "        └─────────────────────────────────────┘\n";
    std::cout << "\033[0m\n";
    std::cout << "        \033[2mPress ENTER to wake from the System...\033[0m";
    std::cin.get();
}

// ─── Main Menu ─────────────────────────────────────────────

void showMainMenu() {
    std::cout << "\n\033[34m\033[1m  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                   MAIN MENU                         ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\033[0m\n\n";
    std::cout << "   1. \033[97mView Status (Hunter Profile)\033[0m\n";
    std::cout << "   2. \033[97mQuest Board (all quests)\033[0m\n";
    std::cout << "   3. \033[97mActive Quests\033[0m\n";
    std::cout << "   4. \033[97mAccept a Quest\033[0m\n";
    std::cout << "   5. \033[97mAbandon a Quest\033[0m\n";
    std::cout << "   6. \033[97mSimulate Event (kill monster / reach location)\033[0m\n";
    std::cout << "   7. \033[97mView Quest Detail\033[0m\n";
    std::cout << "   8. \033[97mCheck Timers (tick)\033[0m\n";
    std::cout << "   9. \033[91mExit System\033[0m\n";
}

// ─── Quest ID Picker ───────────────────────────────────────

std::string pickQuestId(QuestManager& qm) {
    std::cout << "\n\033[36m  Enter Quest ID: \033[0m";
    std::string id;
    std::cin >> id;
    std::cin.ignore();
    return id;
}

// ─── Simulate Event Menu ───────────────────────────────────

void simulateEventMenu(QuestManager& qm, Player& player) {
    SystemMessage::sectionHeader("SIMULATE EVENT");
    std::cout << "   1. Kill monsters\n";
    std::cout << "   2. Collect items\n";
    std::cout << "   3. Reach a location\n";
    std::cout << "   4. Back\n";
    int choice = getChoice(1, 4);

    if (choice == 4) return;

    std::string targetId;
    int count = 1;

    if (choice == 1) {
        std::cout << "\n  Available monster targets:\n";
        std::cout << "  pushup, situp, squat, double_boss, gate_guardian\n";
        std::cout << "  penalty_shade, reaper_hunter, goblin_warlord, dungeon_mob, break_boss\n";
        std::cout << "\n  Monster ID: ";
        std::cin >> targetId; std::cin.ignore();
        std::cout << "  Count: ";
        std::cin >> count; std::cin.ignore();
        qm.onMonsterKilled(targetId, count, player);
        std::cout << "\033[32m  [EVENT] Killed " << count << "x " << targetId << "\033[0m\n";
    }
    else if (choice == 2) {
        std::cout << "\n  Available item targets:\n";
        std::cout << "  dungeon_core, monarch_stone, rune_fragment\n";
        std::cout << "  shadow_soldier, territory_flag, hero_medal\n";
        std::cout << "\n  Item ID: ";
        std::cin >> targetId; std::cin.ignore();
        std::cout << "  Count: ";
        std::cin >> count; std::cin.ignore();
        qm.onItemCollected(targetId, count, player);
        std::cout << "\033[32m  [EVENT] Collected " << count << "x " << targetId << "\033[0m\n";
    }
    else if (choice == 3) {
        std::cout << "\n  Available location targets:\n";
        std::cout << "  floor_b5, gate_seal, busan_gate, red_gate, red_gate_exit\n";
        std::cout << "  km_10, guild_hq, shadow_test, arise_altar\n";
        std::cout << "\n  Location ID: ";
        std::cin >> targetId; std::cin.ignore();
        qm.onLocationReached(targetId, player);
        std::cout << "\033[32m  [EVENT] Reached " << targetId << "\033[0m\n";
    }
    waitForEnter();
}

// ─── Full Quest Detail with action ─────────────────────────

void viewQuestDetailMenu(QuestManager& qm) {
    SystemMessage::sectionHeader("SELECT QUEST TO VIEW");
    auto quests = qm.getQuestsByStatus(QuestStatus::AVAILABLE);
    auto active = qm.getQuestsByStatus(QuestStatus::ACTIVE);
    auto completed = qm.getQuestsByStatus(QuestStatus::COMPLETED);

    quests.insert(quests.end(), active.begin(), active.end());
    quests.insert(quests.end(), completed.begin(), completed.end());

    if (quests.empty()) { std::cout << "  No quests to display.\n"; waitForEnter(); return; }

    for (int i = 0; i < (int)quests.size(); ++i)
        std::cout << "  " << (i+1) << ". [" << rankToString(quests[i]->rank) << "] "
                  << quests[i]->name << " — " << questStatusToString(quests[i]->status) << "\n";

    std::cout << "  " << (quests.size()+1) << ". Back\n";
    int c = getChoice(1, (int)quests.size()+1);
    if (c == (int)quests.size()+1) return;

    qm.printQuestDetail(quests[c-1]->id);
    waitForEnter();
}

// ─── Accept Quest Menu ─────────────────────────────────────

void acceptQuestMenu(QuestManager& qm, Player& player) {
    SystemMessage::sectionHeader("AVAILABLE QUESTS");
    auto available = qm.getAvailableQuests(player);

    if (available.empty()) {
        std::cout << "  No quests available at your current level.\n";
        waitForEnter(); return;
    }

    for (int i = 0; i < (int)available.size(); ++i)
        std::cout << "  " << (i+1) << ". ["
                  << rankToString(available[i]->rank) << "] "
                  << available[i]->name
                  << " (Lv." << available[i]->recommendedLevel << ")"
                  << " — " << questStatusToString(available[i]->status) << "\n";

    std::cout << "  " << (available.size()+1) << ". Back\n";
    int c = getChoice(1, (int)available.size()+1);
    if (c == (int)available.size()+1) return;

    qm.acceptQuest(available[c-1]->id, player);
    waitForEnter();
}

// ─── Abandon Quest Menu ────────────────────────────────────

void abandonQuestMenu(QuestManager& qm, Player& player) {
    SystemMessage::sectionHeader("ACTIVE QUESTS");
    auto active = qm.getQuestsByStatus(QuestStatus::ACTIVE);

    if (active.empty()) {
        std::cout << "  No active quests.\n";
        waitForEnter(); return;
    }

    for (int i = 0; i < (int)active.size(); ++i)
        std::cout << "  " << (i+1) << ". ["
                  << rankToString(active[i]->rank) << "] "
                  << active[i]->name << "\n";

    std::cout << "  " << (active.size()+1) << ". Back\n";
    int c = getChoice(1, (int)active.size()+1);
    if (c == (int)active.size()+1) return;

    std::cout << "\n  \033[31mAre you sure? (1=Yes 2=No): \033[0m";
    int confirm = getChoice(1, 2);
    if (confirm == 1)
        qm.abandonQuest(active[c-1]->id, player);

    waitForEnter();
}

// ─── Setup ─────────────────────────────────────────────────

Player createPlayer() {
    Player p;
    p.name  = "Sung Jin-woo";
    p.level = 1;
    p.exp   = 0;
    p.gold  = 500;
    p.hunterRank = Rank::E;
    p.stats = {10, 10, 10, 10, 10, 10};
    return p;
}

void registerAllQuests(QuestManager& qm) {
    qm.registerQuest(QuestFactory::makeDailyTraining());
    qm.registerQuest(QuestFactory::makeDoubleGateDungeon());
    qm.registerQuest(QuestFactory::makePenaltyQuest());
    qm.registerQuest(QuestFactory::makeEmergencyEvacuation());
    qm.registerQuest(QuestFactory::makeGuildWar());
    qm.registerQuest(QuestFactory::makeHiddenMonarchPath());
    qm.registerQuest(QuestFactory::makeShadowExtractionMission());
    qm.registerQuest(QuestFactory::makeKillQuest("goblin", "Forest Goblins", 10, Rank::E));
    qm.registerQuest(QuestFactory::makeKillQuest("orc_warrior", "Orc Warriors", 5, Rank::D));
    qm.registerQuest(QuestFactory::makeItemCollectionQuest("mana_crystal","Mana Crystal", 20, Rank::C));
}

// ─── Main Game Loop ────────────────────────────────────────

int main() {
    showSplash();
    clearScreen();

    Player player = createPlayer();
    QuestManager qm;
    registerAllQuests(qm);

    SystemMessage::show("System initialized. Welcome, Hunter " + player.name + ".");
    pause(800);

    bool running = true;
    while (running) {
        clearScreen();
        std::cout << "\033[34m\033[1m  ── HUNTER: \033[97m" << player.name
                  << "\033[34m  Lv." << player.level
                  << "  [" << rankToString(player.hunterRank) << "-Rank]"
                  << "  EXP:" << player.exp << "/" << player.expToNextLevel()
                  << "  Gold:" << player.gold << "G\033[0m\n";

        // Timer check each loop
        qm.checkTimers(player);

        showMainMenu();
        int choice = getChoice(1, 9);
        clearScreen();

        switch (choice) {
            case 1:
                player.print();
                waitForEnter();
                break;
            case 2:
                qm.printQuestBoard(true);
                waitForEnter();
                break;
            case 3:
                qm.printActiveQuests();
                waitForEnter();
                break;
            case 4:
                acceptQuestMenu(qm, player);
                break;
            case 5:
                abandonQuestMenu(qm, player);
                break;
            case 6:
                simulateEventMenu(qm, player);
                break;
            case 7:
                viewQuestDetailMenu(qm);
                break;
            case 8:
                qm.checkTimers(player);
                std::cout << "  \033[36m[SYSTEM] Timers checked.\033[0m\n";
                waitForEnter();
                break;
            case 9:
                SystemMessage::show("System logging out... Farewell, Hunter.");
                running = false;
                break;
        }
    }

    return 0;
}
