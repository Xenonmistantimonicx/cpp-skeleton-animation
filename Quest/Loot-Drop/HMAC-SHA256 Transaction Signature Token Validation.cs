using System;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Runtime.CompilerServices;
using System.Collections.Concurrent;

namespace SecureEnterpriseNetwork.Cryptography.Verification
{
    #region CORE ARCHITECTURE STRUCTS AND DATA SCHEMAS

    /// <summary>
    /// Represents a high-integrity transaction packet with deterministic property structural models.
    /// </summary>
    public sealed class TransactionPayload
    {
        public string TransactionId { get; set; } = string.Empty;
        public string AccountId { get; set; } = string.Empty;
        public long ItemCatalogId { get; set; }
        public decimal MicrotransactionAmount { get; set; }
        public string CurrencyToken { get; set; } = "USD";
        public long UnixEpochTimestamp { get; set; }
        public string CryptographicNonce { get; set; } = string.Empty;

        /// <summary>
        /// Converts the explicit schema object into a strictly deterministic canonical sorted text payload.
        /// This eliminates multi-platform JSON field sorting layout variances entirely.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        public string BuildCanonicalStringStream()
        {
            // Lexicographically ordered property index mapping pairs
            var sortedDictionary = new SortedDictionary<string, string>(StringComparer.Ordinal)
            {
                { "account_id", AccountId.Trim() },
                { "currency_token", CurrencyToken.ToUpperInvariant() },
                { "item_catalog_id", ItemCatalogId.ToString() },
                { "microtransaction_amount", MicrotransactionAmount.ToString("F4") }, // Force precise decimal string mapping
                { "nonce_vector", CryptographicNonce },
                { "timestamp_epoch", UnixEpochTimestamp.ToString() },
                { "transaction_id", TransactionId.Trim() }
            };

            var stringBuilder = new StringBuilder();
            foreach (var keyValuePair in sortedDictionary)
            {
                stringBuilder.Append(keyValuePair.Key).Append(':').Append(keyValuePair.Value).Append(';');
            }

            return stringBuilder.ToString();
        }
    }

    /// <summary>
    /// Holds the cryptographic validation metrics results for transaction analysis audits.
    /// </summary>
    public enum ValidationResultCode
    {
        AuthorizedSuccess = 0,
        CryptographicSignatureMismatch = 1,
        ReplayAttackDetected = 2,
        TransactionExpired = 3,
        MalformedStructuralPayload = 4,
        RevokedSecretKeyIdentifier = 5
    }

    public sealed class TokenValidationReport
    {
        public ValidationResultCode ResultCode { get; internal set; }
        public string InternalTelemetryLog { get; internal set; } = string.Empty;
        public double ProcessingLatencyMilliseconds { get; internal set; }
    }

    #endregion

    /// <summary>
    /// High-throughput, memory-optimized, thread-safe HMAC-SHA256 Transaction Authentication Pipeline.
    /// </summary>
    public sealed class TransactionSignatureValidator : IDisposable
    {
        private readonly byte[] _cryptographicSecretKeyBuffer;
        private readonly long _allowedTimestampDriftSeconds;
        private bool _isDisposed;

        // In-memory sliding ledger cache tracking processed unique nonces to fully deflect replay vectors
        private static readonly ConcurrentDictionary<string, DateTime> NonceBlacklistRegistry = new ConcurrentDictionary<string, DateTime>();
        private static readonly object SyncRoot = new object();

        /// <summary>
        /// Instantiates the cryptographic core controller engine.
        /// </summary>
        /// <param name="secretKey">The root signature authentication master key.</param>
        /// <param name="allowedDriftSeconds">Maximum age tolerance gap before rejecting transaction states.</param>
        public TransactionSignatureValidator(byte[] secretKey, long allowedDriftSeconds = 300)
        {
            if (secretKey == null || secretKey.Length == 0)
                throw new ArgumentException("[Crypto Core Error] Invalid signature authentication master key initialization buffer.");

            // Defensive deep copy of key array to isolate memory buffers from outer modifications
            _cryptographicSecretKeyBuffer = new byte[secretKey.Length];
            Buffer.BlockCopy(secretKey, 0, _cryptographicSecretKeyBuffer, 0, secretKey.Length);
            
            _allowedTimestampDriftSeconds = allowedDriftSeconds;
        }

        #region CRYPTOGRAPHIC VALIDATION PIPELINE

        /// <summary>
        /// Performs deep verification over standard transaction payloads by checking validation criteria.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveOptimization)]
        public TokenValidationReport VerifyTransactionToken(TransactionPayload payload, string incomingSignatureBase64)
        {
            ThrowIfDisposed();
            var highPrecisionStopwatch = System.Diagnostics.Stopwatch.StartNew();

            if (payload == null || string.IsNullOrWhiteSpace(incomingSignatureBase64))
            {
                return CreateReport(ValidationResultCode.MalformedStructuralPayload, "Empty or structurally invalid packet objects incoming.", highPrecisionStopwatch);
            }

            // -------------------------------------------------------------
            // MODULE 1: TIMESTAMP DRIFT WINDOW VALIDATION
            // -------------------------------------------------------------
            long currentUnixTime = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            long computationDriftDelta = Math.Abs(currentUnixTime - payload.UnixEpochTimestamp);

            if (computationDriftDelta > _allowedTimestampDriftSeconds)
            {
                return CreateReport(ValidationResultCode.TransactionExpired, $"Out of bounds timestamp drift detected. Delta Offset: {computationDriftDelta}s.", highPrecisionStopwatch);
            }

            // -------------------------------------------------------------
            // MODULE 2: NONCE TRACKING RADAR (REPLAY DEFLECTION)
            // -------------------------------------------------------------
            // Clean up old expired nonces inside the static memory footprints registry map to prevent scale leaking
            PruneStaleNonces(currentUnixTime);

            if (!NonceBlacklistRegistry.TryAdd(payload.CryptographicNonce, DateTime.UtcNow))
            {
                return CreateReport(ValidationResultCode.ReplayAttackDetected, $"Duplicate execution sequence detected! Nonce token '{payload.CryptographicNonce}' already processed inside the cache database.", highPrecisionStopwatch);
            }

            // -------------------------------------------------------------
            // MODULE 3: CANONICAL STREAM SIGNATURE RE-COMPUTATION
            // -------------------------------------------------------------
            string canonicalDataStream = payload.BuildCanonicalStringStream();
            byte[] processedInputBytes = Encoding.UTF8.GetBytes(canonicalDataStream);
            byte[] locallyGeneratedSignatureHash;

            // Direct isolated instantiation of high-performance HMAC implementation primitive block
            using (var hmacInstance = new HMACSHA256(_cryptographicSecretKeyBuffer))
            {
                locallyGeneratedSignatureHash = hmacInstance.ComputeHash(processedInputBytes);
            }

            // Convert external user tracking data from Base64 configuration back to binary representation arrays
            byte[] incomingSignatureBuffer;
            try
            {
                incomingSignatureBuffer = Convert.FromBase64String(incomingSignatureBase64);
            }
            catch (FormatException)
            {
                return CreateReport(ValidationResultCode.MalformedStructuralPayload, "Incoming transaction token signature format is not valid Base64 notation stream arrays.", highPrecisionStopwatch);
            }

            // -------------------------------------------------------------
            // MODULE 4: CONSTANT-TIME TIMING ATTACK CRYPTO ANALYSIS
            // -------------------------------------------------------------
            bool isSignatureValid = FixedTimeSecureBufferComparison(locallyGeneratedSignatureHash, incomingSignatureBuffer);

            if (!isSignatureValid)
            {
                return CreateReport(ValidationResultCode.CryptographicSignatureMismatch, "Calculated checksum does not match incoming digital signature tokens.", highPrecisionStopwatch);
            }

            return CreateReport(ValidationResultCode.AuthorizedSuccess, "Transaction cryptographic token identity checks fully validated.", highPrecisionStopwatch);
        }

        #endregion

        #region PROTECTED CRYPTOGRAPHIC MATHEMATICAL LAYER

        /// <summary>
        /// Compares two byte arrays in constant time using bitwise XOR accumulation pipelines.
        /// Prevents sub-millisecond execution runtime side-channel observation leaks.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static bool FixedTimeSecureBufferComparison(byte[] firstBuffer, byte[] secondBuffer)
        {
            if (firstBuffer == null || secondBuffer == null) return false;
            if (firstBuffer.Length != secondBuffer.Length) return false;

            int bitwiseAccumulatorResult = 0;
            int totalBufferLength = firstBuffer.Length;

            for (int index = 0; index < totalBufferLength; index++)
            {
                // Bitwise XOR yields 0 if values match exactly. Any variance permanently trips the accumulator bits.
                bitwiseAccumulatorResult |= firstBuffer[index] ^ secondBuffer[index];
            }

            return bitwiseAccumulatorResult == 0;
        }

        /// <summary>
        /// Garbage collector routine to prune the central processing tables of past historical nonces.
        /// </summary>
        private void PruneStaleNonces(long currentUnixTime)
        {
            lock (SyncRoot)
            {
                var targetThresholdDateTime = DateTime.UtcNow.AddSeconds(-_allowedTimestampDriftSeconds);
                var expiredKeys = NonceBlacklistRegistry
                    .Where(pair => pair.Value < targetThresholdDateTime)
                    .Select(pair => pair.Key)
                    .ToList();

                foreach (var expiredKey in expiredKeys)
                {
                    NonceBlacklistRegistry.TryRemove(expiredKey, out _);
                }
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private TokenValidationReport CreateReport(ValidationResultCode status, string notes, System.Diagnostics.Stopwatch watch)
        {
            watch.Stop();
            return new TokenValidationReport
            {
                ResultCode = status,
                InternalTelemetryLog = notes,
                ProcessingLatencyMilliseconds = watch.Elapsed.TotalMilliseconds
            };
        }

        #endregion

        #region SYSTEM LIFECYCLE MANAGEMENT IMPLEMENTATION

        private void ThrowIfDisposed()
        {
            if (_isDisposed) throw new ObjectDisposedException(nameof(TransactionSignatureValidator));
        }

        public void Dispose()
        {
            if (_isDisposed) return;
            lock (SyncRoot)
            {
                if (_isDisposed) return;
                
                // Secure clear: Zero out volatile core key arrays prior to system release recycling loops
                if (_cryptographicSecretKeyBuffer != null)
                {
                    Array.Clear(_cryptographicSecretKeyBuffer, 0, _cryptographicSecretKeyBuffer.Length);
                }
                _isDisposed = true;
            }
        }

        #endregion
    }

    #region SIMULATION ENVIRONMENT PRODUCTION DIAGNOSTICS

    class Program
    {
        static void Main()
        {
            Console.WriteLine("=========================================================================");
            Console.WriteLine("    ENTERPRISE HMAC-SHA256 SECURE TRANSACTION VERIFICATION SYSTEM LAB   ");
            Console.WriteLine("================================================================*********\n");

            // Define private server communication key vectors
            byte[] serverSecretKeySpace = Encoding.UTF8.GetBytes("SuperSecretRootServerMasterKeyVectorIndex_2026_Core!");

            // Instantiate valid system configuration infrastructure objects
            using (var validatorEngine = new TransactionSignatureValidator(serverSecretKeySpace, allowedDriftSeconds: 60))
            {
                // Setup baseline healthy parameters
                var freshTransactionPacket = new TransactionPayload
                {
                    TransactionId = "TXN-90812490812-X",
                    AccountId = "USER-ALPHA-99",
                    ItemCatalogId = 771029,
                    MicrotransactionAmount = 99.9900m,
                    CurrencyToken = "USD",
                    UnixEpochTimestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
                    CryptographicNonce = Guid.NewGuid().ToString("N")
                };

                // CLIENT SIDE: Simulated Signature Construction Loop
                string clientGeneratedSignatureBase64;
                string clientDataStream = freshTransactionPacket.BuildCanonicalStringStream();
                
                using (var hmacClient = new HMACSHA256(serverSecretKeySpace))
                {
                    byte[] rawClientHash = hmacClient.ComputeHash(Encoding.UTF8.GetBytes(clientDataStream));
                    clientGeneratedSignatureBase64 = Convert.ToBase64String(rawClientHash);
                }

                Console.WriteLine($"[Client Node] Canonical String Matrix generated:\n   -> \"{clientDataStream}\"");
                Console.WriteLine($"[Client Node] Final Computed Signature Token: {clientGeneratedSignatureBase64}\n");

                // =========================================================================
                // LIVE AUDIT EXPERIMENT 1: Processing standard legit validation requests
                // =========================================================================
                Console.WriteLine("[Audit Processing] Launching Verification Step Scenario 1 (Legitimate Request)...");
                var scenarioReport1 = validatorEngine.VerifyTransactionToken(freshTransactionPacket, clientGeneratedSignatureBase64);
                
                Console.WriteLine($" => Status Outcome Code: {scenarioReport1.ResultCode}");
                Console.WriteLine($" => Telemetry Report: {scenarioReport1.InternalTelemetryLog}");
                Console.WriteLine($" => Engine Execution Cost: {scenarioReport1.ProcessingLatencyMilliseconds:F6} ms\n");

                // =========================================================================
                // LIVE AUDIT EXPERIMENT 2: Attempting Replay Attack Vector Interception
                // =========================================================================
                Console.WriteLine("[Audit Processing] Launching Verification Step Scenario 2 (Replay Attack Attempt)...");
                // Resending the exact same structured packet data streams instantly
                var scenarioReport2 = validatorEngine.VerifyTransactionToken(freshTransactionPacket, clientGeneratedSignatureBase64);
                
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($" => Status Outcome Code: {scenarioReport2.ResultCode}");
                Console.WriteLine($" => Telemetry Report: {scenarioReport2.InternalTelemetryLog}");
                Console.ResetColor();
                Console.WriteLine($" => Engine Execution Cost: {scenarioReport2.ProcessingLatencyMilliseconds:F6} ms\n");

                // =========================================================================
                // LIVE AUDIT EXPERIMENT 3: Intercepting Malicious Payload Alterations (Tampering)
                // =========================================================================
                Console.WriteLine("[Audit Processing] Launching Verification Step Scenario 3 (Data Tampering Attack)...");
                
                var tamperedTransactionPacket = new TransactionPayload
                {
                    TransactionId = freshTransactionPacket.TransactionId,
                    AccountId = freshTransactionPacket.AccountId,
                    ItemCatalogId = freshTransactionPacket.ItemCatalogId,
                    MicrotransactionAmount = 99999.9900m, // Evil injector modified the cost from 99.99 to 99999.99!
                    CurrencyToken = freshTransactionPacket.CurrencyToken,
                    UnixEpochTimestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
                    CryptographicNonce = Guid.NewGuid().ToString("N") // Fresh unique nonce to pass replay firewall
                };

                var scenarioReport3 = validatorEngine.VerifyTransactionToken(tamperedTransactionPacket, clientGeneratedSignatureBase64);
                
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($" => Status Outcome Code: {scenarioReport3.ResultCode}");
                Console.WriteLine($" => Telemetry Report: {scenarioReport3.InternalTelemetryLog}");
                Console.ResetColor();
                Console.WriteLine($" => Engine Execution Cost: {scenarioReport3.ProcessingLatencyMilliseconds:F6} ms\n");
            }

            Console.WriteLine("=========================================================================");
            Console.WriteLine("    DIAGNOSTICS PROTOCOL ENDED: SYSTEM PIPELINE STATUS COMPILING SHUTDOWN ");
            Console.WriteLine("=========================================================================");
        }
    }

    #endregion
}
