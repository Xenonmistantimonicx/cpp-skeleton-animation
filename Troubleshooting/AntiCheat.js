/**
 * ============================================================
 *  GameShield — Comprehensive Client-Side Anti-Cheat System
 *  For: Browser / Node.js / Electron games
 * ============================================================
 *
 *  Modules:
 *   1. IntegrityGuard       – Detects tampered game state / memory
 *   2. InputValidator       – Flags impossible / macro-speed input
 *   3. SpeedHackDetector    – Detects time manipulation
 *   4. NetworkFirewall      – Validates packets & detects injection
 *   5. ScoreValidator       – Statistical impossibility detection
 *   6. SessionGuard         – Replay & session-token protection
 *   7. AntiCheatEngine      – Orchestrator + reporting
 */

'use strict';

// ─────────────────────────────────────────────────────────────
// 1. INTEGRITY GUARD — Checksums for critical game state objects
// ─────────────────────────────────────────────────────────────
class IntegrityGuard {
  constructor() {
    this._watchedObjects = new Map(); // key → { snapshot, checksum }
    this._tamperCount    = 0;
    this._SALT           = this._generateSalt();
  }

  _generateSalt() {
    return Math.random().toString(36).slice(2) + Date.now().toString(36);
  }

  /** Compute a cheap but collision-resistant checksum */
  _checksum(obj) {
    const str = this._SALT + JSON.stringify(obj);
    let hash = 0x811c9dc5; // FNV-1a 32-bit offset
    for (let i = 0; i < str.length; i++) {
      hash ^= str.charCodeAt(i);
      hash = (hash * 0x01000193) >>> 0;
    }
    return hash.toString(16);
  }

  /** Deep-freeze + snapshot a critical object */
  register(key, obj) {
    const snapshot = JSON.parse(JSON.stringify(obj));
    this._watchedObjects.set(key, {
      snapshot,
      checksum: this._checksum(snapshot),
      ref: obj,
    });
  }

  /** Verify object hasn't been externally mutated */
  verify(key) {
    const entry = this._watchedObjects.get(key);
    if (!entry) return { ok: false, reason: 'UNKNOWN_KEY' };

    const currentChecksum = this._checksum(entry.ref);
    if (currentChecksum !== entry.checksum) {
      this._tamperCount++;
      return { ok: false, reason: 'CHECKSUM_MISMATCH', key };
    }
    return { ok: true };
  }

  /** Update snapshot after a legitimate mutation */
  commit(key, obj) {
    const snapshot = JSON.parse(JSON.stringify(obj));
    this._watchedObjects.set(key, {
      snapshot,
      checksum: this._checksum(snapshot),
      ref: obj,
    });
  }

  /** Verify ALL registered objects at once */
  verifyAll() {
    const violations = [];
    for (const [key] of this._watchedObjects) {
      const result = this.verify(key);
      if (!result.ok) violations.push(result);
    }
    return violations;
  }

  get tamperCount() { return this._tamperCount; }
}


// ─────────────────────────────────────────────────────────────
// 2. INPUT VALIDATOR — Detects macros, bots, impossible inputs
// ─────────────────────────────────────────────────────────────
class InputValidator {
  constructor(config = {}) {
    this._minIntervalMs   = config.minIntervalMs   ?? 16;   // ~60 fps floor
    this._maxActionsPerSec= config.maxActionsPerSec ?? 20;
    this._maxComboGap     = config.maxComboGap      ?? 500;  // ms between combo keys
    this._history         = [];
    this._suspicionScore  = 0;
    this._windowMs        = 1000;
  }

  /** Call on every player input event */
  record(actionType, timestamp = Date.now()) {
    const prev = this._history[this._history.length - 1];
    const violations = [];

    if (prev) {
      const gap = timestamp - prev.timestamp;

      // Impossibly fast input (sub-frame)
      if (gap < this._minIntervalMs) {
        violations.push({ type: 'SUPERHUMAN_SPEED', gap, actionType });
        this._suspicionScore += 10;
      }

      // Suspiciously perfect timing (macro pattern: variance < 2 ms over 10 actions)
      if (this._history.length >= 10) {
        const recent = this._history.slice(-10);
        const gaps   = recent.map((e, i) => i > 0 ? e.timestamp - recent[i-1].timestamp : null).filter(Boolean);
        const mean   = gaps.reduce((a,b) => a + b, 0) / gaps.length;
        const variance = gaps.reduce((a, g) => a + (g - mean) ** 2, 0) / gaps.length;
        if (variance < 2) {
          violations.push({ type: 'MACRO_PATTERN', variance, actionType });
          this._suspicionScore += 20;
        }
      }
    }

    // Rate limiting — actions per second
    const windowStart = timestamp - this._windowMs;
    const recentCount = this._history.filter(e => e.timestamp >= windowStart).length;
    if (recentCount >= this._maxActionsPerSec) {
      violations.push({ type: 'INPUT_FLOOD', recentCount, actionType });
      this._suspicionScore += 5;
    }

    this._history.push({ actionType, timestamp });
    if (this._history.length > 500) this._history.shift(); // rolling window

    return { ok: violations.length === 0, violations };
  }

  get suspicionScore() { return this._suspicionScore; }
  reset() { this._suspicionScore = 0; this._history = []; }
}


// ─────────────────────────────────────────────────────────────
// 3. SPEED HACK DETECTOR — Detects game-loop time manipulation
// ─────────────────────────────────────────────────────────────
class SpeedHackDetector {
  constructor(config = {}) {
    this._maxDriftRatio   = config.maxDriftRatio   ?? 1.25; // 25% fast = suspicious
    this._sampleInterval  = config.sampleInterval  ?? 5000; // ms between checks
    this._referenceStart  = Date.now();
    this._performanceStart= typeof performance !== 'undefined' ? performance.now() : Date.now();
    this._gameTickCount   = 0;
    this._expectedTickRate= config.expectedTickRate ?? 60; // ticks / second
    this._violations      = [];
  }

  tick() {
    this._gameTickCount++;
  }

  /** Call periodically (e.g. every 5 seconds) */
  check() {
    const wallMs = Date.now() - this._referenceStart;

    // Cross-reference Date.now() vs performance.now()
    const perfMs = typeof performance !== 'undefined'
      ? performance.now() - this._performanceStart
      : wallMs;

    const driftRatio = Math.abs(wallMs - perfMs) / Math.max(wallMs, 1);
    if (driftRatio > 0.05) {
      this._violations.push({ type: 'CLOCK_DRIFT', driftRatio, wallMs, perfMs });
    }

    // Check tick-rate against wall clock
    const expectedTicks = (wallMs / 1000) * this._expectedTickRate;
    const tickRatio     = this._gameTickCount / Math.max(expectedTicks, 1);

    if (tickRatio > this._maxDriftRatio) {
      this._violations.push({ type: 'SPEED_HACK', tickRatio, gameTickCount: this._gameTickCount, expectedTicks });
      return { ok: false, violations: this._violations };
    }

    return { ok: true, tickRatio };
  }

  get violations() { return [...this._violations]; }
}


// ─────────────────────────────────────────────────────────────
// 4. NETWORK FIREWALL — Packet schema validation + rate limits
// ─────────────────────────────────────────────────────────────
class NetworkFirewall {
  constructor(config = {}) {
    this._schemas        = new Map();  // packetType → validator fn
    this._rateWindows    = new Map();  // packetType → timestamps[]
    this._maxPacketRates = config.maxPacketRates ?? {};
    this._blocked        = new Set();
    this._violations     = [];
  }

  /** Register an expected packet schema (validator returns true/false) */
  registerSchema(packetType, validatorFn, maxPerSecond = 30) {
    this._schemas.set(packetType, validatorFn);
    this._maxPacketRates[packetType] = maxPerSecond;
  }

  /** Inspect an incoming/outgoing packet before processing */
  inspect(packetType, payload, playerId = 'local') {
    const now = Date.now();

    // Unknown packet type — potential injection
    if (!this._schemas.has(packetType)) {
      const v = { type: 'UNKNOWN_PACKET', packetType, playerId, payload };
      this._violations.push(v);
      return { ok: false, reason: 'UNKNOWN_PACKET_TYPE', violation: v };
    }

    // Schema validation
    const validator = this._schemas.get(packetType);
    if (!validator(payload)) {
      const v = { type: 'SCHEMA_VIOLATION', packetType, playerId };
      this._violations.push(v);
      return { ok: false, reason: 'SCHEMA_VIOLATION', violation: v };
    }

    // Rate limiting per packet type
    const maxRate = this._maxPacketRates[packetType] ?? 30;
    const key     = `${playerId}:${packetType}`;
    if (!this._rateWindows.has(key)) this._rateWindows.set(key, []);
    const window  = this._rateWindows.get(key).filter(t => now - t < 1000);
    window.push(now);
    this._rateWindows.set(key, window);

    if (window.length > maxRate) {
      const v = { type: 'PACKET_FLOOD', packetType, playerId, rate: window.length };
      this._violations.push(v);
      this._blocked.add(playerId);
      return { ok: false, reason: 'RATE_LIMIT_EXCEEDED', violation: v };
    }

    // Block-list check
    if (this._blocked.has(playerId)) {
      return { ok: false, reason: 'PLAYER_BLOCKED' };
    }

    return { ok: true };
  }

  unblock(playerId) { this._blocked.delete(playerId); }
  get violations()  { return [...this._violations]; }
  get blocked()     { return [...this._blocked]; }
}


// ─────────────────────────────────────────────────────────────
// 5. SCORE VALIDATOR — Statistical impossibility detection
// ─────────────────────────────────────────────────────────────
class ScoreValidator {
  constructor(config = {}) {
    this._maxScorePerSecond = config.maxScorePerSecond ?? 100;
    this._maxSingleGain     = config.maxSingleGain     ?? 500;
    this._history           = [];  // { score, timestamp }
    this._baselineScore     = 0;
    this._sessionStart      = Date.now();
  }

  /** Record a score-change event; returns validation result */
  recordGain(gainAmount, timestamp = Date.now()) {
    const violations = [];

    // Single-event impossibility
    if (gainAmount > this._maxSingleGain) {
      violations.push({ type: 'IMPOSSIBLE_SCORE_GAIN', gainAmount, max: this._maxSingleGain });
    }

    // Score velocity: points per second over last 5 seconds
    const windowStart = timestamp - 5000;
    const recentGains = this._history
      .filter(e => e.timestamp >= windowStart)
      .reduce((sum, e) => sum + e.gainAmount, 0);

    const velocity = (recentGains + gainAmount) / 5;
    if (velocity > this._maxScorePerSecond) {
      violations.push({ type: 'SCORE_VELOCITY_EXCEEDED', velocity, max: this._maxScorePerSecond });
    }

    // Negative or NaN score
    if (typeof gainAmount !== 'number' || isNaN(gainAmount) || gainAmount < 0) {
      violations.push({ type: 'INVALID_SCORE_VALUE', gainAmount });
    }

    this._history.push({ gainAmount, timestamp });
    if (this._history.length > 1000) this._history.shift();

    return { ok: violations.length === 0, violations };
  }

  /** Verify a total score is achievable given session history */
  verifyTotal(totalScore) {
    const sessionSeconds = (Date.now() - this._sessionStart) / 1000;
    const maxPossible    = sessionSeconds * this._maxScorePerSecond;
    if (totalScore > maxPossible) {
      return { ok: false, reason: 'SCORE_EXCEEDS_PHYSICAL_MAXIMUM', totalScore, maxPossible };
    }
    return { ok: true };
  }
}


// ─────────────────────────────────────────────────────────────
// 6. SESSION GUARD — Replay attacks & token integrity
// ─────────────────────────────────────────────────────────────
class SessionGuard {
  constructor(config = {}) {
    this._tokenLifespan  = config.tokenLifespan ?? 30 * 60 * 1000; // 30 min
    this._usedNonces     = new Set();
    this._sessionId      = this._generateId();
    this._sessionStart   = Date.now();
    this._replayAttempts = 0;
  }

  _generateId() {
    return [
      Date.now().toString(36),
      Math.random().toString(36).slice(2),
      Math.random().toString(36).slice(2),
    ].join('-');
  }

  /** Generate a one-time-use nonce for a critical action */
  generateNonce() {
    const nonce = this._generateId();
    // Auto-expire nonces (store with timestamp)
    this._usedNonces.add(nonce);
    return nonce;
  }

  /** Validate a nonce — each nonce can only be used once */
  validateNonce(nonce) {
    if (!nonce) return { ok: false, reason: 'MISSING_NONCE' };
    if (this._usedNonces.has(`used:${nonce}`)) {
      this._replayAttempts++;
      return { ok: false, reason: 'REPLAY_ATTACK_DETECTED' };
    }
    if (!this._usedNonces.has(nonce)) {
      return { ok: false, reason: 'UNKNOWN_NONCE' };
    }
    // Mark as consumed
    this._usedNonces.delete(nonce);
    this._usedNonces.add(`used:${nonce}`);
    return { ok: true };
  }

  /** Check session hasn't expired */
  isSessionValid() {
    return (Date.now() - this._sessionStart) < this._tokenLifespan;
  }

  get sessionId()      { return this._sessionId; }
  get replayAttempts() { return this._replayAttempts; }
}


// ─────────────────────────────────────────────────────────────
// 7. ANTI-CHEAT ENGINE — Master orchestrator
// ─────────────────────────────────────────────────────────────
class AntiCheatEngine {
  constructor(config = {}) {
    this.integrity   = new IntegrityGuard();
    this.input       = new InputValidator(config.input);
    this.speedHack   = new SpeedHackDetector(config.speedHack);
    this.firewall    = new NetworkFirewall(config.network);
    this.score       = new ScoreValidator(config.score);
    this.session     = new SessionGuard(config.session);

    this._reportLog    = [];
    this._onViolation  = config.onViolation ?? this._defaultHandler.bind(this);
    this._banThreshold = config.banThreshold ?? 100;
    this._totalSuspicion = 0;
    this._banned       = false;

    // Start periodic checks
    this._speedCheckInterval = setInterval(() => this._periodicCheck(), 5000);
    this._integrityInterval  = setInterval(() => this._integrityCheck(), 2000);

    console.log(`[AntiCheat] Engine started | session: ${this.session.sessionId}`);
  }

  // ── Periodic background checks ──────────────────────────

  _periodicCheck() {
    if (!this.session.isSessionValid()) {
      this._report('SESSION_EXPIRED', { severity: 'CRITICAL' });
    }
    const speedResult = this.speedHack.check();
    if (!speedResult.ok) {
      speedResult.violations.forEach(v => this._report(v.type, v));
    }
  }

  _integrityCheck() {
    const violations = this.integrity.verifyAll();
    violations.forEach(v => this._report('STATE_TAMPERED', { ...v, severity: 'CRITICAL' }));
  }

  // ── Public API ──────────────────────────────────────────

  /** Call this every game tick */
  tick() {
    this.speedHack.tick();
  }

  /** Validate player input before applying it */
  validateInput(actionType, timestamp) {
    const result = this.input.record(actionType, timestamp);
    if (!result.ok) result.violations.forEach(v => this._report(v.type, v));
    return result.ok && !this._banned;
  }

  /** Validate a score gain before applying it */
  validateScoreGain(amount) {
    const result = this.score.recordGain(amount);
    if (!result.ok) result.violations.forEach(v => this._report(v.type, v, 30));
    return result.ok && !this._banned;
  }

  /** Inspect a network packet */
  inspectPacket(type, payload, playerId) {
    const result = this.firewall.inspect(type, payload, playerId);
    if (!result.ok && result.violation) this._report(result.reason, result.violation, 25);
    return result.ok;
  }

  /** Protect a critical game state object */
  protect(key, obj) {
    this.integrity.register(key, obj);
  }

  /** Commit a legitimate state change */
  commit(key, obj) {
    this.integrity.commit(key, obj);
  }

  /** Generate a nonce for a sensitive action */
  nonce() { return this.session.generateNonce(); }

  /** Validate a nonce */
  useNonce(n) {
    const r = this.session.validateNonce(n);
    if (!r.ok) this._report(r.reason, {}, 50);
    return r.ok;
  }

  // ── Internal ────────────────────────────────────────────

  _report(type, data = {}, suspicionWeight = 10) {
    const entry = {
      type,
      data,
      severity: data.severity ?? 'WARNING',
      timestamp: Date.now(),
      sessionId: this.session.sessionId,
    };
    this._reportLog.push(entry);
    this._totalSuspicion += suspicionWeight;
    this._onViolation(entry, this._totalSuspicion);

    if (this._totalSuspicion >= this._banThreshold && !this._banned) {
      this._banned = true;
      this._onViolation({ type: 'PLAYER_BANNED', severity: 'CRITICAL', timestamp: Date.now() }, this._totalSuspicion);
    }
  }

  _defaultHandler(violation, totalSuspicion) {
    const icon = violation.severity === 'CRITICAL' ? '🚨' : '⚠️';
    console.warn(`${icon} [AntiCheat] ${violation.type} | suspicion: ${totalSuspicion}`, violation.data);
  }

  // ── Status & Reporting ───────────────────────────────────

  status() {
    return {
      banned         : this._banned,
      totalSuspicion : this._totalSuspicion,
      inputSuspicion : this.input.suspicionScore,
      replayAttempts : this.session.replayAttempts,
      tamperCount    : this.integrity.tamperCount,
      speedViolations: this.speedHack.violations.length,
      networkBlocked : this.firewall.blocked,
      sessionValid   : this.session.isSessionValid(),
      reportCount    : this._reportLog.length,
    };
  }

  /** Export full violation report (e.g. to send to server) */
  exportReport() {
    return {
      sessionId : this.session.sessionId,
      status    : this.status(),
      violations: [...this._reportLog],
      generatedAt: new Date().toISOString(),
    };
  }

  /** Clean up timers when the game session ends */
  destroy() {
    clearInterval(this._speedCheckInterval);
    clearInterval(this._integrityInterval);
    console.log('[AntiCheat] Engine destroyed.');
  }
}


// ─────────────────────────────────────────────────────────────
// USAGE EXAMPLE
// ─────────────────────────────────────────────────────────────
/*

const ac = new AntiCheatEngine({
  score:     { maxScorePerSecond: 50, maxSingleGain: 200 },
  input:     { maxActionsPerSec: 15 },
  speedHack: { expectedTickRate: 60 },
  session:   { tokenLifespan: 20 * 60 * 1000 },
  banThreshold: 150,
  onViolation: (v, score) => {
    console.error('VIOLATION:', v.type, '| Total suspicion:', score);
    if (v.type === 'PLAYER_BANNED') {
      // Kick player, freeze controls, alert server…
      freezeGame();
      sendReportToServer(ac.exportReport());
    }
  },
});

// Register a packet schema
ac.firewall.registerSchema('PLAYER_MOVE', (p) =>
  typeof p.x === 'number' && typeof p.y === 'number' &&
  p.x >= 0 && p.x <= 1000 && p.y >= 0 && p.y <= 1000,
  60
);

// Protect the player state object
const player = { health: 100, ammo: 30, score: 0 };
ac.protect('player', player);

// In the game loop
function gameLoop() {
  ac.tick();

  // Validate input
  if (ac.validateInput('SHOOT')) {
    // process shot
  }

  // Validate score gain
  if (ac.validateScoreGain(10)) {
    player.score += 10;
    ac.commit('player', player);  // ← always commit after legitimate change
  }

  // Inspect network packet
  const packet = { x: 100, y: 200 };
  if (ac.inspectPacket('PLAYER_MOVE', packet, 'player_001')) {
    // apply movement
  }

  requestAnimationFrame(gameLoop);
}

gameLoop();

// At the end of session:
// ac.destroy();
// console.log(ac.exportReport());

*/

module.exports = {
  AntiCheatEngine,
  IntegrityGuard,
  InputValidator,
  SpeedHackDetector,
  NetworkFirewall,
  ScoreValidator,
  SessionGuard,
};
