using System;
using System.Collections.Generic;

namespace AdvancedQuestSystem.Core
{
    #region Advanced Enums & Bitwise Flags

    /// <summary>
    /// Bitwise flags to define Enemy Attributes. 
    /// Allows a single enemy instance to possess multiple classifications simultaneously.
    /// </summary>
    [Flags]
    public enum EnemyClassification : uint
    {
        None         = 0,
        Humanoid     = 1 << 0,  // 1
        Beast        = 1 << 1,  // 2
        Undead       = 1 << 2,  // 4
        Demon        = 1 << 3,  // 8
        Elemental    = 1 << 4,  // 16
        Mechanical   = 1 << 5,  // 32
        Elite        = 1 << 28, // High bit for scaling
        Boss         = 1 << 29, 
        RaidBoss     = 1 << 30
    }

    /// <summary>
    /// Categorizes the elemental alignment of the enemy, which impacts dynamic high-precision reward multipliers.
    /// </summary>
    public enum ElementalAffiliation : byte
    {
        Physical,
        Fire,
        Frost,
        Void,
        Holy,
        Chaos
    }

    /// <summary>
    /// Granular tracking of all possible reward categories including progression mechanics.
    /// </summary>
    public enum RewardType : ushort
    {
        BaseExperience       = 100,
        BonusExperience      = 101,
        StandardCurrency     = 200,
        PremiumCurrency      = 201,
        FactionReputation    = 300,
        GuildContribution    = 301,
        LootTableRoll        = 400,
        TokenDrop            = 401
    }

    #endregion

    #region High-Precision Math & Storage Structures

    /// <summary>
    /// A high-precision, fixed-point alternative structure for handling currency and fractional rewards 
    /// without the deterministic floating-point precision loss inherent in float/double primitives.
    /// </summary>
    public struct PreciseValue : IEquatable<PreciseValue>, IComparable<PreciseValue>
    {
        private readonly long _scaledValue;
        private const long ScalingFactor = 10000; // 4 Decimal places precision protection

        public double AsDouble => (double)_scaledValue / ScalingFactor;
        public decimal AsDecimal => (decimal)_scaledValue / ScalingFactor;

        public PreciseValue(double rawValue)
        {
            _scaledValue = (long)Math.Round(rawValue * ScalingFactor);
        }

        private PreciseValue(long scaledValue, bool internalConstruct)
        {
            _scaledValue = scaledValue;
        }

        // Implicit and Explicit Casting Operators for clean mathematical integrations
        public static implicit operator PreciseValue(double val) => new PreciseValue(val);
        public static implicit operator PreciseValue(int val) => new PreciseValue((double)val);
        
        // Operator Overloading for Precise calculations in heavy combat iterations
        public static PreciseValue operator +(PreciseValue a, PreciseValue b) => new PreciseValue(a._scaledValue + b._scaledValue, true);
        public static PreciseValue operator -(PreciseValue a, PreciseValue b) => new PreciseValue(a._scaledValue - b._scaledValue, true);
        public static PreciseValue operator *(PreciseValue a, double multiplier) => new PreciseValue((long)Math.Round(a._scaledValue * multiplier), true);
        public static PreciseValue operator /(PreciseValue a, double divisor) => new PreciseValue((long)Math.Round(a._scaledValue / divisor), true);

        public bool Equals(PreciseValue other) => _scaledValue == other._scaledValue;
        public override bool Equals(object obj) => obj is PreciseValue other && Equals(other);
        public override int GetHashCode() => _scaledValue.GetHashCode();
        public int CompareTo(PreciseValue other) => _scaledValue.CompareTo(other._scaledValue);
        public override string ToString() => AsDecimal.ToString("F4");
    }

    /// <summary>
    /// Highly detailed struct containing immutable historical and tracking data for a structural reward item.
    /// </summary>
    public struct RewardPayload
    {
        public RewardType Type { get; }
        public PreciseValue BaseAmount { get; }
        public PreciseValue MaximumScaledCap { get; }
        public string SourceOriginId { get; }
        public Dictionary<string, string> MetadataContext { get; }

        public RewardPayload(RewardType type, PreciseValue baseAmount, PreciseValue maxCap, string sourceOriginId)
        {
            Type = type;
            BaseAmount = baseAmount;
            MaximumScaledCap = maxCap;
            SourceOriginId = !string.IsNullOrEmpty(sourceOriginId) ? sourceOriginId : throw new ArgumentNullException(nameof(sourceOriginId));
            MetadataContext = new Dictionary<string, string>();
        }

        public void AddMetadata(string key, string value)
        {
            if (MetadataContext.ContainsKey(key))
                MetadataContext[key] = value;
            else
                MetadataContext.Add(key, value);
        }
    }

    #endregion

    #region Flyweight Pattern for Enemy MetaData Profiles

    /// <summary>
    /// Flyweight Meta profile. Instead of duplicating data across 10,000 enemy runtime instances, 
    /// they point to this singular precise definitions blueprint.
    /// </summary>
    public sealed class EnemyStaticProfile
    {
        public string UniqueId { get; }
        public string TechnicalName { get; }
        public EnemyClassification Classification { get; }
        public ElementalAffiliation ElementalType { get; }
        public int ThreatLevel { get; }
        
        // Structuring the reward matrices directly into the static metadata
        public List<RewardPayload> StaticBaseRewardMatrix { get; }

        public EnemyStaticProfile(
            string uniqueId, 
            string technicalName, 
            EnemyClassification classification, 
            ElementalAffiliation elementalType, 
            int threatLevel)
        {
            UniqueId = uniqueId;
            TechnicalName = technicalName;
            Classification = classification;
            ElementalType = elementalType;
            ThreatLevel = threatLevel;
            StaticBaseRewardMatrix = new List<RewardPayload>();
        }
        
        public bool MatchesClassification(EnemyClassification flags)
        {
            return (Classification & flags) == flags;
        }
    }

    /// <summary>
    /// Thread-safe Blueprint Factory to registry and fetch static Profiles precisely.
    /// </summary>
    public static class EnemyRegistry
    {
        private static readonly Dictionary<string, EnemyStaticProfile> _profiles = new Dictionary<string, EnemyStaticProfile>();
        private static readonly object _lock = new object();

        public static void RegisterProfile(EnemyStaticProfile profile)
        {
            lock (_lock)
            {
                if (!_profiles.ContainsKey(profile.UniqueId))
                {
                    _profiles.Add(profile.UniqueId, profile);
                }
            }
        }

        public static EnemyStaticProfile GetProfile(string uniqueId)
        {
            lock (_lock)
            {
                if (_profiles.TryGetValue(uniqueId, out var profile))
                {
                    return profile;
                }
                throw new KeyNotFoundException($"[Registry Error] Profile with ID {uniqueId} is missing from active memory allocations.");
            }
        }
    }

    #endregion
}
