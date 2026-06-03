#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>
#include <variant>

// ============================================================
//  SOLO LEVELING QUEST SYSTEM  —  QuestSystem.h
//  Enums, data structures, and class declarations
// ============================================================

namespace SoloLeveling {

// ─── Enumerations ────────────────────────────────────────────

enum class Rank { E = 0, D, C, B, A, S };
enum class QuestType { DAILY, MAIN, SIDE, HIDDEN, EMERGENCY, PENALTY, GUILD };
enum class QuestStatus { LOCKED, AVAILABLE, ACTIVE, COMPLETED, FAILED, ABANDONED };
enum class ObjectiveType { KILL, COLLECT, REACH, SURVIVE, PROTECT, CRAFT, INTERACT };
enum class RewardType { EXP, GOLD, ITEM, SKILL_POINT, STAT_BOOST, TITLE };
enum class StatType { STR, AGI, VIT, INT, PER, SEN };
enum class PenaltyType { STAT_REDUCTION, LEVEL_DRAIN, IMPRISONMENT, INSTANT_DEATH };

// Utility: convert enums to strings
std::string rankToString(Rank r);
std::string questTypeToString(QuestType t);
std::string questStatusToString(QuestStatus s);
std::string statTypeToString(StatType s);

// ─── Core Data Structures ─────────────────────────────────────

struct StatBundle {
    int str = 0, agi = 0, vit = 0, intel = 0, per = 0, sen = 0;
    StatBundle operator+(const StatBundle& o) const;
    StatBundle operator*(float f) const;
    void clampMin(int minVal = 1);
};

struct Item {
    std::string id;
    std::string name;
    std::string description;
    Rank rank;
    int quantity = 1;
};

struct RewardEntry {
    RewardType type;
    int amount = 0;
    std::optional<Item> item;
    std::optional<StatBundle> statBoost;
    std::string title;
};

struct PenaltyEntry {
    PenaltyType type;
    std::string description;
    StatBundle statReduction;
    int levelDrain = 0;
};

// ─── Objective ───────────────────────────────────────────────

struct Objective {
    std::string id;
    std::string description;
    ObjectiveType type;
    std::string targetId;     // monster ID, item ID, location ID, etc.
    int required = 1;
    int current  = 0;
    bool optional = false;
    bool isComplete() const { return current >= required; }
    float progress() const  { return required > 0 ? (float)current / required : 1.0f; }
};

// ─── Quest Definition ─────────────────────────────────────────

struct Quest {
    std::string id;
    std::string name;
    std::string description;
    Rank rank                = Rank::E;
    QuestType type           = QuestType::SIDE;
    QuestStatus status       = QuestStatus::AVAILABLE;
    bool repeatable          = false;
    bool hasPenalty          = false;
    bool isHidden            = false;           // visible only when triggered
    int  recommendedLevel    = 1;

    std::vector<Objective>    objectives;
    std::vector<RewardEntry>  rewards;
    std::optional<PenaltyEntry> penalty;

    std::vector<std::string>  prerequisiteQuestIds;
    std::vector<std::string>  unlocksQuestIds;

    // Timer (seconds; 0 = no limit)
    int timeLimitSeconds = 0;
    std::chrono::steady_clock::time_point startTime;

    // Callbacks (set by game engine)
    std::function<void()> onAccept;
    std::function<void()> onComplete;
    std::function<void()> onFail;

    // ── Computed helpers ──
    float overallProgress() const;
    bool allObjectivesComplete() const;
    bool isExpired() const;
    int secondsRemaining() const;
    std::string progressBar(int width = 20) const;
};

// ─── Player Stats ─────────────────────────────────────────────

struct Player {
    std::string name;
    int level           = 1;
    long long exp       = 0;
    long long gold      = 0;
    int skillPoints     = 0;
    Rank hunterRank     = Rank::E;
    StatBundle stats;
    std::vector<std::string> titles;
    std::vector<Item> inventory;
    std::string activeTitle;

    // XP needed for next level (simple quadratic curve)
    long long expToNextLevel() const { return (long long)level * level * 1000LL; }
    bool tryLevelUp();
    void applyStatBoost(const StatBundle& b);
    void addItem(const Item& item);
    bool hasItem(const std::string& itemId, int qty = 1) const;
    void addTitle(const std::string& title);
    Rank calculateHunterRank() const;   // based on level
    void print() const;
};

// ─── Quest Manager ────────────────────────────────────────────

class QuestManager {
public:
    // ── Quest Registration ──
    void registerQuest(Quest quest);
    void registerQuests(std::vector<Quest> quests);

    // ── Lifecycle ──
    bool acceptQuest(const std::string& questId, Player& player);
    bool abandonQuest(const std::string& questId, Player& player);
    bool updateObjective(const std::string& questId,
                         const std::string& objectiveId,
                         int delta,
                         Player& player);
    bool completeQuest(const std::string& questId, Player& player);
    void checkTimers(Player& player);    // call each game tick

    // ── Queries ──
    std::optional<Quest*> getQuest(const std::string& questId);
    std::vector<Quest*> getQuestsByStatus(QuestStatus status);
    std::vector<Quest*> getQuestsByType(QuestType type);
    std::vector<Quest*> getQuestsByRank(Rank rank);
    std::vector<Quest*> getAvailableQuests(const Player& player);
    bool isQuestComplete(const std::string& questId) const;

    // ── System Events (call from game engine) ──
    void onMonsterKilled(const std::string& monsterId, int count, Player& player);
    void onItemCollected(const std::string& itemId, int count, Player& player);
    void onLocationReached(const std::string& locationId, Player& player);

    // ── Display ──
    void printQuestBoard(bool verbose = false) const;
    void printQuestDetail(const std::string& questId) const;
    void printActiveQuests() const;

    // ── Penalty System ──
    void applyPenalty(const std::string& questId, Player& player);

    int totalQuests() const { return (int)quests_.size(); }

private:
    std::unordered_map<std::string, Quest> quests_;
    std::vector<std::string> completedIds_;

    void grantRewards(const Quest& quest, Player& player);
    void unlockDependentQuests(const Quest& quest, Player& player);
    bool prerequisitesMet(const Quest& quest) const;
    bool levelRequirementMet(const Quest& quest, const Player& player) const;
};

// ─── System Message (Solo Leveling style) ─────────────────────

class SystemMessage {
public:
    static void show(const std::string& message);
    static void questAccepted(const Quest& q);
    static void questCompleted(const Quest& q);
    static void questFailed(const Quest& q);
    static void levelUp(const Player& p);
    static void rankUp(Rank oldRank, Rank newRank);
    static void rewardGranted(const RewardEntry& r);
    static void penaltyApplied(const PenaltyEntry& p);
    static void objectiveUpdated(const Objective& obj);
    static void separator();
    static void sectionHeader(const std::string& title);

public:
    static std::string rankColor(Rank r);   // ANSI color codes
private:
    static std::string padRight(const std::string& s, int width);
    static std::string padCenter(const std::string& s, int width);
    static const int PANEL_WIDTH = 60;
};

// ─── Quest Factory (pre-built quest templates) ────────────────

class QuestFactory {
public:
    static Quest makeDailyTraining();
    static Quest makeDoubleGateDungeon();
    static Quest makePenaltyQuest();
    static Quest makeEmergencyEvacuation();
    static Quest makeGuildWar();
    static Quest makeHiddenMonarchPath();
    static Quest makeShadowExtractionMission();
    static Quest makeItemCollectionQuest(const std::string& itemId,
                                         const std::string& itemName,
                                         int quantity,
                                         Rank rank);
    static Quest makeKillQuest(const std::string& monsterId,
                                const std::string& monsterName,
                                int quantity,
                                Rank rank);
};

} // namespace SoloLeveling
