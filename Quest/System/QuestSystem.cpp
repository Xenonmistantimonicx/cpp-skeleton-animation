// ============================================================
//  SOLO LEVELING QUEST SYSTEM  —  QuestSystem.cpp
//  Full implementation of all declared classes
// ============================================================

#include "QuestSystem.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cassert>
#include <climits>

namespace SoloLeveling {

// ─── ANSI Color Codes ──────────────────────────────────────
#define RST  "\033[0m"
#define BOLD "\033[1m"
#define DIM  "\033[2m"
#define BLU  "\033[34m"
#define CYN  "\033[36m"
#define GRN  "\033[32m"
#define YEL  "\033[33m"
#define RED  "\033[31m"
#define MAG  "\033[35m"
#define WHT  "\033[97m"
#define BGRN "\033[92m"
#define BRED "\033[91m"
#define BYEL "\033[93m"
#define BMAG "\033[95m"
#define BCYN "\033[96m"

// ─── Enum-to-String Helpers ───────────────────────────────

std::string rankToString(Rank r) {
    switch (r) {
        case Rank::E: return "E";
        case Rank::D: return "D";
        case Rank::C: return "C";
        case Rank::B: return "B";
        case Rank::A: return "A";
        case Rank::S: return "S";
    }
    return "?";
}

std::string questTypeToString(QuestType t) {
    switch (t) {
        case QuestType::DAILY:     return "DAILY";
        case QuestType::MAIN:      return "MAIN STORY";
        case QuestType::SIDE:      return "SIDE";
        case QuestType::HIDDEN:    return "HIDDEN";
        case QuestType::EMERGENCY: return "EMERGENCY";
        case QuestType::PENALTY:   return "PENALTY";
        case QuestType::GUILD:     return "GUILD";
    }
    return "UNKNOWN";
}

std::string questStatusToString(QuestStatus s) {
    switch (s) {
        case QuestStatus::LOCKED:    return "LOCKED";
        case QuestStatus::AVAILABLE: return "AVAILABLE";
        case QuestStatus::ACTIVE:    return "ACTIVE";
        case QuestStatus::COMPLETED: return "COMPLETED";
        case QuestStatus::FAILED:    return "FAILED";
        case QuestStatus::ABANDONED: return "ABANDONED";
    }
    return "UNKNOWN";
}

std::string statTypeToString(StatType s) {
    switch (s) {
        case StatType::STR: return "Strength";
        case StatType::AGI: return "Agility";
        case StatType::VIT: return "Vitality";
        case StatType::INT: return "Intelligence";
        case StatType::PER: return "Perception";
        case StatType::SEN: return "Sense";
    }
    return "?";
}

// ─── StatBundle ───────────────────────────────────────────

StatBundle StatBundle::operator+(const StatBundle& o) const {
    return {str+o.str, agi+o.agi, vit+o.vit, intel+o.intel, per+o.per, sen+o.sen};
}

StatBundle StatBundle::operator*(float f) const {
    return {(int)(str*f),(int)(agi*f),(int)(vit*f),(int)(intel*f),(int)(per*f),(int)(sen*f)};
}

void StatBundle::clampMin(int minVal) {
    str   = std::max(str,   minVal);
    agi   = std::max(agi,   minVal);
    vit   = std::max(vit,   minVal);
    intel = std::max(intel, minVal);
    per   = std::max(per,   minVal);
    sen   = std::max(sen,   minVal);
}

// ─── Quest helpers ─────────────────────────────────────────

float Quest::overallProgress() const {
    if (objectives.empty()) return 1.0f;
    float sum = 0.0f;
    int mandatory = 0;
    for (const auto& obj : objectives) {
        if (!obj.optional) {
            sum += obj.progress();
            ++mandatory;
        }
    }
    return mandatory > 0 ? sum / mandatory : 1.0f;
}

bool Quest::allObjectivesComplete() const {
    for (const auto& obj : objectives)
        if (!obj.optional && !obj.isComplete())
            return false;
    return true;
}

bool Quest::isExpired() const {
    if (timeLimitSeconds <= 0 || status != QuestStatus::ACTIVE) return false;
    return secondsRemaining() <= 0;
}

int Quest::secondsRemaining() const {
    if (timeLimitSeconds <= 0) return INT_MAX;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    return (int)(timeLimitSeconds - elapsed);
}

std::string Quest::progressBar(int width) const {
    float pct = overallProgress();
    int filled = (int)(pct * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? "█" : "░";
    bar += "] ";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (pct * 100.0f) << "%";
    return bar + oss.str();
}

// ─── Player ────────────────────────────────────────────────

bool Player::tryLevelUp() {
    bool leveled = false;
    while (exp >= expToNextLevel()) {
        exp -= expToNextLevel();
        ++level;
        skillPoints += 3;
        // stat auto-growth per level
        stats.str   += 2;
        stats.agi   += 2;
        stats.vit   += 2;
        stats.intel += 1;
        stats.per   += 1;
        stats.sen   += 1;
        hunterRank = calculateHunterRank();
        leveled = true;
    }
    return leveled;
}

void Player::applyStatBoost(const StatBundle& b) {
    stats = stats + b;
    stats.clampMin(1);
}

void Player::addItem(const Item& item) {
    for (auto& existing : inventory) {
        if (existing.id == item.id) {
            existing.quantity += item.quantity;
            return;
        }
    }
    inventory.push_back(item);
}

bool Player::hasItem(const std::string& itemId, int qty) const {
    for (const auto& item : inventory)
        if (item.id == itemId && item.quantity >= qty)
            return true;
    return false;
}

void Player::addTitle(const std::string& title) {
    if (std::find(titles.begin(), titles.end(), title) == titles.end())
        titles.push_back(title);
}

Rank Player::calculateHunterRank() const {
    if (level >= 100) return Rank::S;
    if (level >= 70)  return Rank::A;
    if (level >= 45)  return Rank::B;
    if (level >= 25)  return Rank::C;
    if (level >= 10)  return Rank::D;
    return Rank::E;
}

void Player::print() const {
    SystemMessage::sectionHeader("HUNTER STATUS");
    std::cout << BOLD << WHT << "  Name   : " << RST << name << "\n";
    std::cout << BOLD << WHT << "  Rank   : " << RST
              << SystemMessage::rankColor(hunterRank)
              << "[" << rankToString(hunterRank) << "-RANK]" << RST << "\n";
    std::cout << BOLD << WHT << "  Level  : " << RST << level << "\n";
    std::cout << BOLD << WHT << "  EXP    : " << RST
              << exp << " / " << expToNextLevel() << "\n";
    std::cout << BOLD << WHT << "  Gold   : " << RST
              << BYEL << gold << RST << " G\n";
    std::cout << BOLD << WHT << "  SP     : " << RST << skillPoints << "\n";
    std::cout << "\n";
    std::cout << BLU << "  ── STATS ──────────────────────────────\n" << RST;
    std::cout << "  STR " << std::setw(6) << stats.str
              << "   AGI " << std::setw(6) << stats.agi
              << "   VIT " << std::setw(6) << stats.vit << "\n";
    std::cout << "  INT " << std::setw(6) << stats.intel
              << "   PER " << std::setw(6) << stats.per
              << "   SEN " << std::setw(6) << stats.sen << "\n";

    if (!titles.empty()) {
        std::cout << "\n" << BLU << "  ── TITLES ─────────────────────────────\n" << RST;
        for (const auto& t : titles)
            std::cout << "  ◆ " << BMAG << t << RST << "\n";
    }
    if (!inventory.empty()) {
        std::cout << "\n" << BLU << "  ── INVENTORY ──────────────────────────\n" << RST;
        for (const auto& item : inventory)
            std::cout << "  [" << rankToString(item.rank) << "] "
                      << item.name << " x" << item.quantity << "\n";
    }
    std::cout << "\n";
}

// ─── QuestManager ──────────────────────────────────────────

void QuestManager::registerQuest(Quest quest) {
    quests_[quest.id] = std::move(quest);
}

void QuestManager::registerQuests(std::vector<Quest> quests) {
    for (auto& q : quests)
        registerQuest(std::move(q));
}

bool QuestManager::prerequisitesMet(const Quest& quest) const {
    for (const auto& preId : quest.prerequisiteQuestIds) {
        auto it = quests_.find(preId);
        if (it == quests_.end() || it->second.status != QuestStatus::COMPLETED)
            return false;
    }
    return true;
}

bool QuestManager::levelRequirementMet(const Quest& quest, const Player& player) const {
    return player.level >= quest.recommendedLevel;
}

bool QuestManager::acceptQuest(const std::string& questId, Player& player) {
    auto it = quests_.find(questId);
    if (it == quests_.end()) {
        std::cout << BRED << "[SYSTEM] Quest not found: " << questId << RST << "\n";
        return false;
    }

    Quest& q = it->second;

    if (q.status == QuestStatus::ACTIVE) {
        std::cout << BRED << "[SYSTEM] Quest already active.\n" << RST;
        return false;
    }
    if (q.status == QuestStatus::COMPLETED && !q.repeatable) {
        std::cout << BRED << "[SYSTEM] Quest already completed.\n" << RST;
        return false;
    }
    if (q.status == QuestStatus::LOCKED) {
        std::cout << BRED << "[SYSTEM] Quest is locked. Complete prerequisites first.\n" << RST;
        return false;
    }
    if (!prerequisitesMet(q)) {
        std::cout << BRED << "[SYSTEM] Prerequisites not met.\n" << RST;
        return false;
    }
    if (!levelRequirementMet(q, player)) {
        std::cout << BRED << "[SYSTEM] Level requirement: " << q.recommendedLevel
                  << ". Your level: " << player.level << RST << "\n";
        return false;
    }

    // Reset objectives if repeating
    if (q.repeatable) {
        for (auto& obj : q.objectives) obj.current = 0;
    }

    q.status = QuestStatus::ACTIVE;
    q.startTime = std::chrono::steady_clock::now();

    SystemMessage::questAccepted(q);

    if (q.onAccept) q.onAccept();
    return true;
}

bool QuestManager::abandonQuest(const std::string& questId, Player& player) {
    auto it = quests_.find(questId);
    if (it == quests_.end()) return false;

    Quest& q = it->second;
    if (q.status != QuestStatus::ACTIVE) return false;

    if (q.type == QuestType::PENALTY) {
        std::cout << BRED << "[SYSTEM] Penalty quests cannot be abandoned.\n" << RST;
        return false;
    }

    q.status = QuestStatus::ABANDONED;
    SystemMessage::show("Quest abandoned: " + q.name);

    if (q.hasPenalty && q.penalty.has_value()) {
        std::cout << BRED << "[SYSTEM] Penalty applied for abandonment!\n" << RST;
        applyPenalty(questId, player);
    }
    return true;
}

bool QuestManager::updateObjective(const std::string& questId,
                                    const std::string& objectiveId,
                                    int delta,
                                    Player& player) {
    auto it = quests_.find(questId);
    if (it == quests_.end()) return false;

    Quest& q = it->second;
    if (q.status != QuestStatus::ACTIVE) return false;

    for (auto& obj : q.objectives) {
        if (obj.id == objectiveId) {
            bool wasDone = obj.isComplete();
            obj.current = std::min(obj.current + delta, obj.required);
            if (!wasDone && obj.isComplete()) {
                SystemMessage::objectiveUpdated(obj);
            }
            // Auto-complete when all done
            if (q.allObjectivesComplete()) {
                completeQuest(questId, player);
            }
            return true;
        }
    }
    return false;
}

bool QuestManager::completeQuest(const std::string& questId, Player& player) {
    auto it = quests_.find(questId);
    if (it == quests_.end()) return false;

    Quest& q = it->second;
    if (q.status != QuestStatus::ACTIVE) return false;
    if (!q.allObjectivesComplete()) {
        std::cout << BRED << "[SYSTEM] Not all objectives completed.\n" << RST;
        return false;
    }

    q.status = QuestStatus::COMPLETED;
    completedIds_.push_back(questId);

    SystemMessage::questCompleted(q);
    grantRewards(q, player);
    unlockDependentQuests(q, player);

    if (q.onComplete) q.onComplete();
    return true;
}

void QuestManager::checkTimers(Player& player) {
    for (auto& [id, q] : quests_) {
        if (q.status == QuestStatus::ACTIVE && q.isExpired()) {
            q.status = QuestStatus::FAILED;
            SystemMessage::questFailed(q);
            if (q.hasPenalty && q.penalty.has_value())
                applyPenalty(id, player);
            if (q.onFail) q.onFail();
        }
    }
}

void QuestManager::grantRewards(const Quest& quest, Player& player) {
    Rank oldRank = player.hunterRank;

    for (const auto& reward : quest.rewards) {
        switch (reward.type) {
            case RewardType::EXP:
                player.exp += reward.amount;
                if (player.tryLevelUp()) {
                    SystemMessage::levelUp(player);
                    if (player.hunterRank != oldRank) {
                        SystemMessage::rankUp(oldRank, player.hunterRank);
                        oldRank = player.hunterRank;
                    }
                }
                break;
            case RewardType::GOLD:
                player.gold += reward.amount;
                break;
            case RewardType::SKILL_POINT:
                player.skillPoints += reward.amount;
                break;
            case RewardType::ITEM:
                if (reward.item.has_value())
                    player.addItem(reward.item.value());
                break;
            case RewardType::STAT_BOOST:
                if (reward.statBoost.has_value())
                    player.applyStatBoost(reward.statBoost.value());
                break;
            case RewardType::TITLE:
                if (!reward.title.empty())
                    player.addTitle(reward.title);
                break;
        }
        SystemMessage::rewardGranted(reward);
    }
}

void QuestManager::unlockDependentQuests(const Quest& quest, Player& /*player*/) {
    for (const auto& unlockId : quest.unlocksQuestIds) {
        auto it = quests_.find(unlockId);
        if (it != quests_.end() && it->second.status == QuestStatus::LOCKED) {
            if (prerequisitesMet(it->second)) {
                it->second.status = QuestStatus::AVAILABLE;
                std::cout << BCYN << "[SYSTEM] New quest unlocked: "
                          << BOLD << it->second.name << RST << "\n";
            }
        }
    }
}

void QuestManager::applyPenalty(const std::string& questId, Player& player) {
    auto it = quests_.find(questId);
    if (it == quests_.end() || !it->second.penalty.has_value()) return;

    const PenaltyEntry& pen = it->second.penalty.value();
    SystemMessage::penaltyApplied(pen);

    switch (pen.type) {
        case PenaltyType::STAT_REDUCTION:
            player.stats = player.stats + (pen.statReduction * -1.0f);
            player.stats.clampMin(1);
            break;
        case PenaltyType::LEVEL_DRAIN:
            player.level = std::max(1, player.level - pen.levelDrain);
            player.hunterRank = player.calculateHunterRank();
            break;
        case PenaltyType::IMPRISONMENT:
            std::cout << BRED << "[SYSTEM] You have been imprisoned in the dungeon!\n" << RST;
            break;
        case PenaltyType::INSTANT_DEATH:
            std::cout << BRED << BOLD << "[SYSTEM] FATAL PENALTY — GAME OVER\n" << RST;
            break;
    }
}

std::optional<Quest*> QuestManager::getQuest(const std::string& questId) {
    auto it = quests_.find(questId);
    if (it == quests_.end()) return std::nullopt;
    return &it->second;
}

std::vector<Quest*> QuestManager::getQuestsByStatus(QuestStatus status) {
    std::vector<Quest*> result;
    for (auto& [id, q] : quests_)
        if (q.status == status)
            result.push_back(&q);
    return result;
}

std::vector<Quest*> QuestManager::getQuestsByType(QuestType type) {
    std::vector<Quest*> result;
    for (auto& [id, q] : quests_)
        if (q.type == type)
            result.push_back(&q);
    return result;
}

std::vector<Quest*> QuestManager::getQuestsByRank(Rank rank) {
    std::vector<Quest*> result;
    for (auto& [id, q] : quests_)
        if (q.rank == rank)
            result.push_back(&q);
    return result;
}

std::vector<Quest*> QuestManager::getAvailableQuests(const Player& player) {
    std::vector<Quest*> result;
    for (auto& [id, q] : quests_) {
        bool available = (q.status == QuestStatus::AVAILABLE || q.status == QuestStatus::ACTIVE);
        bool prereqOk  = prerequisitesMet(q);
        bool levelOk   = levelRequirementMet(q, player);
        if (available && prereqOk && levelOk)
            result.push_back(&q);
    }
    // Sort by rank descending
    std::sort(result.begin(), result.end(), [](const Quest* a, const Quest* b){
        return (int)a->rank > (int)b->rank;
    });
    return result;
}

bool QuestManager::isQuestComplete(const std::string& questId) const {
    auto it = quests_.find(questId);
    return it != quests_.end() && it->second.status == QuestStatus::COMPLETED;
}

// ── Event Handlers ─────────────────────────────────────────

void QuestManager::onMonsterKilled(const std::string& monsterId,
                                    int count,
                                    Player& player) {
    for (auto& [qid, q] : quests_) {
        if (q.status != QuestStatus::ACTIVE) continue;
        for (auto& obj : q.objectives) {
            if (obj.type == ObjectiveType::KILL && obj.targetId == monsterId)
                updateObjective(qid, obj.id, count, player);
        }
    }
}

void QuestManager::onItemCollected(const std::string& itemId,
                                    int count,
                                    Player& player) {
    for (auto& [qid, q] : quests_) {
        if (q.status != QuestStatus::ACTIVE) continue;
        for (auto& obj : q.objectives) {
            if (obj.type == ObjectiveType::COLLECT && obj.targetId == itemId)
                updateObjective(qid, obj.id, count, player);
        }
    }
}

void QuestManager::onLocationReached(const std::string& locationId, Player& player) {
    for (auto& [qid, q] : quests_) {
        if (q.status != QuestStatus::ACTIVE) continue;
        for (auto& obj : q.objectives) {
            if (obj.type == ObjectiveType::REACH && obj.targetId == locationId)
                updateObjective(qid, obj.id, 1, player);
        }
    }
}

// ── Display ────────────────────────────────────────────────

void QuestManager::printQuestBoard(bool verbose) const {
    SystemMessage::sectionHeader("QUEST BOARD");

    auto statusColor = [](QuestStatus s) -> const char* {
        switch (s) {
            case QuestStatus::ACTIVE:    return BGRN;
            case QuestStatus::AVAILABLE: return BCYN;
            case QuestStatus::LOCKED:    return DIM;
            case QuestStatus::COMPLETED: return BYEL;
            case QuestStatus::FAILED:    return BRED;
            case QuestStatus::ABANDONED: return RED;
        }
        return RST;
    };

    for (const auto& [id, q] : quests_) {
        if (q.isHidden && q.status == QuestStatus::LOCKED) continue;

        std::cout << "  "
                  << SystemMessage::rankColor(q.rank)
                  << "[" << rankToString(q.rank) << "]" << RST << " "
                  << BOLD << std::left << std::setw(36) << q.name << RST << " "
                  << statusColor(q.status)
                  << std::setw(10) << questStatusToString(q.status) << RST;

        if (q.status == QuestStatus::ACTIVE)
            std::cout << "  " << q.progressBar(15);

        std::cout << "\n";

        if (verbose && q.status == QuestStatus::ACTIVE) {
            for (const auto& obj : q.objectives) {
                std::cout << "       " << (obj.isComplete() ? BGRN "✓" : YEL "○") << RST
                          << " " << obj.description
                          << " [" << obj.current << "/" << obj.required << "]\n";
            }
        }
    }
    std::cout << "\n";
}

void QuestManager::printQuestDetail(const std::string& questId) const {
    auto it = quests_.find(questId);
    if (it == quests_.end()) { std::cout << "Quest not found.\n"; return; }

    const Quest& q = it->second;
    SystemMessage::sectionHeader(q.name);

    std::cout << "  " << BLU << "Rank  : " << RST
              << SystemMessage::rankColor(q.rank) << "[" << rankToString(q.rank) << "-RANK]" << RST << "\n";
    std::cout << "  " << BLU << "Type  : " << RST << questTypeToString(q.type) << "\n";
    std::cout << "  " << BLU << "Status: " << RST << questStatusToString(q.status) << "\n";

    if (q.timeLimitSeconds > 0 && q.status == QuestStatus::ACTIVE) {
        int secs = q.secondsRemaining();
        int h = secs/3600, m = (secs%3600)/60, s = secs%60;
        std::cout << "  " << BLU << "Time  : " << RST
                  << (secs < 300 ? BRED : BYEL)
                  << h << "h " << m << "m " << s << "s remaining" << RST << "\n";
    }

    std::cout << "\n  " << DIM << q.description << RST << "\n\n";

    // Objectives
    std::cout << BLU << "  ── OBJECTIVES ─────────────────────────────────────────\n" << RST;
    for (const auto& obj : q.objectives) {
        const char* check = obj.isComplete() ? (BGRN "✓") : (YEL "○");
        std::cout << "    " << check << RST << " " << obj.description;
        if (obj.required > 1)
            std::cout << " [" << obj.current << " / " << obj.required << "]";
        if (obj.optional)
            std::cout << " " << DIM << "(optional)" << RST;
        std::cout << "\n";
    }

    if (q.status == QuestStatus::ACTIVE)
        std::cout << "\n  Progress: " << q.progressBar(25) << "\n";

    // Rewards
    std::cout << "\n" << BLU << "  ── REWARDS ────────────────────────────────────────────\n" << RST;
    for (const auto& r : q.rewards) {
        switch (r.type) {
            case RewardType::EXP:
                std::cout << "    " << BCYN << "EXP       +" << r.amount << RST << "\n"; break;
            case RewardType::GOLD:
                std::cout << "    " << BYEL << "Gold      +" << r.amount << " G" << RST << "\n"; break;
            case RewardType::SKILL_POINT:
                std::cout << "    " << BGRN << "Skill Pts +" << r.amount << RST << "\n"; break;
            case RewardType::ITEM:
                if (r.item) std::cout << "    " << MAG << "Item      " << r.item->name << RST << "\n"; break;
            case RewardType::STAT_BOOST:
                if (r.statBoost) {
                    const auto& b = r.statBoost.value();
                    std::cout << "    " << BMAG << "Stats     STR+" << b.str
                              << " AGI+" << b.agi << " VIT+" << b.vit
                              << " INT+" << b.intel << RST << "\n";
                }
                break;
            case RewardType::TITLE:
                std::cout << "    " << BMAG << "Title     [" << r.title << "]" << RST << "\n"; break;
        }
    }

    // Penalty
    if (q.hasPenalty && q.penalty.has_value()) {
        std::cout << "\n" << BRED << "  ── FAILURE PENALTY ──────────────────────────────────\n" << RST;
        std::cout << "    " << BRED << q.penalty->description << RST << "\n";
    }
    std::cout << "\n";
}

void QuestManager::printActiveQuests() const {
    SystemMessage::sectionHeader("ACTIVE QUESTS");
    auto active = const_cast<QuestManager*>(this)->getQuestsByStatus(QuestStatus::ACTIVE);
    if (active.empty()) {
        std::cout << "  No active quests.\n\n";
        return;
    }
    for (const Quest* q : active) {
        std::cout << "  " << SystemMessage::rankColor(q->rank)
                  << "[" << rankToString(q->rank) << "]" << RST
                  << " " << BOLD << q->name << RST << "\n";
        std::cout << "     " << q->progressBar(20) << "\n";
        for (const auto& obj : q->objectives) {
            if (!obj.isComplete())
                std::cout << "     " << YEL << "→ " << RST
                          << obj.description << " ["
                          << obj.current << "/" << obj.required << "]\n";
        }
        std::cout << "\n";
    }
}

// ─── SystemMessage ─────────────────────────────────────────

std::string SystemMessage::rankColor(Rank r) {
    switch (r) {
        case Rank::S: return BMAG;
        case Rank::A: return BRED;
        case Rank::B: return BLU;
        case Rank::C: return BGRN;
        case Rank::D: return BYEL;
        case Rank::E: return DIM;
    }
    return RST;
}

std::string SystemMessage::padRight(const std::string& s, int width) {
    if ((int)s.size() >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

std::string SystemMessage::padCenter(const std::string& s, int width) {
    int pad = width - (int)s.size();
    if (pad <= 0) return s;
    int left = pad / 2, right = pad - left;
    return std::string(left, ' ') + s + std::string(right, ' ');
}

void SystemMessage::separator() {
    std::cout << BLU << "  ─────────────────────────────────────────────────────────\n" << RST;
}

void SystemMessage::sectionHeader(const std::string& title) {
    std::cout << "\n" << BLU << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║" << WHT << padCenter(title, 54) << BLU << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST;
}

void SystemMessage::show(const std::string& message) {
    std::cout << BCYN << "\n  ┌─ SYSTEM ─────────────────────────────────────────────\n";
    std::cout << "  │  " << WHT << message << BCYN << "\n";
    std::cout << "  └──────────────────────────────────────────────────────\n" << RST;
}

void SystemMessage::questAccepted(const Quest& q) {
    std::cout << "\n" << BCYN << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║          ★  QUEST ACCEPTED  ★                        ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    std::cout << "  ║  " << RST << rankColor(q.rank) << "[" << rankToString(q.rank) << "] "
              << WHT << padRight(q.name, 48) << BCYN << "║\n";
    std::cout << "  ║  " << RST << DIM << padRight(questTypeToString(q.type), 52) << BCYN << "║\n";
    if (q.timeLimitSeconds > 0) {
        int h = q.timeLimitSeconds/3600, m = (q.timeLimitSeconds%3600)/60;
        std::string tl = "Time limit: " + std::to_string(h) + "h " + std::to_string(m) + "m";
        std::cout << "  ║  " << RST << BYEL << padRight(tl, 52) << BCYN << "║\n";
    }
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::questCompleted(const Quest& q) {
    std::cout << "\n" << BYEL << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║       ✦  QUEST COMPLETE  ✦                           ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    std::cout << "  ║  " << RST << rankColor(q.rank) << "[" << rankToString(q.rank) << "] "
              << WHT << padRight(q.name, 48) << BYEL << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::questFailed(const Quest& q) {
    std::cout << "\n" << BRED << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║       ✖  QUEST FAILED  ✖                             ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    std::cout << "  ║  " << RST << rankColor(q.rank) << "[" << rankToString(q.rank) << "] "
              << WHT << padRight(q.name, 48) << BRED << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::levelUp(const Player& p) {
    std::cout << "\n" << BYEL << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║              ▲  LEVEL UP!  ▲                         ║\n";
    std::cout << "  ║" << RST << WHT << padCenter("Level " + std::to_string(p.level), 54) << BYEL << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::rankUp(Rank oldRank, Rank newRank) {
    std::cout << "\n" << BMAG << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║           ★★  RANK UP  ★★                            ║\n";
    std::cout << "  ║" << RST << WHT
              << padCenter(rankToString(oldRank) + "-Rank  →  " + rankToString(newRank) + "-Rank", 54)
              << BMAG << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::rewardGranted(const RewardEntry& r) {
    switch (r.type) {
        case RewardType::EXP:
            std::cout << "  " << BCYN << "[ EXP ] +" << r.amount << RST << "\n"; break;
        case RewardType::GOLD:
            std::cout << "  " << BYEL << "[ GOLD ] +" << r.amount << " G" << RST << "\n"; break;
        case RewardType::SKILL_POINT:
            std::cout << "  " << BGRN << "[ SP ] +" << r.amount << " Skill Points" << RST << "\n"; break;
        case RewardType::ITEM:
            if (r.item) std::cout << "  " << MAG << "[ ITEM ] " << r.item->name << RST << "\n"; break;
        case RewardType::STAT_BOOST:
            std::cout << "  " << BMAG << "[ STAT BOOST ] Applied" << RST << "\n"; break;
        case RewardType::TITLE:
            std::cout << "  " << BMAG << "[ TITLE ] Acquired: [" << r.title << "]" << RST << "\n"; break;
    }
}

void SystemMessage::penaltyApplied(const PenaltyEntry& p) {
    std::cout << "\n" << BRED << BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║          ⚠  PENALTY APPLIED  ⚠                       ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    std::cout << "  ║  " << RST << WHT << padRight(p.description, 52) << BRED << "║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n" << RST << "\n";
}

void SystemMessage::objectiveUpdated(const Objective& obj) {
    std::cout << "  " << BGRN << "✓ OBJECTIVE COMPLETE: " << RST << obj.description << "\n";
}

// ─── QuestFactory ──────────────────────────────────────────

Quest QuestFactory::makeDailyTraining() {
    Quest q;
    q.id                = "daily_training_001";
    q.name              = "Daily Training";
    q.description       = "The System demands daily physical conditioning. Complete all exercises before midnight or face the penalty.";
    q.rank              = Rank::E;
    q.type              = QuestType::DAILY;
    q.status            = QuestStatus::AVAILABLE;
    q.repeatable        = true;
    q.hasPenalty        = true;
    q.timeLimitSeconds  = 86400;
    q.recommendedLevel  = 1;

    q.objectives = {
        {"obj_pushups",  "Complete 100 push-ups",  ObjectiveType::KILL,   "pushup",  100, 0},
        {"obj_situps",   "Complete 100 sit-ups",   ObjectiveType::KILL,   "situp",   100, 0},
        {"obj_run",      "Run 10 kilometers",       ObjectiveType::REACH,  "km_10",     1, 0},
        {"obj_squats",   "Complete 100 squats",     ObjectiveType::KILL,   "squat",   100, 0},
    };

    q.rewards = {
        {RewardType::EXP,        10000},
        {RewardType::GOLD,        2000},
        {RewardType::SKILL_POINT, 1},
        {RewardType::STAT_BOOST, 0, std::nullopt, StatBundle{2,2,2,0,0,0}},
    };

    q.penalty = PenaltyEntry{
        PenaltyType::IMPRISONMENT,
        "Failure to complete daily training results in imprisonment in the Penalty Zone.",
        StatBundle{5,5,5,0,0,0}, 0
    };
    return q;
}

Quest QuestFactory::makeDoubleGateDungeon() {
    Quest q;
    q.id               = "main_double_gate_001";
    q.name             = "Conquer the Double Gate Dungeon";
    q.description      = "A Double Gate has appeared in Seoul. Two S-rank dungeons merged into one. Clear the dungeon before a gate break triggers a catastrophe.";
    q.rank             = Rank::S;
    q.type             = QuestType::MAIN;
    q.status           = QuestStatus::AVAILABLE;
    q.hasPenalty       = true;
    q.timeLimitSeconds = 10800;
    q.recommendedLevel = 80;

    q.objectives = {
        {"obj_reach_b5",    "Reach Floor B5",                ObjectiveType::REACH, "floor_b5",     1,  0},
        {"obj_kill_boss",   "Defeat the Double Gate Boss",   ObjectiveType::KILL,  "double_boss",  1,  0},
        {"obj_kill_elites", "Kill Elite Gate Guardians",     ObjectiveType::KILL,  "gate_guardian", 20, 0},
        {"obj_collect_core","Collect the Dungeon Core",      ObjectiveType::COLLECT,"dungeon_core", 1,  0},
        {"obj_seal_gate",   "Seal the Double Gate",          ObjectiveType::REACH, "gate_seal",    1,  0, true},
    };

    q.rewards = {
        {RewardType::EXP,  200000},
        {RewardType::GOLD,  80000},
        {RewardType::SKILL_POINT, 5},
        {RewardType::ITEM,  0, Item{"double_gate_core","Double Gate Core","Crystallized dungeon energy.", Rank::S, 1}},
        {RewardType::TITLE, 0, std::nullopt, std::nullopt, "Dungeon Conqueror"},
    };

    q.penalty = PenaltyEntry{
        PenaltyType::INSTANT_DEATH,
        "Failure results in being consumed by the gate overflow. Instant death.",
        {}, 0
    };

    q.unlocksQuestIds = {"hidden_monarch_path_001"};
    return q;
}

Quest QuestFactory::makePenaltyQuest() {
    Quest q;
    q.id               = "penalty_zone_001";
    q.name             = "Penalty Zone: Survive";
    q.description      = "You have failed a quest with an active penalty. You have been transported to the Penalty Zone. Survive or perish.";
    q.rank             = Rank::C;
    q.type             = QuestType::PENALTY;
    q.status           = QuestStatus::AVAILABLE;
    q.hasPenalty       = true;
    q.timeLimitSeconds = 3600;
    q.recommendedLevel = 1;

    q.objectives = {
        {"obj_survive",     "Survive 60 minutes",           ObjectiveType::SURVIVE, "penalty_zone", 1, 0},
        {"obj_kill_shades", "Kill Penalty Zone Shades",     ObjectiveType::KILL,    "penalty_shade",50,0},
    };

    q.rewards = {
        {RewardType::EXP, 5000},
    };

    q.penalty = PenaltyEntry{
        PenaltyType::INSTANT_DEATH,
        "Failing the Penalty Quest results in permanent death.",
        {}, 0
    };
    return q;
}

Quest QuestFactory::makeEmergencyEvacuation() {
    Quest q;
    q.id               = "emergency_evac_001";
    q.name             = "Emergency: Dungeon Break — Busan";
    q.description      = "A dungeon break has occurred in Busan. Civilians are in danger. Eliminate all escaped monsters and protect as many lives as possible.";
    q.rank             = Rank::B;
    q.type             = QuestType::EMERGENCY;
    q.status           = QuestStatus::AVAILABLE;
    q.timeLimitSeconds = 1800;
    q.recommendedLevel = 30;

    q.objectives = {
        {"obj_arrive",   "Arrive at Busan",               ObjectiveType::REACH,   "busan_gate",    1,  0},
        {"obj_protect",  "Protect civilians (save 50+)",   ObjectiveType::PROTECT, "civilian",     50,  0},
        {"obj_monsters", "Eliminate escaped monsters",     ObjectiveType::KILL,    "dungeon_mob",  80,  0},
        {"obj_boss",     "Defeat the Dungeon Break Boss",  ObjectiveType::KILL,    "break_boss",    1,  0},
    };

    q.rewards = {
        {RewardType::EXP,  60000},
        {RewardType::GOLD, 25000},
        {RewardType::ITEM,  0, Item{"hero_medal","Busan Hero Medal","Awarded for protecting civilians.", Rank::B, 1}},
        {RewardType::TITLE, 0, std::nullopt, std::nullopt, "Defender of Busan"},
    };
    return q;
}

Quest QuestFactory::makeGuildWar() {
    Quest q;
    q.id               = "guild_war_001";
    q.name             = "Guild War: Ahjin vs. Reaper";
    q.description      = "An official Guild War has been declared. Defend Ahjin Guild territories and defeat rival hunters.";
    q.rank             = Rank::A;
    q.type             = QuestType::GUILD;
    q.status           = QuestStatus::LOCKED;
    q.timeLimitSeconds = 7200;
    q.recommendedLevel = 50;

    q.prerequisiteQuestIds = {"main_double_gate_001"};

    q.objectives = {
        {"obj_defend",   "Defend Ahjin Guild HQ",        ObjectiveType::SURVIVE, "guild_hq",     1, 0},
        {"obj_pvp",      "Defeat Reaper Guild hunters",  ObjectiveType::KILL,    "reaper_hunter",30, 0},
        {"obj_flags",    "Capture territory flags",      ObjectiveType::COLLECT, "territory_flag",3, 0},
    };

    q.rewards = {
        {RewardType::EXP,  90000},
        {RewardType::GOLD, 50000},
        {RewardType::SKILL_POINT, 3},
        {RewardType::TITLE, 0, std::nullopt, std::nullopt, "Guild Champion"},
    };
    return q;
}

Quest QuestFactory::makeHiddenMonarchPath() {
    Quest q;
    q.id               = "hidden_monarch_path_001";
    q.name             = "★ Path of the Shadow Monarch";
    q.description      = "Hidden quest. A message left by the Rulers themselves. Complete this to claim the true power of the Shadow Monarch.";
    q.rank             = Rank::S;
    q.type             = QuestType::HIDDEN;
    q.status           = QuestStatus::LOCKED;
    q.isHidden         = true;
    q.recommendedLevel = 100;

    q.prerequisiteQuestIds = {"main_double_gate_001"};

    q.objectives = {
        {"obj_stones",   "Collect all 7 Monarch Stones",  ObjectiveType::COLLECT, "monarch_stone", 7, 0},
        {"obj_shadow",   "Pass the Shadow Test",          ObjectiveType::SURVIVE, "shadow_test",   1, 0},
        {"obj_arise",    "Claim the Arise ability",       ObjectiveType::INTERACT,"arise_altar",   1, 0},
    };

    q.rewards = {
        {RewardType::EXP, 999999},
        {RewardType::STAT_BOOST, 0, std::nullopt, StatBundle{50,50,50,50,50,50}},
        {RewardType::ITEM, 0, Item{"shadow_authority","Shadow Monarch's Authority","The power to rule all shadows.", Rank::S, 1}},
        {RewardType::TITLE, 0, std::nullopt, std::nullopt, "The Shadow Monarch"},
    };
    return q;
}

Quest QuestFactory::makeShadowExtractionMission() {
    Quest q;
    q.id               = "side_shadow_extraction_001";
    q.name             = "Shadow Extraction: Battlefield Remnants";
    q.description      = "Remnants of a fallen raid party near the Red Gate. Extract shadow soldiers from the cursed battlefield.";
    q.rank             = Rank::A;
    q.type             = QuestType::SIDE;
    q.status           = QuestStatus::AVAILABLE;
    q.timeLimitSeconds = 14400;
    q.recommendedLevel = 60;

    q.objectives = {
        {"obj_reach_red",  "Reach the Red Gate battlefield",  ObjectiveType::REACH,   "red_gate",      1, 0},
        {"obj_warlords",   "Defeat Elite Goblin Warlords",    ObjectiveType::KILL,    "goblin_warlord",20, 0},
        {"obj_extract",    "Extract Shadow Soldiers",         ObjectiveType::COLLECT, "shadow_soldier",10, 0},
        {"obj_escape",     "Escape before gate closes",       ObjectiveType::REACH,   "red_gate_exit", 1, 0},
    };

    q.rewards = {
        {RewardType::EXP,  80000},
        {RewardType::GOLD, 40000},
        {RewardType::ITEM, 0, Item{"rune_fragment","Rune Fragment","Fragment of an ancient mana rune.", Rank::A, 3}},
    };
    return q;
}

Quest QuestFactory::makeItemCollectionQuest(const std::string& itemId,
                                              const std::string& itemName,
                                              int quantity,
                                              Rank rank) {
    Quest q;
    q.id          = "collect_" + itemId + "_001";
    q.name        = "Collect: " + itemName;
    q.description = "Collect " + std::to_string(quantity) + "x " + itemName + " from the dungeon.";
    q.rank        = rank;
    q.type        = QuestType::SIDE;
    q.status      = QuestStatus::AVAILABLE;

    q.objectives = {
        {"obj_collect", "Collect " + itemName, ObjectiveType::COLLECT, itemId, quantity, 0},
    };

    int expReward  = quantity * (int)std::pow(10.0, (int)rank + 2);
    int goldReward = quantity * (int)std::pow(10.0, (int)rank + 1);

    q.rewards = {
        {RewardType::EXP,  expReward},
        {RewardType::GOLD, goldReward},
    };
    return q;
}

Quest QuestFactory::makeKillQuest(const std::string& monsterId,
                                   const std::string& monsterName,
                                   int quantity,
                                   Rank rank) {
    Quest q;
    q.id          = "kill_" + monsterId + "_001";
    q.name        = "Hunt: " + monsterName;
    q.description = "Eliminate " + std::to_string(quantity) + "x " + monsterName + ".";
    q.rank        = rank;
    q.type        = QuestType::SIDE;
    q.status      = QuestStatus::AVAILABLE;

    q.objectives = {
        {"obj_kill", "Kill " + monsterName, ObjectiveType::KILL, monsterId, quantity, 0},
    };

    int expReward  = quantity * (int)std::pow(10.0, (int)rank + 3);
    int goldReward = quantity * (int)std::pow(10.0, (int)rank + 2);

    q.rewards = {
        {RewardType::EXP,  expReward},
        {RewardType::GOLD, goldReward},
    };
    return q;
}

} // namespace SoloLeveling
