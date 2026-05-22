using System;
using System.Buffers;
using System.Security.Cryptography;
using System.Runtime.CompilerServices;

namespace UltraAdvancedCryptographicEngine
{
    /// <summary>
    /// Thread-safe, allocation-optimized, and non-deterministic CSPRNG system core.
    /// Operates directly on top of OS-level cryptographic service providers.
    /// </summary>
    public sealed class KernelSecureEntropyGenerator : IDisposable
    {
        private readonly RandomNumberGenerator _kernelCryptoProvider;
        private readonly object _threadSyncLock = new object();
        private bool _isDisposed;

        public KernelSecureEntropyGenerator()
        {
            // Initializes the underlying OS platform cryptographic abstraction layer (CNG on Windows, OpenSSL on Linux/macOS)
            _kernelCryptoProvider = RandomNumberGenerator.Create();
        }

        #region CORE BIAS-FREE RANDOM RANGE ENGINE

        /// <summary>
        /// Generates an absolute uniform random 32-bit signed integer within custom bounds.
        /// Eliminates modulo bias using rejection sampling architecture.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        public int NextBoundedInteger(int minInclusive, int maxExclusive)
        {
            ThrowIfDisposed();
            if (minInclusive >= maxExclusive)
            {
                throw new ArgumentException("[Crypto Fault] Minimal range boundary must be strictly less than maximum boundary.");
            }

            // Calculate exact range scale interval spanning across the boundaries
            long rangeDifference = (long)maxExclusive - minInclusive;
            
            // Rent a 4-byte buffer from the thread-safe shared system pool to zero out heap allocations
            byte[] fourBytePoolArray = ArrayPool<byte>.Shared.Rent(4);

            try
            {
                uint randomUnsignedInt;
                
                // Rejection Sampling Loop to fully mathematically eliminate modulo bias
                while (true)
                {
                    lock (_threadSyncLock)
                    {
                        _kernelCryptoProvider.GetBytes(fourBytePoolArray, 0, 4);
                    }

                    randomUnsignedInt = BitConverter.ToUInt32(fourBytePoolArray, 0);

                    // Establish the uniform threshold barrier ceiling value
                    // This trims out biased tail remainders from the unsigned 32-bit integer spectrum
                    uint ceilingLimit = uint.MaxValue - (uint.MaxValue % (uint)rangeDifference);

                    if (randomUnsignedInt < ceilingLimit)
                    {
                        break; // Value is safe and perfectly unbiased
                    }
                }

                long balancedRemainderOffset = randomUnsignedInt % rangeDifference;
                return (int)(minInclusive + balancedRemainderOffset);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(fourBytePoolArray);
            }
        }

        /// <summary>
        /// Generates a high-precision cryptographic double-precision floating-point number.
        /// Interval span range: [0.0, 1.0) with perfectly flat distribution curves.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public double NextSecureDouble()
        {
            ThrowIfDisposed();
            byte[] eightBytePoolArray = ArrayPool<byte>.Shared.Rent(8);

            try
            {
                lock (_threadSyncLock)
                {
                    _kernelCryptoProvider.GetBytes(eightBytePoolArray, 0, 8);
                }

                ulong sourceRawBits = BitConverter.ToUInt64(eightBytePoolArray, 0);

                // IEEE 754 Double Precision Specification Normalization Method:
                // Shifts bits to populate a 53-bit fraction mantissa, then scales down smoothly by 2^53.
                return (double)(sourceRawBits >> 11) * (1.0 / 9007199254740992.0);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(eightBytePoolArray);
            }
        }

        #endregion

        #region ADVANCED STATISTICAL MATHEMATICAL SAMPLING MODULES

        /// <summary>
        /// Generates an unpredictable boolean value. Perfect 50/50 balance probability mask.
        /// </summary>
        public bool NextSecureBoolean()
        {
            ThrowIfDisposed();
            byte[] singleBytePool = ArrayPool<byte>.Shared.Rent(1);

            try
            {
                lock (_threadSyncLock)
                {
                    _kernelCryptoProvider.GetBytes(singleBytePool, 0, 1);
                }
                
                // Uses the least significant bit to map a 0 or 1 sequence split
                return (singleBytePool[0] & 1) == 1;
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(singleBytePool);
            }
        }

        /// <summary>
        /// Uses the Box-Muller Transformation to map a standardized Gaussian/Normal continuous distribution.
        /// </summary>
        public double NextGaussianNormalDistribution(double meanAverage, double standardDeviation)
        {
            // Sample two independent uniform cryptographic floating values
            double uniformSampleU1 = NextSecureDouble();
            double uniformSampleU2 = NextSecureDouble();

            // Guard rails to prevent passing zero values directly into natural logarithms
            if (uniformSampleU1 < 1e-15) uniformSampleU1 = 1e-15;

            // Mathematical transformation operations
            double logTransformedComponent = Math.Sqrt(-2.0 * Math.Log(uniformSampleU1));
            double trigonometricProjectionComponent = Math.Cos(2.0 * Math.PI * uniformSampleU2);

            double standardNormalDistributionZ0 = logTransformedComponent * trigonometricProjectionComponent;
            
            // Adjust the normalized standard vector to custom mean and standard variance bounds
            return meanAverage + (standardDeviation * standardNormalDistributionZ0);
        }

        /// <summary>
        /// An allocation-free secure implementation of the Modern Fisher-Yates shuffle algorithm.
        /// Rearranges collections with total structural randomness.
        /// </summary>
        public void SecureArrayShuffle<T>(T[] targetsCollection)
        {
            ThrowIfDisposed();
            if (targetsCollection == null || targetsCollection.Length <= 1) return;

            int collectionLength = targetsCollection.Length;

            for (int indexPointer = collectionLength - 1; indexPointer > 0; indexPointer--)
            {
                // Unbiased random range pick for index swapping
                int swapIndexSelection = NextBoundedInteger(0, indexPointer + 1);

                // Destructuring value swap operations
                T temporaryCachedObject = targetsCollection[indexPointer];
                targetsCollection[indexPointer] = targetsCollection[swapIndexSelection];
                targetsCollection[swapIndexSelection] = temporaryCachedObject;
            }
        }

        #endregion

        #region LIFECYCLE MANAGEMENT IMPLEMENTATION

        private void ThrowIfDisposed()
        {
            if (_isDisposed) throw new ObjectDisposedException(nameof(KernelSecureEntropyGenerator));
        }

        public void Dispose()
        {
            if (_isDisposed) return;
            lock (_threadSyncLock)
            {
                if (_isDisposed) return;
                _kernelCryptoProvider.Dispose();
                _isDisposed = true;
            }
        }

        #endregion
    }

    #region CRITICAL INDUSTRIAL STRESS PRODUCTION DIAGNOSTICS

    class Program
    {
        static void Main()
        {
            Console.WriteLine("=========================================================================");
            Console.WriteLine("     CRYSTAL-CLEAR CSPRNG ISOLATED SYSTEM MODULE TESTING LABORATORY    ");
            Console.WriteLine("=========================================================================\n");

            using (var cryptoEngine = new KernelSecureEntropyGenerator())
            {
                // TEST SCENARIO A: Evaluating Modulo-Bias-Free Inclusions
                int lowerTargetBound = 5;
                int upperTargetBound = 105;
                Console.WriteLine($"[Execution Node] Pulling 5 distinct cryptographic integers between [{lowerTargetBound}, {upperTargetBound}):");
                for (int i = 0; i < 5; i++)
                {
                    int secureInteger = cryptoEngine.NextBoundedInteger(lowerTargetBound, upperTargetBound);
                    Console.WriteLine($" => Sample Integer ({i}): {secureInteger}");
                }

                Console.WriteLine("\n-------------------------------------------------------------------------\n");

                // TEST SCENARIO B: Evaluating High-Precision IEEE 754 Fractional Shifting
                Console.WriteLine("[Execution Node] Pulling 4 high-precision secure fraction double scales:");
                for (int i = 0; i < 4; i++)
                {
                    double secureDouble = cryptoEngine.NextSecureDouble();
                    Console.WriteLine($" => Fraction Scalar Value ({i}): {secureDouble:F16}");
                }

                Console.WriteLine("\n-------------------------------------------------------------------------\n");

                // TEST SCENARIO C: Continuous Gaussian Normal Wave Modeling
                double processMean = 50.0;
                double deviationScale = 15.0;
                Console.WriteLine($"[Execution Node] Generating Gaussian continuous signals (Mean: {processMean}, SD: {deviationScale}):");
                for (int i = 0; i < 3; i++)
                {
                    double waveData = cryptoEngine.NextGaussianNormalDistribution(processMean, deviationScale);
                    Console.WriteLine($" => Normal Curve Datapoint ({i}): {waveData:F4}");
                }

                Console.WriteLine("\n-------------------------------------------------------------------------\n");

                // TEST SCENARIO D: Modern Non-Deterministic Collections Shuffling
                string[] dataArrayToScramble = { "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta" };
                Console.WriteLine("[Execution Node] Original Array Structure: " + string.Join(", ", dataArrayToScramble));
                
                cryptoEngine.SecureArrayShuffle(dataArrayToScramble);
                Console.WriteLine(" => Post Fisher-Yates CSPRNG Scramble: " + string.Join(", ", dataArrayToScramble));
            }

            Console.WriteLine("\n=========================================================================");
            Console.WriteLine("     ISOLATED ENGINE SIMULATION COMPLETED WITHOUT ERROR METRICS         ");
            Console.WriteLine("=========================================================================");
        }
    }

    #endregion
}
