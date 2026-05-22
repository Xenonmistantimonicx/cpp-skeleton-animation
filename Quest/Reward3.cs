using System;
using System.Collections.Generic;
using System.Threading;
using AdvancedQuestSystem.Core;    // High-Precision structs from Step 1
using AdvancedQuestSystem.Engine;  // Pipeline Engine from Step 2

namespace AdvancedQuestSystem.Objectives
{
    #region State Persistence DTOs

    /// <summary>
    /// Data Transfer Object (DTO) explicitly designed for DB serialization/deserialization.
    /// Keeps quest tracking pure and abstract from memory pointer allocations.
    /// </summary>
    [Serializable]
    public class QuestObjectiveState
    {
        public string ObjectiveId { get; set; }
        public int CurrentProgressCounter { get; set; }
        public bool IsDispatchedAndFinalized { get; set; }
        public string LastUpdatedTimestamp { get; set; }
    }

    #endregion

    #region Execution Delegates & Event Contracts

    public class ObjectiveProgressChangedEventArgs : EventArgs
    {
        public string ObjectiveId { get; }
        public int DeltaIncrement { get; }
        public int TotalCurrentProgress { get; }
        public bool MarkedCompletedNow { get; }

        public ObjectiveProgressChangedEventArgs(string objectiveId, int delta, int current, bool completed)
        {
            ObjectiveId = objectiveId;
            DeltaIncrement = delta;
            TotalCurrentProgress = current;
            MarkedCompletedNow = completed;
        }
    }

    #endregion

    #region The Concrete Precise Objective Module

    /// <summary>
    /// Thread-safe, highly modular and precise objective evaluator.
    /// Tracks complex structural parameters across active runtime entities.
    /// </summary>
    public sealed class PreciseKillObjective : IDisposable
    {
        // Thread synchronization lock primitive
        private readonly ReaderWriterLockSlim _stateLock = new ReaderWriterLockSlim();

        // Structural Definitions
        public string UniqueObjectiveId { get; }
        public string DescriptionBlueprint { get; }
        
        private readonly EnemyClassification _targetClassificationCriteria;
        private readonly int _targetRequiredCount;
        private readonly int _minimumThreatLevelRequirement;
        
        // State Fields
        private int _currentProgressCount;
        private bool _isCompleted;
        private bool _isDispatched;

        // Rewards Data Pipeline Links
        private readonly List<RewardPayload> _perKillRewardMatrix;
        private readonly IRewardEngineOrchestrator _rewardEngine;
        private readonly string _assignedPlayerContextId;

        // Advanced Runtime Context: Predicate filtering for dynamic rule processing (e.g. stealth kills only, timed kills)
        private readonly Func<EnemyStaticProfile, bool> _customValidationPredicate;

        // Structural Observers (Events)
        public event EventHandler<ObjectiveProgressChangedEventArgs> OnProgressChanged;
        public event Action<PreciseKillObjective> OnObjectiveSatisfied;

        #region Properties

        public bool IsCompleted
        {
            get
            {
                _stateLock.EnterReadLock();
                try { return _isCompleted; }
                finally { _stateLock.ExitReadLock(); }
            }
        }

        public int CurrentProgress
        {
            get
            {
                _stateLock.EnterReadLock();
                try { return _currentProgressCount; }
                finally { _stateLock.ExitReadLock(); }
            }
        }

        #endregion

        public PreciseKillObjective(
            string uniqueObjectiveId,
            string descriptionBlueprint,
            EnemyClassification targetClassificationCriteria,
            int targetRequiredCount,
            int minThreatLevel,
            List<RewardPayload> perKillRewardMatrix,
            IRewardEngineOrchestrator rewardEngine,
            string assignedPlayerContextId,
            Func<EnemyStaticProfile, bool> customValidationPredicate = null)
        {
            UniqueObjectiveId = !string.IsNullOrEmpty(uniqueObjectiveId) ? uniqueObjectiveId : Guid.NewGuid().ToString();
            DescriptionBlueprint = descriptionBlueprint ?? "Eliminate designated target matrices.";
            _targetClassificationCriteria = targetClassificationCriteria;
            _targetRequiredCount = targetRequiredCount > 0 ? targetRequiredCount : 1;
            _minimumThreatLevelRequirement = minThreatLevel;
            _perKillRewardMatrix = perKillRewardMatrix ?? new List<RewardPayload>();
            _rewardEngine = rewardEngine ?? throw new ArgumentNullException(nameof(rewardEngine));
            _assignedPlayerContextId = assignedPlayerContextId;
            _customValidationPredicate = customValidationPredicate ?? (profile => true); // Default to absolute true if no filter is injected
            
            _currentProgressCount = 0;
            _isCompleted = false;
            _isDispatched = false;
        }

        /// <summary>
        /// Evaluates a real-time combat death interception event. 
        /// Uses high-precision locking loops to prevent packet collision in multi-threaded network engines.
        /// </summary>
        public void InterceptEnemyDeathEvent(EnemyStaticProfile slainEnemyProfile, int executionContextCount)
        {
            if (slainEnemyProfile == null || executionContextCount <= 0) return;

            // Step 1: Structural verification using short-circuit bitwise checks before acquiring heavy write-locks
            if ((slainEnemyProfile.Classification & _targetClassificationCriteria) != _targetClassificationCriteria) return;
            if (slainEnemyProfile.ThreatLevel < _minimumThreatLevelRequirement) return;
            if (!_customValidationPredicate(slainEnemyProfile)) return;

            bool triggersCompletionNow = false;
            int recordedDelta = 0;
            int postUpdateProgress = 0;

            // Step 2: Thread-Safe State Write Modification Zone
            _stateLock.EnterWriteLock();
            try
            {
                if (_isCompleted) return;

                for (int i = 0; i < executionContextCount; i++)
                {
                    if (_currentProgressCount >= _targetRequiredCount) break;

                    _currentProgressCount++;
                    recordedDelta++;

                    // Execution Node Reward Pipeline dispatch simulation per valid atomic entity elimination
                    ExecuteAtomicKillRewardAllocation(slainEnemyProfile);
                }

                postUpdateProgress = _currentProgressCount;

                if (_currentProgressCount >= _targetRequiredCount)
                {
                    _isCompleted = true;
                    triggersCompletionNow = true;
                }
            }
            finally
            {
                _stateLock.ExitWriteLock();
            }

            // Step 3: Event Notification Propagation (Safely executed outside of lock scopes to prevent cross-thread deadlocks)
            if (recordedDelta > 0)
            {
                OnProgressChanged?.Invoke(this, new ObjectiveProgressChangedEventArgs(
                    UniqueObjectiveId, recordedDelta, postUpdateProgress, triggersCompletionNow));
            }

            if (triggersCompletionNow)
            {
                OnObjectiveSatisfied?.Invoke(this);
            }
        }

        /// <summary>
        /// Direct injection pipeline pushing granular calculations instantly to the database network nodes.
        /// </summary>
        private void ExecuteAtomicKillRewardAllocation(EnemyStaticProfile contextProfile)
        {
            // Dynamic progression scaling calculation using custom high-precision types
            // Scale multiplier is modified by the structural tier of the objective vs enemy threat differential
            double internalScaleFactor = 1.0 + ((double)contextProfile.ThreatLevel / 100.0);
            
            var temporaryContext = new RewardExecutionContext(
                _assignedPlayerContextId, 
                internalScaleFactor, 
                partySizeBonus: 0.15, // 15% flat bonus simulation for distributed network groupings
                isPremiumAccount: true
            );

            // Fire-and-forget processing over the thread pool to shield main thread mechanics
            Task.Run(async () =>
            {
                try
                {
                    await _rewardEngine.DispatchBatchRewardsAsync(_perKillRewardMatrix, temporaryContext);
                }
                catch (Exception pipelinePanicException)
                {
                    // Redirect anomalies immediately to central system telemetry streams
                    Console.WriteLine($"[Telemetry Critical Fault] Objective Reward Pipeline collapsed: {pipelinePanicException.Message}");
                }
            });
        }

        #region State Management & Serialization Protocols

        /// <summary>
        /// Captures the structural state memory footprint to generate a clean serializable snapshot.
        /// </summary>
        public QuestObjectiveState CaptureStateSnapshot()
        {
            _stateLock.EnterReadLock();
            try
            {
                return new QuestObjectiveState
                {
                    ObjectiveId = this.UniqueObjectiveId,
                    CurrentProgressCounter = this._currentProgressCount,
                    IsDispatchedAndFinalized = this._isCompleted,
                    LastUpdatedTimestamp = DateTime.UtcNow.ToString("O")
                };
            }
            finally
            {
                _stateLock.ExitReadLock();
            }
        }

        /// <summary>
        /// Safely reconstructs memory metrics back from historical database states during session rehydration.
        /// </summary>
        public void RestoreStateFromSnapshot(QuestObjectiveState snapshot)
        {
            if (snapshot == null || snapshot.ObjectiveId != UniqueObjectiveId) return;

            _stateLock.EnterWriteLock();
            try
            {
                _currentProgressCount = snapshot.CurrentProgressCounter;
                _isCompleted = snapshot.IsDispatchedAndFinalized;
            }
            finally
            {
                _stateLock.ExitWriteLock();
            }
        }

        #endregion

        #region Resource Clean-up Handling

        public void Dispose()
        {
            _stateLock.Dispose();
        }

        #endregion
    }

    #endregion
}
