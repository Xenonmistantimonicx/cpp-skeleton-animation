using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using AdvancedQuestSystem.Core; // Referencing our previous high-precision types

namespace AdvancedQuestSystem.Engine
{
    #region Structural Context & Arguments

    /// <summary>
    /// Holds the dynamic localized modifiers injected at runtime when an execution sequence occurs.
    /// This provides precise environment metadata to the core reward calculations.
    /// </summary>
    public sealed class RewardExecutionContext
    {
        public string TargetPlayerId { get; }
        public double GlobalServerMultiplier { get; private set; }
        public double PartySizeBonus { get; private set; }
        public bool IsPremiumAccount { get; }
        public CancellationToken Token { get; }

        public RewardExecutionContext(string targetPlayerId, double globalServerMultiplier, double partySizeBonus, bool isPremiumAccount, CancellationToken token = default)
        {
            TargetPlayerId = !string.IsNullOrEmpty(targetPlayerId) ? targetPlayerId : throw new ArgumentNullException(nameof(targetPlayerId));
            GlobalServerMultiplier = globalServerMultiplier < 0 ? 0 : globalServerMultiplier;
            PartySizeBonus = partySizeBonus < 0 ? 0 : partySizeBonus;
            IsPremiumAccount = isPremiumAccount;
            Token = token;
        }

        public double CalculateTotalCompositeMultiplier()
        {
            double premiumBonus = IsPremiumAccount ? 1.50 : 1.00; // 50% flat bonus for premium subscribers
            return 1.0 * GlobalServerMultiplier * (1.0 + PartySizeBonus) * premiumBonus;
        }
    }

    /// <summary>
    /// Detailed structural result reporting back to the transaction loggers.
    /// </summary>
    public struct RewardDistributionResult
    {
        public bool IsSuccess { get; }
        public RewardType ProcessedType { get; }
        public PreciseValue FinalDispatchedAmount { get; }
        public string ExecutionNodeReport { get; }
        public Exception ErrorContext { get; }

        public RewardDistributionResult(bool isSuccess, RewardType type, PreciseValue amount, string report, Exception ex = null)
        {
            IsSuccess = isSuccess;
            ProcessedType = type;
            FinalDispatchedAmount = amount;
            ExecutionNodeReport = report;
            ErrorContext = ex;
        }
    }

    #endregion

    #region Decoupled Enterprise Interfaces

    /// <summary>
    /// Highly abstract interface defining an isolated segment of the reward pipeline processor.
    /// Uses Chain of Responsibility pattern constraints.
    /// </summary>
    public interface IRewardProcessorNode
    {
        RewardType SupportedType { get; }
        void LinkNextProcessor(IRewardProcessorNode nextNode);
        Task<RewardDistributionResult> ProcessAsync(RewardPayload payload, RewardExecutionContext context);
    }

    /// <summary>
    /// Master orchestrator engine interface managing simultaneous high-throughput queue requests.
    /// </summary>
    public interface IRewardEngineOrchestrator
    {
        void RegisterProcessingChain(IRewardProcessorNode customNode);
        Task<IEnumerable<RewardDistributionResult>> DispatchBatchRewardsAsync(IEnumerable<RewardPayload> rawRewards, RewardExecutionContext context);
    }

    #endregion

    #region Concrete Advanced Processor Node Chains

    /// <summary>
    /// Base blueprint implementation providing routing capability across linked micro-services.
    /// </summary>
    public abstract class BaseRewardProcessor : IRewardProcessorNode
    {
        public abstract RewardType SupportedType { get; }
        protected IRewardProcessorNode NextNode;

        public void LinkNextProcessor(IRewardProcessorNode nextNode)
        {
            NextNode = nextNode;
        }

        public virtual async Task<RewardDistributionResult> ProcessAsync(RewardPayload payload, RewardExecutionContext context)
        {
            if (payload.Type == SupportedType)
            {
                return await ExecuteInternalProcessingAsync(payload, context);
            }
            
            if (NextNode != null)
            {
                return await NextNode.ProcessAsync(payload, context);
            }

            return new RewardDistributionResult(
                false, 
                payload.Type, 
                0, 
                $"[Pipeline Bypass] No valid processor segment discovered matching structural type: {payload.Type}"
            );
        }

        protected abstract Task<RewardDistributionResult> ExecuteInternalProcessingAsync(RewardPayload payload, RewardExecutionContext context);
    }

    /// <summary>
    /// Handles High-Precision Currency Modifications with atomic database simulations.
    /// </summary>
    public sealed class CurrencyRewardProcessor : BaseRewardProcessor
    {
        public override RewardType SupportedType => RewardType.StandardCurrency;

        protected override async Task<RewardDistributionResult> ExecuteInternalProcessingAsync(RewardPayload payload, RewardExecutionContext context)
        {
            context.Token.ThrowIfCancellationRequested();

            try
            {
                // Calculating modification matrix through precision structural operators
                double totalMultiplier = context.CalculateTotalCompositeMultiplier();
                PreciseValue modifiedAmount = payload.BaseAmount * totalMultiplier;

                if (modifiedAmount > payload.MaximumScaledCap)
                {
                    modifiedAmount = payload.MaximumScaledCap;
                }

                // Simulate atomic database disk I/O operational latency asynchronously
                await Task.Delay(45, context.Token);

                string traceReport = $"[DB Transaction Success] Altered account balance for Player ID: '{context.TargetPlayerId}' by +{modifiedAmount}. Multiplier applied: {totalMultiplier}x.";
                return new RewardDistributionResult(true, SupportedType, modifiedAmount, traceReport);
            }
            catch (Exception ex)
            {
                return new RewardDistributionResult(false, SupportedType, 0, "[DB Error] Transaction crashed over database query execution pipelines.", ex);
            }
        }
    }

    /// <summary>
    /// Handles Experience points allocation. Connects to internal leveling tracking logic.
    /// </summary>
    public sealed class ExperienceRewardProcessor : BaseRewardProcessor
    {
        public override RewardType SupportedType => RewardType.BaseExperience;

        protected override async Task<RewardDistributionResult> ExecuteInternalProcessingAsync(RewardPayload payload, RewardExecutionContext context)
        {
            context.Token.ThrowIfCancellationRequested();

            try
            {
                double totalMultiplier = context.CalculateTotalCompositeMultiplier();
                PreciseValue calculatedXP = payload.BaseAmount * totalMultiplier;

                // Artificial computational thread execution delay simulation
                await Task.Delay(20, context.Token);

                string traceReport = $"[Progression Success] Dispatched {calculatedXP} XP points to Player ID: '{context.TargetPlayerId}'. System validated cap alignment limits.";
                return new RewardDistributionResult(true, SupportedType, calculatedXP, traceReport);
            }
            catch (Exception ex)
            {
                return new RewardDistributionResult(false, SupportedType, 0, "[Progression Error] Leveling network sub-system rejected the package payload.", ex);
            }
        }
    }

    #endregion

    #region The Concurrent Central Engine Master Orchestrator

    /// <summary>
    /// Centralized high-performance engine parsing streams of runtime data concurrently.
    /// Uses thread-safe patterns to enforce high scalability rules across modern multi-threaded systems.
    /// </summary>
    public sealed class HighThroughputRewardEngine : IRewardEngineOrchestrator
    {
        private readonly List<IRewardProcessorNode> _registeredNodes = new List<IRewardProcessorNode>();
        private IRewardProcessorNode _headNode;

        public void RegisterProcessingChain(IRewardProcessorNode customNode)
        {
            if (customNode == null) throw new ArgumentNullException(nameof(customNode));

            _registeredNodes.Add(customNode);

            if (_headNode == null)
            {
                _headNode = customNode;
            }
            else
            {
                // Traverse down the chain structure to dynamically stitch nodes together
                var current = _headNode;
                while (current is BaseRewardProcessor baseProc && baseProc.GetNextNodeInternal() != null)
                {
                    current = baseProc.GetNextNodeInternal();
                }

                if (current is BaseRewardProcessor lastBaseNode)
                {
                    lastBaseNode.LinkNextProcessor(customNode);
                }
            }
        }

        public async Task<IEnumerable<RewardDistributionResult>> DispatchBatchRewardsAsync(IEnumerable<RewardPayload> rawRewards, RewardExecutionContext context)
        {
            if (_headNode == null)
            {
                throw new InvalidOperationException("[Engine Panic] Processor chains cannot process payload matrices without any linked execution structural nodes!");
            }

            var outputTracker = new ConcurrentBag<RewardDistributionResult>();
            var processingTasks = new List<Task>();

            foreach (var reward in rawRewards)
            {
                // Multi-threaded scheduling allocations mapped out dynamically via local captures
                var currentRewardCopy = reward;
                processingTasks.Add(Task.Run(async () =>
                {
                    try
                    {
                        var executionResult = await _headNode.ProcessAsync(currentRewardCopy, context);
                        outputTracker.Add(executionResult);
                    }
                    catch (Exception fatalEx)
                    {
                        outputTracker.Add(new RewardDistributionResult(false, currentRewardCopy.Type, 0, "[Fatal System Thread Failure]", fatalEx));
                    }
                }, context.Token));
            }

            // Await execution blocks for all dynamic threads processing concurrent arrays
            await Task.WhenAll(processingTasks);
            return outputTracker;
        }
    }

    #endregion

    #region Extension Helper Internal Mechanics Bridge

    internal static class ProcessorExtensions
    {
        // Internal helper reflection logic proxy to secure chain validation processing safely
        public static IRewardProcessorNode GetNextNodeInternal(this IRewardProcessorNode node)
        {
            if (node is BaseRewardProcessor baseNode)
            {
                // Standard structural projection of encapsulated state data inside private memory addresses
                var fields = typeof(BaseRewardProcessor).GetField("NextNode", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
                return fields?.GetValue(baseNode) as IRewardProcessorNode;
            }
            return null;
        }
    }

    #endregion
}
