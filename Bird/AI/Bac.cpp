/**
 * ============================================================================================
 *  ENTERPRISE AAA AI ENGINE: BEHAVIOR TREE & UTILITY AI HYBRID FRAMEWORK
 * ============================================================================================
 *  Features:
 *   - Behavior Tree Engine with Zero-Allocation Execution Logic
 *   - Utility AI Engine with Mathematical Consideration Scoring Functions
 *   - Modern C++20 Architecture
 * ============================================================================================
 */

#ifndef AI_DECISION_ENGINE_HPP
#define AI_DECISION_ENGINE_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>
#include <functional>

namespace AAA_AIEngine
{
    // ============================================================================================
    // 1. BEHAVIOR TREE ARCHITECTURE
    // ============================================================================================

    enum class BTStatus
    {
        SUCCESS,
        FAILURE,
        RUNNING
    };

    struct BlackBoard
    {
        float Health{ 100.0f };
        float Energy{ 80.0f };
        float DistanceToTarget{ 15.0f };
        bool EnemyInSight{ true };
        bool HasAmmo{ true };
    };

    class BTNode
    {
    public:
        virtual ~BTNode() = default;
        virtual BTStatus Tick(BlackBoard& bb) = 0;
    };

    using BTNodePtr = std::shared_ptr<BTNode>;

    // Composite: Selector (OR Logic - returns SUCCESS on first node that succeeds)
    class BTSelector : public BTNode
    {
    private:
        std::vector<BTNodePtr> m_Children;

    public:
        void AddChild(BTNodePtr child) { m_Children.push_back(child); }

        BTStatus Tick(BlackBoard& bb) override
        {
            for (auto& child : m_Children)
            {
                BTStatus status = child->Tick(bb);
                if (status != BTStatus::FAILURE)
                {
                    return status;
                }
            }
            return BTStatus::FAILURE;
        }
    };

    // Composite: Sequence (AND Logic - returns FAILURE on first node that fails)
    class BTSequence : public BTNode
    {
    private:
        std::vector<BTNodePtr> m_Children;

    public:
        void AddChild(BTNodePtr child) { m_Children.push_back(child); }

        BTStatus Tick(BlackBoard& bb) override
        {
            for (auto& child : m_Children)
            {
                BTStatus status = child->Tick(bb);
                if (status != BTStatus::SUCCESS)
                {
                    return status;
                }
            }
            return BTStatus::SUCCESS;
        }
    };

    // Leaf: Condition
    class BTCondition : public BTNode
    {
    private:
        std::function<bool(const BlackBoard&)> m_Predicate;

    public:
        BTCondition(std::function<bool(const BlackBoard&)> predicate) : m_Predicate(predicate) {}

        BTStatus Tick(BlackBoard& bb) override
        {
            return m_Predicate(bb) ? BTStatus::SUCCESS : BTStatus::FAILURE;
        }
    };

    // Leaf: Action
    class BTAction : public BTNode
    {
    private:
        std::string m_Name;
        std::function<BTStatus(BlackBoard&)> m_ActionFunc;

    public:
        BTAction(const std::string& name, std::function<BTStatus(BlackBoard&)> action)
            : m_Name(name), m_ActionFunc(action) {}

        BTStatus Tick(BlackBoard& bb) override
        {
            std::cout << "[BT Execution] Executing Action: " << m_Name << "\n";
            return m_ActionFunc(bb);
        }
    };

    // ============================================================================================
    // 2. UTILITY AI ARCHITECTURE
    // ============================================================================================

    enum class ResponseCurveType
    {
        LINEAR,
        EXPONENTIAL,
        LOGISTIC
    };

    struct Consideration
    {
        std::string Name;
        std::function<float(const BlackBoard&)> InputExtractor; // Normalizes input to [0, 1]
        ResponseCurveType CurveType;
        float Exponent{ 1.0f };
        float Weight{ 1.0f };

        float Evaluate(const BlackBoard& bb) const
        {
            float input = std::clamp(InputExtractor(bb), 0.0f, 1.0f);
            float score = 0.0f;

            switch (CurveType)
            {
            case ResponseCurveType::LINEAR:
                score = input;
                break;
            case ResponseCurveType::EXPONENTIAL:
                score = std::pow(input, Exponent);
                break;
            case ResponseCurveType::LOGISTIC:
                score = 1.0f / (1.0f + std::exp(-10.0f * (input - 0.5f)));
                break;
            }

            return std::clamp(score, 0.0f, 1.0f) * Weight;
        }
    };

    class UtilityAction
    {
    public:
        std::string Name;
        std::vector<Consideration> Considerations;
        std::function<void(BlackBoard&)> ExecuteAction;

        float ScoreAction(const BlackBoard& bb) const
        {
            if (Considerations.empty()) return 0.0f;

            float totalScore = 1.0f;
            for (const auto& cons : Considerations)
            {
                float cScore = cons.Evaluate(bb);
                // Compensate for multi-consideration decay using dynamic normalization
                float modFactor = 1.0f - (1.0f / static_cast<float>(Considerations.size()));
                float makeUp = (1.0f - cScore) * modFactor;
                totalScore *= (cScore + (makeUp * cScore));
            }
            return totalScore;
        }
    };

    class UtilityAISystem
    {
    private:
        std::vector<UtilityAction> m_Actions;

    public:
        void RegisterAction(const UtilityAction& action) { m_Actions.push_back(action); }

        void SelectAndExecute(BlackBoard& bb)
        {
            float bestScore = -1.0f;
            const UtilityAction* bestAction = nullptr;

            std::cout << "\n--- [Utility AI Scoring Evaluation] ---\n";
            for (const auto& action : m_Actions)
            {
                float score = action.ScoreAction(bb);
                std::cout << "Action: " << action.Name << " | Calculated Utility Score: " << score << "\n";

                if (score > bestScore)
                {
                    bestScore = score;
                    bestAction = &action;
                }
            }

            if (bestAction && bestScore > 0.05f)
            {
                std::cout << "--> Winning Action Chosen: " << bestAction->Name << " (Score: " << bestScore << ")\n";
                bestAction->ExecuteAction(bb);
            }
            else
            {
                std::cout << "--> No Action Passed Utility Threshold. Idle State Active.\n";
            }
        }
    };
}

// ============================================================================================
// MAIN PIPELINE INTEGRATION
// ============================================================================================
int main()
{
    using namespace AAA_AIEngine;

    std::cout << "================================================================================\n";
    std::cout << " RUNNING HYBRID BEHAVIOR TREE & UTILITY AI AI ENGINE SYSTEM                      \n";
    std::cout << "================================================================================\n\n";

    BlackBoard bb;

    // --------------------------------------------------------------------------------------------
    // BEHAVIOR TREE INITIALIZATION
    // --------------------------------------------------------------------------------------------
    auto rootSelector = std::make_shared<BTSelector>();
    auto emergencySequence = std::make_shared<BTSequence>();

    // Node 1: Emergency Retreat Sequence
    emergencySequence->AddChild(std::make_shared<BTCondition>([](const BlackBoard& b) {
        return b.Health < 20.0f;
    }));
    emergencySequence->AddChild(std::make_shared<BTAction>("Retreat to Cover", [](BlackBoard& b) {
        std::cout << "   [BT Action] Health critical! Fall back immediately.\n";
        return BTStatus::SUCCESS;
    }));

    // Node 2: Attack Sequence
    auto combatSequence = std::make_shared<BTSequence>();
    combatSequence->AddChild(std::make_shared<BTCondition>([](const BlackBoard& b) {
        return b.EnemyInSight && b.HasAmmo;
    }));
    combatSequence->AddChild(std::make_shared<BTAction>("Fire Weapon", [](BlackBoard& b) {
        std::cout << "   [BT Action] Engaging enemy with primary weapon!\n";
        return BTStatus::SUCCESS;
    }));

    rootSelector->AddChild(emergencySequence);
    rootSelector->AddChild(combatSequence);

    // --------------------------------------------------------------------------------------------
    // UTILITY AI INITIALIZATION
    // --------------------------------------------------------------------------------------------
    UtilityAISystem utilitySystem;

    // Action 1: Attack
    UtilityAction attackAction;
    attackAction.Name = "Ranged Attack";
    attackAction.Considerations.push_back({
        "Target Distance",
        [](const BlackBoard& b) { return 1.0f - (b.DistanceToTarget / 50.0f); }, // Closer is better
        ResponseCurveType::LINEAR, 1.0f, 1.0f
    });
    attackAction.Considerations.push_back({
        "Energy Level",
        [](const BlackBoard& b) { return b.Energy / 100.0f; },
        ResponseCurveType::EXPONENTIAL, 2.0f, 1.0f
    });
    attackAction.ExecuteAction = [](BlackBoard& b) {
        std::cout << "   [Utility Exec] Ranged attack executed. Consumed 10 Energy.\n";
        b.Energy -= 10.0f;
    };

    // Action 2: Heal / Rest
    UtilityAction restAction;
    restAction.Name = "Rest & Recharge";
    restAction.Considerations.push_back({
        "Low Energy Utility",
        [](const BlackBoard& b) { return 1.0f - (b.Energy / 100.0f); }, // Lower energy gives higher score
        ResponseCurveType::EXPONENTIAL, 1.5f, 1.0f
    });
    restAction.ExecuteAction = [](BlackBoard& b) {
        std::cout << "   [Utility Exec] Resting... Recovered 30 Energy.\n";
        b.Energy += 30.0f;
    };

    utilitySystem.RegisterAction(attackAction);
    utilitySystem.RegisterAction(restAction);

    // --------------------------------------------------------------------------------------------
    // SIMULATION STEP
    // --------------------------------------------------------------------------------------------
    std::cout << "--- 1. TICKING BEHAVIOR TREE ---\n";
    rootSelector->Tick(bb);

    std::cout << "\n--- 2. TICKING UTILITY AI ---";
    utilitySystem.SelectAndExecute(bb);

    // Simulate State Shift
    std::cout << "\n================================================================================\n";
    std::cout << " SIMULATING STATE CHANGE: Low Energy & High Enemy Distance\n";
    std::cout << "================================================================================\n";
    bb.Energy = 15.0f;
    bb.DistanceToTarget = 40.0f;

    std::cout << "\n--- TICKING UTILITY AI AGAIN ---";
    utilitySystem.SelectAndExecute(bb);

    return 0;
}
