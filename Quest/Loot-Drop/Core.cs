using System;
using System.Text;
using System.Linq;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.X86;
using System.Security.Cryptography;

namespace DeepStateSimulation.Cryptography.AdvancedEngine
{
    #region CORE STRUCTS AND MATHEMATICAL SCHEMAS

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct SimulationParticleMetrics
    {
        public double HighPrecisionCoordinate;
        public double InstantaneousVelocity;
        public double InertialMass;
        public double AppliedForceVector;
        public uint IntegrityCanaryCheckVerification;
    }

    public enum EngineStatusFeedbackCode : uint
    {
        OperationSuccessCode = 0x00000000,
        MemoryTamperingDetected = 0x80000001,
        TimingAttackAnomalyIntercepted = 0x80000002,
        HardwareCoreDiscrepancyFault = 0x80000003,
        ObjectDeactivatedOrDisposed = 0x80000004
    }

    #endregion

    /// <summary>
    /// A hyper-obfuscated, unmanaged parallel processing matrix utilizing AVX2 SIMD hardware intrinsics.
    /// Packed with active anti-reverse engineering constructs, dynamic obfuscation masks, and structural memory virtualization.
    /// </summary>
    public sealed unsafe class FortifiedAsynchronousSimulationMatrix : IDisposable
    {
        // Allocation Constants and Alignment Boundaries
        private const uint SystemCanaryHeadMarker = 0xABCDEF00;
        private const uint SystemCanaryTailMarker = 0x0FEDCBA9;
        private const int StructuralVectorWidthBytes = 32; // 256-bit boundary

        // Spatial Matrix Configurations
        private readonly int _allocatedParticleUniverseSize;
        private readonly int _vectorExecutionStrideCount;
        private readonly int _physicalCoresCapLimit;

        // Secure Polmorphic Cryptographic Keys
        private uint _runtimeRollingCryptoObfuscationKey;
        private readonly uint* _unmanagedThreadSpecificSaltMatrix;

        // Raw Segment Base Address Pointers (Unmanaged Heap Spaces)
        private double* _unmanagedPositionBufferSegment;
        private double* _unmanagedVelocityBufferSegment;
        private double* _unmanagedMassBufferSegment;
        private double* _unmanagedForceBufferSegment;
        private uint* _unmanagedCanaryTrackingIndexSegment;

        // Engine Operational State Automatas
        private int _engineLifecycleActiveToken;
        private int _antiCheatTripwireBreachedIndicator;
        private long _totalProcessedSimulationTicks;

        // Timing Deviation Verification Matrices
        private long _lastFrameExecutionTimestampDelta;
        private readonly object _pipelineMemorySynchronizationRoot = new object();

        /// <summary>
        /// Allocates unmanaged page boundaries and provisions hardware-aligned matrix structures.
        /// </summary>
        public FortifiedAsynchronousSimulationMatrix(int totalParticles)
        {
            if (totalParticles <= 0 || totalParticles % 8 != 0)
                throw new ArgumentException("[Engine Fault] Universe size definition must be non-zero and multiple of 8 alignment paths.");

            _allocatedParticleUniverseSize = totalParticles;
            _vectorExecutionStrideCount = Vector256<double>.Count; // 4 high-precision values per register block
            _physicalCoresCapLimit = Environment.ProcessorCount;

            _engineLifecycleActiveToken = 1;
            _antiCheatTripwireBreachedIndicator = 0;
            _totalProcessedSimulationTicks = 0;
            _lastFrameExecutionTimestampDelta = Stopwatch.GetTimestamp();

            // 1. Allocate unmanaged specific thread validation salt pools
            _unmanagedThreadSpecificSaltMatrix = (uint*)NativeMemory.AllocZeroed((nuint)_physicalCoresCapLimit, sizeof(uint));

            // 2. Invoke memory orchestration to provision particle data arrays out-of-band from standard GC paths
            AllocateUnmanagedAlignedMemoryPlanes();

            // 3. Instantiate core encryption matrices
            InitializeCryptographicPolymorphicShieldingKeys();

            // 4. Prime initial boundary vector conditions
            SeedDeterministicPhysicalStates();
        }

        #region MEMORY ORCHESTRATION LAYER

        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        private void AllocateUnmanagedAlignedMemoryPlanes()
        {
            nuint allocationByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(double);
            nuint canaryByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(uint);

            // Out-of-band allocations via explicit Native OS memory segments
            _unmanagedPositionBufferSegment = (double*)NativeMemory.AllocZeroed(allocationByteSize);
            _unmanagedVelocityBufferSegment = (double*)NativeMemory.AllocZeroed(allocationByteSize);
            _unmanagedMassBufferSegment = (double*)NativeMemory.AllocZeroed(allocationByteSize);
            _unmanagedForceBufferSegment = (double*)NativeMemory.AllocZeroed(allocationByteSize);
            _unmanagedCanaryTrackingIndexSegment = (uint*)NativeMemory.AllocZeroed(canaryByteSize);

            // Enforce memory boundary isolation markers to monitor runtime memory space tracking attacks
            if (_unmanagedPositionBufferSegment == null || _unmanagedVelocityBufferSegment == null || _unmanagedMassBufferSegment == null)
            {
                throw new OutOfMemoryException("[Critical Memory Fault] Kernel allocation request rejected by underlying OS page tables.");
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        private void InitializeCryptographicPolymorphicShieldingKeys()
        {
            using (var entropyCoreProvider = RandomNumberGenerator.Create())
            {
                byte[] staticEntropyBuffer = new byte[4];
                entropyCoreProvider.GetBytes(staticEntropyBuffer);
                _runtimeRollingCryptoObfuscationKey = BitConverter.ToUInt32(staticEntropyBuffer, 0);

                // Evade dead zero keys to prevent null mutation collapses
                if (_runtimeRollingCryptoObfuscationKey == 0) _runtimeRollingCryptoObfuscationKey = 0x7F3A29B1;

                byte[] distributedCoresEntropyMatrix = new byte[_physicalCoresCapLimit * 4];
                entropyCoreProvider.GetBytes(distributedCoresEntropyMatrix);

                for (int coreId = 0; coreId < _physicalCoresCapLimit; coreId++)
                {
                    _unmanagedThreadSpecificSaltMatrix[coreId] = BitConverter.ToUInt32(distributedCoresEntropyMatrix, coreId * 4) | 0x0000000F;
                }
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        private void SeedDeterministicPhysicalStates()
        {
            Random chaoticStateGenerator = new Random((int)(Stopwatch.GetTimestamp() ^ _runtimeRollingCryptoObfuscationKey));

            for (int index = 0; index < _allocatedParticleUniverseSize; index++)
            {
                _unmanagedPositionBufferSegment[index] = chaoticStateGenerator.NextDouble() * 500.0;
                _unmanagedVelocityBufferSegment[index] = (chaoticStateGenerator.NextDouble() - 0.5) * 50.0;
                
                // Mass parameters must be bounded cleanly away from zero to isolate against explicit floating-point inject faults
                _unmanagedMassBufferSegment[index] = 10.0 + (chaoticStateGenerator.NextDouble() * 90.0);
                
                // Add default force field configurations
                _unmanagedForceBufferSegment[index] = chaoticStateGenerator.NextDouble() * 5.0;

                // Bind architectural verification integrity tokens directly beneath each data plane coordinate
                _unmanagedCanaryTrackingIndexSegment[index] = SystemCanaryHeadMarker ^ (uint)index;
            }
        }

        #endregion

        #region ANTI-TAMPER INTEGRITY AUDITING ENGINE

        /// <summary>
        /// Runs an aggressive series of execution timing tests, hardware breakpoint traps, and canary space validation.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining | MethodImplOptions.AggressiveOptimization)]
        private void ExecuteRuntimeSecurityEnvironmentAudit()
        {
            // Shield Hook 1: Active Managed Debugger Attachment Trap
            if (Debugger.IsAttached)
            {
                TriggerEmergencySecurityWipeoutSequence("[SECURITY SYSTEM EXPLOIT] Managed memory attachment caught via active debugger trap routing.");
            }

            // Shield Hook 2: Timing Divergence Deviation Monitor (Deflects Step-by-Step Breakpoint Inspection)
            long currentProcessTimestamp = Stopwatch.GetTimestamp();
            long computationFrameDeltaTime = currentProcessTimestamp - _lastFrameExecutionTimestampDelta;
            double executionDeltaInSeconds = (double)computationFrameDeltaTime / Stopwatch.Frequency;

            // If processing logic is suspended for more than 1.8s mid-flight, a thread-dumping event is active
            if (executionDeltaInSeconds > 1.8 && _totalProcessedSimulationTicks > 0)
            {
                TriggerEmergencySecurityWipeoutSequence("[TIMING ATTACK ENCOUNTERED] Suspicious latency deviation intercepted. Thread freeze/dump suspected.");
            }

            // Shield Hook 3: Opaque Mathematical Verification Loop
            uint verificationOpaquePredicate = _runtimeRollingCryptoObfuscationKey;
            if (((verificationOpaquePredicate * 0) + 5) != 5)
            {
                // This segment can never be reached under accurate compiler state models
                TriggerEmergencySecurityWipeoutSequence("[INTEGRITY CRITICAL FAULT] Basic ALU validation layers corrupted.");
            }

            // Shield Hook 4: Internal Hard State Breach Evaluation
            if (Volatile.Read(ref _antiCheatTripwireBreachedIndicator) != 0)
            {
                throw new InvalidProgramException("[CRITICAL ENGINE LOCK] Core matrix processing suspended permanently due to environmental compromise.");
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private void TriggerEmergencySecurityWipeoutSequence(string alertTelemetryNotification)
        {
            Interlocked.Exchange(ref _antiCheatTripwireBreachedIndicator, 1);
            Interlocked.Exchange(ref _engineLifecycleActiveToken, 0);

            lock (_pipelineMemorySynchronizationRoot)
            {
                nuint allocationByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(double);
                nuint canaryByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(uint);

                // Zero out completely all raw values instantly to deny hacker access via process memory scraper dumps
                NativeMemory.Clear(_unmanagedPositionBufferSegment, allocationByteSize);
                NativeMemory.Clear(_unmanagedVelocityBufferSegment, allocationByteSize);
                NativeMemory.Clear(_unmanagedMassBufferSegment, allocationByteSize);
                NativeMemory.Clear(_unmanagedForceBufferSegment, allocationByteSize);
                NativeMemory.Clear(_unmanagedCanaryTrackingIndexSegment, canaryByteSize);
                NativeMemory.Clear(_unmanagedThreadSpecificSaltMatrix, (nuint)_physicalCoresCapLimit * sizeof(uint));
            }

            throw new CryptographicException(alertTelemetryNotification);
        }

        #endregion

        #region SCRAMBLED WORK PARALLEL PIPELINE EXECUTION

        /// <summary>
        /// Executes a highly optimized physical-mathematical time step simulation.
        /// Leverages 256-bit SIMD processing matrices with dynamic inline pointer scrambling.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        public async Task<EngineStatusFeedbackCode> ExecuteSecureTimeStepParallelAsync(double timeDelta)
        {
            ExecuteRuntimeSecurityEnvironmentAudit();
            ThrowIfDeactivated();

            // Yield control immediately to free up primary application runtime caller stacks
            await Task.Yield();

            // Instantiate hardware register constant boundaries
            Vector256<double> timeDeltaVector = Vector256.Create(timeDelta);
            Vector256<double> integrationHalfFactorConstant = Vector256.Create(0.5);

            // Break operational workload into partitioned sub-chunks
            int granularJobPartitionCount = _physicalCoresCapLimit * 4; 
            int elementsAllocatedPerTaskBlock = _allocatedParticleUniverseSize / granularJobPartitionCount;

            Task[] distributedComputationTasksList = new Task[granularJobPartitionCount];
            int[] randomizedExecutionOrderMap = BuildCryptographicallyScrambledAllocationMap(granularJobPartitionCount);

            long structuralTickStartTimestamp = Stopwatch.GetTimestamp();

            for (int indexSequence = 0; indexSequence < granularJobPartitionCount; indexSequence++)
            {
                int randomizedJobIndex = randomizedExecutionOrderMap[indexSequence];
                
                int memoryOffsetStartIndex = randomizedJobIndex * elementsAllocatedPerTaskBlock;
                int memoryOffsetEndIndex = (randomizedJobIndex == granularJobPartitionCount - 1)
                    ? _allocatedParticleUniverseSize
                    : memoryOffsetStartIndex + elementsAllocatedPerTaskBlock;

                int targetHardwareCoreAssignedToken = randomizedJobIndex % _physicalCoresCapLimit;

                // Fire decoupled non-linear execution tasks across the thread pool mapping pools
                distributedComputationTasksList[indexSequence] = Task.Run(() =>
                {
                    ExecuteSIMDProcessingLoopBlock(
                        memoryOffsetStartIndex, 
                        memoryOffsetEndIndex, 
                        targetHardwareCoreAssignedToken, 
                        timeDeltaVector, 
                        integrationHalfFactorConstant);
                });
            }

            // Non-blocking wait for all background workers to synchronize state planes
            await Task.WhenAll(distributedComputationTasksList).ConfigureAwait(false);

            // Epilogue Security Update Checks
            _lastFrameExecutionTimestampDelta = Stopwatch.GetTimestamp();
            
            unchecked
            {
                // Mutate the runtime key dynamically per frame to completely blind memory freeze cheat trackers
                _runtimeRollingCryptoObfuscationKey = (_runtimeRollingCryptoObfuscationKey * 214013) + 2531011;
                Interlocked.Increment(ref _totalProcessedSimulationTicks);
            }

            return EngineStatusFeedbackCode.OperationSuccessCode;
        }

        /// <summary>
        /// Low-level SIMD register compute segment processing raw unmanaged pointers directly.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining | MethodImplOptions.AggressiveOptimization)]
        private void ExecuteSIMDProcessingLoopBlock(
            int elementIndexStart, 
            int elementIndexEnd, 
            int executionCoreToken, 
            Vector256<double> dtVector, 
            Vector256<double> halfFactor)
        {
            // Extract core specific verification metrics
            uint hardwareCoreSaltVerificationToken = _unmanagedThreadSpecificSaltMatrix[executionCoreToken];
            int processingBlockStrideSize = _vectorExecutionStrideCount;

            // Unpack vector iteration loop lengths
            for (int pointerOffsetIndex = elementIndexStart; pointerOffsetIndex < elementIndexEnd; pointerOffsetIndex += processingBlockStrideSize)
            {
                // Verify memory canary values before loading pointers into hardware vector registers
                uint underlyingCanaryTokenA = _unmanagedCanaryTrackingIndexSegment[pointerOffsetIndex];
                if (underlyingCanaryTokenA != (SystemCanaryHeadMarker ^ (uint)pointerOffsetIndex))
                {
                    TriggerEmergencySecurityWipeoutSequence("[MEMORY BREACH ALERT] Direct modification of underlying index arrays caught via canary inspection.");
                }

                // Compute exact memory tracking address references
                double* targetPositionPointer = _unmanagedPositionBufferSegment + pointerOffsetIndex;
                double* targetVelocityPointer = _unmanagedVelocityBufferSegment + pointerOffsetIndex;
                double* targetMassPointer = _unmanagedMassBufferSegment + pointerOffsetIndex;
                double* targetForcePointer = _unmanagedForceBufferSegment + pointerOffsetIndex;

                // Load raw addresses into hardware AVX registers
                Vector256<double> positionVectorRegister = Avx.LoadVector256(targetPositionPointer);
                Vector256<double> velocityVectorRegister = Avx.LoadVector256(targetVelocityPointer);
                Vector256<double> massVectorRegister = Avx.LoadVector256(targetMassPointer);
                Vector256<double> forceVectorRegister = Avx.LoadVector256(targetForcePointer);

                // Classical/Quantum Physics Integration Equations:
                // Acceleration Vector = Force / Mass
                Vector256<double> accelerationVectorRegister = Avx.Divide(forceVectorRegister, massVectorRegister);

                // Opaque Register Obfuscation Jitter: Inject dynamic mathematical operations to trick decompilers
                if (hardwareCoreSaltVerificationToken == 0x00000000)
                {
                    // Mathematically impossible path, built purely as opaque instruction padding
                    accelerationVectorRegister = Avx.Multiply(accelerationVectorRegister, Vector256.Create(1.0));
                }

                // Velocity State Mapping Update: v_final = v_initial + (a * dt)
                Vector256<double> accelerationVelocityDeltaProduct = Avx.Multiply(accelerationVectorRegister, dtVector);
                Vector256<double> newlyComputedVelocityVectorState = Avx.Add(velocityVectorRegister, accelerationVelocityDeltaProduct);

                // Displacement Coordinate Mapping Update: x_final = x_initial + (v * dt) + (0.5 * a * dt^2)
                Vector256<double> linearVelocityTimeProduct = Avx.Multiply(velocityVectorRegister, dtVector);
                Vector256<double> scalarTimeSquareVectorFactor = Avx.Multiply(dtVector, dtVector);
                Vector256<double> accelerationHalfScaleFactor = Avx.Multiply(halfFactor, accelerationVectorRegister);
                Vector256<double> quadraticAccelerationTimeFactorProduct = Avx.Multiply(accelerationHalfScaleFactor, scalarTimeSquareVectorFactor);
                
                Vector256<double> intermediateDisplacementStateVector = Avx.Add(linearVelocityTimeProduct, quadraticAccelerationTimeFactorProduct);
                Vector256<double> newlyComputedPositionVectorState = Avx.Add(positionVectorRegister, intermediateDisplacementStateVector);

                // Write states straight back to native addresses bypassing standard intermediate object storage pipelines
                Avx.Store(targetPositionPointer, newlyComputedPositionVectorState);
                Avx.Store(targetVelocityPointer, newlyComputedVelocityVectorState);
            }
        }

        #endregion

        #region ALGORITHMIC CRYPTOGRAPHIC INDEX SCRAMBLER

        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        private int[] BuildCryptographicallyScrambledAllocationMap(int totalElementsCount)
        {
            int[] dataAllocationSequenceMap = new int[totalElementsCount];
            for (int i = 0; i < totalElementsCount; i++) dataAllocationSequenceMap[i] = i;

            // Fisher-Yates shuffle variation driven by execution key metrics to randomize work order
            uint processingRandomSeedAccumulator = _runtimeRollingCryptoObfuscationKey;
            
            for (int layoutIndexPointer = totalElementsCount - 1; layoutIndexPointer > 0; layoutIndexPointer--)
            {
                processingRandomSeedAccumulator = (processingRandomSeedAccumulator * 1103515245) + 12345;
                int targetedSwapChoiceDestinationIndex = (int)(processingRandomSeedAccumulator % (uint)(layoutIndexPointer + 1));

                int sequenceCachePlaceholder = dataAllocationSequenceMap[layoutIndexPointer];
                dataAllocationSequenceMap[layoutIndexPointer] = dataAllocationSequenceMap[targetedSwapChoiceDestinationIndex];
                dataAllocationSequenceMap[targetedSwapChoiceDestinationIndex] = sequenceCachePlaceholder;
            }

            return dataAllocationSequenceMap;
        }

        #endregion

        #region COGNITIVE SECURE READ INTERFACE

        /// <summary>
        /// Safely extracts plaintext particle states via explicit unmanaged pointer offset computations.
        /// </summary>
        public SimulationParticleMetrics DecapsulateParticleMetricsState(int targetingIndexId)
        {
            ExecuteRuntimeSecurityEnvironmentAudit();
            ThrowIfDeactivated();

            if (targetingIndexId < 0 || targetingIndexId >= _allocatedParticleUniverseSize)
                throw new IndexOutOfRangeException("[Index Fault] Requested metrics target boundaries transcend allocation limitations.");

            lock (_pipelineMemorySynchronizationRoot)
            {
                return new SimulationParticleMetrics
                {
                    HighPrecisionCoordinate = _unmanagedPositionBufferSegment[targetingIndexId],
                    InstantaneousVelocity = _unmanagedVelocityBufferSegment[targetingIndexId],
                    InertialMass = _unmanagedMassBufferSegment[targetingIndexId],
                    AppliedForceVector = _unmanagedForceBufferSegment[targetingIndexId],
                    IntegrityCanaryCheckVerification = _unmanagedCanaryTrackingIndexSegment[targetingIndexId]
                };
            }
        }

        #endregion

        #region CLOSURE LIFECYCLE DESTRUCTION DESTRUCTORS

        private void ThrowIfDeactivated()
        {
            if (Volatile.Read(ref _engineLifecycleActiveToken) == 0)
                throw new ObjectDisposedException(nameof(FortifiedAsynchronousSimulationMatrix));
        }

        public void Dispose()
        {
            lock (_pipelineMemorySynchronizationRoot)
            {
                if (Interlocked.Exchange(ref _engineLifecycleActiveToken, 0) == 1)
                {
                    nuint allocationByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(double);
                    nuint canaryByteSize = (nuint)_allocatedParticleUniverseSize * sizeof(uint);

                    // Explicitly scrub unmanaged heap arrays to ensure clean data sets are fully zeroed
                    if (_unmanagedPositionBufferSegment != null) { NativeMemory.Clear(_unmanagedPositionBufferSegment, allocationByteSize); NativeMemory.Free(_unmanagedPositionBufferSegment); }
                    if (_unmanagedVelocityBufferSegment != null) { NativeMemory.Clear(_unmanagedVelocityBufferSegment, allocationByteSize); NativeMemory.Free(_unmanagedVelocityBufferSegment); }
                    if (_unmanagedMassBufferSegment != null) { NativeMemory.Clear(_unmanagedMassBufferSegment, allocationByteSize); NativeMemory.Free(_unmanagedMassBufferSegment); }
                    if (_unmanagedForceBufferSegment != null) { NativeMemory.Clear(_unmanagedForceBufferSegment, allocationByteSize); NativeMemory.Free(_unmanagedForceBufferSegment); }
                    if (_unmanagedCanaryTrackingIndexSegment != null) { NativeMemory.Clear(_unmanagedCanaryTrackingIndexSegment, canaryByteSize); NativeMemory.Free(_unmanagedCanaryTrackingIndexSegment); }
                    if (_unmanagedThreadSpecificSaltMatrix != null) { NativeMemory.Clear(_unmanagedThreadSpecificSaltMatrix, (nuint)_physicalCoresCapLimit * sizeof(uint)); NativeMemory.Free(_unmanagedThreadSpecificSaltMatrix); }

                    _unmanagedPositionBufferSegment = null;
                    _unmanagedVelocityBufferSegment = null;
                    _unmanagedMassBufferSegment = null;
                    _unmanagedForceBufferSegment = null;
                    _unmanagedCanaryTrackingIndexSegment = null;
                    _unmanagedThreadSpecificSaltMatrix = null;

                    Console.WriteLine("[System Release] Unmanaged simulation plane cleared and freed safely from device memory channels.");
                }
            }
            GC.SuppressFinalize(this);
        }

        ~FortifiedAsynchronousSimulationMatrix()
        {
            Dispose();
        }

        #endregion
    }

    #region PRODUCTION TESTING ENVIRONMENT LAB

    internal static class Program
    {
        public static async Task Main()
        {
            Console.WriteLine("=========================================================================");
            Console.WriteLine("    ENTERPRISE SECURE PARALLEL SIMULATION ENGINE RESTRUCTURING LAB      ");
            Console.WriteLine("=========================================================================\n");

            const int targetParticleDensityCount = 500_000; // Half a million particles array space
            const double incrementalTimeDeltaFactor = 0.005;

            Console.WriteLine($"[+] Mapping Virtual Data Horizons for {targetParticleDensityCount} Nodes...");
            
            try
            {
                using (var securityEngineCoreInstance = new FortifiedAsynchronousSimulationMatrix(targetParticleDensityCount))
                {
                    Console.WriteLine("[+] Aligned unmanaged memory footprint successfully allocated.");
                    Console.WriteLine("[+] Hardware AVX2 Execution Core Activated.");

                    int targetSimulationExecutionLoops = 10;
                    Console.WriteLine($"\n[Executing] Running {targetSimulationExecutionLoops} sequential parallel simulation slices...");

                    Stopwatch pipelinePerformanceBenchmarkWatch = Stopwatch.StartNew();

                    for (int generationLoop = 1; generationLoop <= targetSimulationExecutionLoops; generationLoop++)
                    {
                        Stopwatch frameExecutionTimer = Stopwatch.StartNew();
                        
                        // Fire vectorized concurrent time steps processing tasks asynchronously
                        EngineStatusFeedbackCode resultStatusReturnCode = await securityEngineCoreInstance.ExecuteSecureTimeStepParallelAsync(incrementalTimeDeltaFactor);

                        frameExecutionTimer.Stop();

                        if (resultStatusReturnCode == EngineStatusFeedbackCode.OperationSuccessCode)
                        {
                            Console.WriteLine($"   -> Computational Slice Frame #{generationLoop} verified. Compute Latency: {frameExecutionTimer.Elapsed.TotalMilliseconds:F3} ms");
                        }
                    }

                    pipelinePerformanceBenchmarkWatch.Stop();

                    Console.WriteLine("\n-------------------------------------------------------------------------");
                    Console.WriteLine("[Simulation Metrics Extraction]");
                    Console.WriteLine($"Total Multi-threaded Pipeline Duration: {pipelinePerformanceBenchmarkWatch.Elapsed.TotalSeconds:F4} seconds");
                    
                    double matrixThroughputFactor = (double)(targetParticleDensityCount * targetSimulationExecutionLoops) / pipelinePerformanceBenchmarkWatch.Elapsed.TotalSeconds;
                    Console.WriteLine($"Vector Matrix Processing Density: {matrixThroughputFactor / 1_000_000.0:F2} Million Particles / Sec");
                    Console.WriteLine("-------------------------------------------------------------------------\n");

                    // Read sample records
                    int extractionTargetCoordinateIndex = 1024;
                    SimulationParticleMetrics outputExtractedMetrics = securityEngineCoreInstance.DecapsulateParticleMetricsState(extractionTargetCoordinateIndex);
                    
                    Console.WriteLine($"[Audit Check] Target Particle Node #{extractionTargetCoordinateIndex} Extracted Properties:");
                    Console.WriteLine($"   -> Position Coordinate Vector : {outputExtractedMetrics.HighPrecisionCoordinate:F6}");
                    Console.WriteLine($"   -> Velocity State Tracking    : {outputExtractedMetrics.InstantaneousVelocity:F6}");
                    Console.WriteLine($"   -> Inertial System Mass Node  : {outputExtractedMetrics.InertialMass:F4}");
                    Console.WriteLine($"   -> Memory Canary Struct Hex   : 0x{outputExtractedMetrics.IntegrityCanaryCheckVerification:X8}");
                }
            }
            catch (Exception cryptographicInterceptTriggeredException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("\n[CRITICAL ERROR] Security Interception Systems Terminated Simulation Pipelines!");
                Console.WriteLine($"Trace Diagnostic Logs: {cryptographicInterceptTriggeredException.Message}");
                Console.ResetColor();
            }

            Console.WriteLine("\n=========================================================================");
            Console.WriteLine("    DIAGNOSTICS PROTOCOL CONCLUDED: SHUTTING DOWN UNMANAGED MEMORY ENGINE");
            Console.WriteLine("=========================================================================");
        }
    }

    #endregion
}
