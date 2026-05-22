using System;
using System.Collections.Generic;

namespace AdvancedQuestSystem
{
    public enum EnemyType
    {
        GoblinMinion,
        OrcWarrior,
        ShadowMage,
        DragonBoss
    }

    public enum RewardType
    {
        Experience,
        Gold,
        FactionReputation,
        RareItemDrop
    }

    // Precise struct for managing reward data
    public struct RewardPayload
    {
        public RewardType Type { get; }
        public double Amount { get; } // Double for absolute high precision tracking
        public string SpecificItemId { get; }

        public RewardPayload(RewardType type, double amount, string itemId = "")
        {
            Type = type;
            Amount = amount;
            SpecificItemId = itemId;
        }
    }
}
