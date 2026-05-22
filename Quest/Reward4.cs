using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using AdvancedQuestSystem.Core;         // Referencing Step 1 High-Precision types
using AdvancedQuestSystem.Engine;       // Referencing Step 2 Pipeline Engine
using AdvancedQuestSystem.Objectives;   // Referencing Step 3 Advanced Objectives

namespace AdvancedQuestSystem.QuestEngine
{
    #region Quest Lifecycle States

    public enum QuestState : byte
    {
        Locked,              // Requirements not met yet
        Available,           // Ready to be accepted by player
        Active,              // Under progress tracking
        RequirementsMet,     // All objectives completed, waiting for claim invocation
        CompletedAndArchived // Finalized, rewards sent, history locked
    }

    #endregion

    #region Serialization & Database Mapping DTOs

    [Serializable]
    public class QuestFullSystemState
    {
        public string QuestId { get; set; }
        public string TechnicalTitle { get; set; }
        public QuestState CurrentLifecycleState { get; set; }
        public List<QuestObjectiveState> InnerObjectiveStates { get; set; } = new List<QuestObjectiveState>();
        public string SerializedTimestamp { get; set; }
    }

    #endregion

    #region Quest Management Notification Contracts

    public class QuestStateChangedEventArgs : EventArgs
    {
        public string QuestId { get; }
        public string Title { get; }
        public QuestState PreviousState { get; }
        public QuestState NewState { get; }

        public QuestStateChangedEventArgs(string questId, string title, QuestState previous, QuestState newSt)
        {
            QuestId = questId;
            Title = title;
            PreviousState = previous;
            NewState = newSt;
        }
    }

    #endregion

    #region The Central Master Quest Definition

    /// <summary>
    /// Thread-safe enterprise-grade core blueprint representing an active/inactive narrative or combat task matrix.
    /// Operates as a complex state controller mapping dynamic events down to sub-objective segments.
    /// </summary>
    public sealed class EliteQuest : IDisposable
    {
        private readonly ReaderWriterLockSlim _questSystemLock = new ReaderWriterLockSlim();
        
        // Identity & Core Information
        public string UniqueQuestId { get; }
        public string TechnicalTitle { get; }
        public string NarrativeLogDescription { get; }

        // State Trackers
        private QuestState _currentLifecycleState;
        private readonly List<PreciseKillObjective> _objectiveNodes;
        private readonly List<RewardPayload> _finalGrandCompletionRewards;
        
        // System Links
        private readonly IRewardEngineOrchestrator _centralRewardEngine;
        private readonly string _assignedPlayerContextId;

        // Prerequisite chain hooks (IDs of quests that must be archived before this unlocks)
        private readonly HashSet<string> _prerequisiteQuestIds;

        // System Broadcast System Observers
        public event EventHandler<QuestStateChangedEventArgs> OnQuestStateAltered;
        public event Action<EliteQuest> OnQuestObjectivesFullySatisfied;

        public QuestState CurrentState
        {
            get
            {
                _questSystemLock.EnterReadLock();
                try { return _currentLifecycleState; }
                finally { _questSystemLock.ExitReadLock(); }
            }
        }

        public EliteQuest(
            string uniqueQuestId,
            string technicalTitle,
            string narrativeLogDescription,
            List<PreciseKillObjective> objectiveNodes,
            List<RewardPayload> finalGrandCompletionRewards,
            IRewardEngineOrchestrator centralRewardEngine,
            string assignedPlayerContextId,
            IEnumerable<string> prerequisiteQuestIds = null)
        {
            UniqueQuestId = !string.IsNullOrEmpty(uniqueQuestId) ? uniqueQuestId : Guid.NewGuid().ToString();
            TechnicalTitle = technicalTitle ?? "Unnamed Structural Quest Module";
            NarrativeLogDescription = narrativeLogDescription ?? "No description logs injected.";
            _objectiveNodes = objectiveNodes ?? throw new ArgumentException("Objective nodes cluster cannot be null.");
            _finalGrandCompletionRewards = finalGrandCompletionRewards ?? new List<RewardPayload>();
            _centralRewardEngine = centralRewardEngine ?? throw new ArgumentNullException(nameof(centralRewardEngine));
            _assignedPlayerContextId = assignedPlayerContextId;
            _prerequisiteQuestIds = prerequisiteQuestIds != null ? new HashSet<string>(prerequisiteQuestIds) : new HashSet<string>();
            
            _currentLifecycleState = QuestState.Locked;

            // Wire up event registration hooks into lower components for decoupled cross-communication loops
            HookIntoObjectiveEvents();
        }

        private void HookIntoObjectiveEvents()
        {
            foreach (var objective in _objectiveNodes)
            {
                objective.OnProgressChanged += HandleSubObjectiveProgressNotification;
                objective.OnObjectiveSatisfied += HandleSubObjectiveCompletionSequence;
            }
        }

        #region Quest Lifecycle State Modification Handlers

        public bool EvaluateAcceptanceEligibility(HashSet<string> playerArchivedQuestHistory)
        {
            _questSystemLock.EnterWriteLock();
            try
            {
                if (_currentLifecycleState != QuestState.Locked) return false;

                // Precision check against pre-requisite collection maps
                foreach (var prereqId in _prerequisiteQuestIds)
                {
                    if (!playerArchivedQuestHistory.Contains(prereqId))
                    {
                        return false; // Dependency chain broken
                    }
                }

                TransitionToStateInternal(QuestState.Available);
                return true;
            }
            finally
            {
                _questSystemLock.ExitWriteLock();
            }
        }

        public bool InitializeQuestActivation()
        {
            _questSystemLock.EnterWriteLock();
            try
            {
                if (_currentLifecycleState != QuestState.Available) return false;

                TransitionToStateInternal(QuestState.Active);
                return true;
            }
            finally
            {
                _questSystemLock.ExitWriteLock();
            }
        }

        private void TransitionToStateInternal(QuestState targetNewState)
        {
            QuestState temporaryOldState = _currentLifecycleState;
            _currentLifecycleState = targetNewState;

            // Fire structural logs outside of locked scopes safely inside local tasks
            Task.Run(() => {
                OnQuestStateAltered?.Invoke(this, new QuestStateChangedEventArgs(
                    UniqueQuestId, TechnicalTitle, temporaryOldState, targetNewState));
            });
        }

        #endregion

        #region Core Interception Engine Router

        /// <summary>
        /// Central event distributor router pushing raw combat event notifications directly into target structural nodes.
        /// </summary>
        public void ProcessGlobalKillEvent(EnemyStaticProfile slainEnemyMetadata, int rawCount)
        {
            _questSystemLock.EnterReadLock();
            try
            {
                // Safety filter block: Do not waste allocations tracking data if the state is not actively listening
                if (_currentLifecycleState != QuestState.Active) return;
            }
            finally
            {
                _questSystemLock.ExitReadLock();
            }

            // Route execution commands downward. Multi-threading protection is handled directly by child instances.
            foreach (var objective in _objectiveNodes)
            {
                objective.InterceptEnemyDeathEvent(slainEnemyMetadata, rawCount);
            }
        }

        #endregion

        #region Internal Communication Callbacks

        private void HandleSubObjectiveProgressNotification(object sender, ObjectiveProgressChangedEventArgs args)
        {
            // Bubble events directly to global telemetry metrics monitor managers if needed
            Console.WriteLine($"[Telemetry Log] Quest '{TechnicalTitle}' - Child Component Event Update: Objective {args.ObjectiveId} updated. Progress: {args.TotalCurrentProgress}");
        }

        private void HandleSubObjectiveCompletionSequence(PreciseKillObjective reportingObjective)
        {
            bool triggerFinalPhaseClaimCheck = false;

            _questSystemLock.EnterWriteLock();
            try
            {
                if (_currentLifecycleState != QuestState.Active) return;

                // Structural verification across all inner evaluation entities
                bool evaluationMatrixFullySatisfied = _objectiveNodes.All(obj => obj.IsCompleted);

                if (evaluationMatrixFullySatisfied)
                {
                    TransitionToStateInternal(QuestState.RequirementsMet);
                    triggerFinalPhaseClaimCheck = true;
                }
            }
            finally
            {
                _questSystemLock.ExitWriteLock();
            }

            if (triggerFinalPhaseClaimCheck)
            {
                OnQuestObjectivesFullySatisfied?.Invoke(this);
            }
        }

        #endregion

        #region Grand Rewards Claim Pipeline

        /// <summary>
        /// Finalizes the quest lifecycle. Fires asynchronous distributed database operations 
        /// to allocate completion assets to the user's accounts.
        /// </summary>
        public async Task<bool> ProcessFinalQuestRewardsClaimAsync(CancellationToken externalCancelToken = default)
        {
            _questSystemLock.EnterWriteLock();
            try
            {
                if (_currentLifecycleState != QuestState.RequirementsMet) return false;
            }
            finally
            {
                _questSystemLock.ExitWriteLock();
            }

            try
            {
                var rewardContext = new RewardExecutionContext(
                    _assignedPlayerContextId,
                    globalServerMultiplier: 1.0, // Base execution scale rules
                    partySizeBonus: 0.0,
                    isPremiumAccount: true,
                    externalCancelToken
                );

                Console.WriteLine($"\n[Engine Execution Node] Initiating grand completion package drop for quest: '{TechnicalTitle}'...");
                
                // Pushing completion matrix structural objects downstream to pipeline segments
                var batchDispatchedLogs = await _centralRewardEngine.DispatchBatchRewardsAsync(_finalGrandCompletionRewards, rewardContext);

                // Verification check across transaction outcomes
                if (batchDispatchedLogs.All(log => log.IsSuccess))
                {
                    _questSystemLock.EnterWriteLock();
                    try
                    {
                        TransitionToStateInternal(QuestState.CompletedAndArchived);
                    }
                    finally
                    {
                        _questSystemLock.ExitWriteLock();
                    }
                    Console.WriteLine($"[Engine Execution Node] Quest '{TechnicalTitle}' successfully archived permanently inside persistent storage clusters.");
                    return true;
                }

                Console.WriteLine($"[Engine Critical Anomaly] Reward processing execution rejected block completion. Postponing lifecycle adjustments.");
                return false;
            }
            catch (Exception transactionalException)
            {
                Console.WriteLine($"[Engine Fatal Error] Rewards collection execution pipeline crash context details: {transactionalException.Message}");
                return false;
            }
        }

        #endregion

        #region Deep State Serialization Rehydration

        public QuestFullSystemState CaptureFullQuestTreeState()
        {
            _questSystemLock.EnterReadLock();
            try
            {
                var combinedDataTree = new QuestFullSystemState
                {
                    QuestId = this.UniqueQuestId,
                    TechnicalTitle = this.TechnicalTitle,
                    CurrentLifecycleState = this._currentLifecycleState,
                    SerializedTimestamp = DateTime.UtcNow.ToString("O")
                };

                foreach (var objNode in _objectiveNodes)
                {
                    combinedDataTree.InnerObjectiveStates.Add(objNode.CaptureStateSnapshot());
                }

                return combinedDataTree;
            }
            finally
            {
                _questSystemLock.ExitReadLock();
            }
        }

        public void RehydrateFullQuestTreeState(QuestFullSystemState targetDataTree)
        {
            if (targetDataTree == null || targetDataTree.QuestId != UniqueQuestId) return;

            _questSystemLock.EnterWriteLock();
            try
            {
                _currentLifecycleState = targetDataTree.CurrentLifecycleState;
                
                // Map out sub-objective arrays safely back to memory context allocations
                foreach (var objState in targetDataTree.InnerObjectiveStates)
                {
                    var matchingNode = _objectiveNodes.FirstOrDefault(n => n.UniqueObjectiveId == objState.ObjectiveId);
                    matchingNode?.RestoreStateFromSnapshot(objState);
                }
            }
            finally
            {
                _questSystemLock.ExitWriteLock();
            }
        }

        #endregion

        #region System Unwiring Protocols

        public void Dispose()
        {
            foreach (var objective in _objectiveNodes)
            {
                objective.OnProgressChanged -= HandleSubObjectiveProgressNotification;
                objective.OnObjectiveSatisfied -= HandleSubObjectiveCompletionSequence;
                objective.Dispose();
            }
            _questSystemLock.Dispose();
        }

        #endregion
    }

    #endregion
}
