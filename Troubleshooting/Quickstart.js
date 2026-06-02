const ac = new AntiCheatEngine({ banThreshold: 150 });

// Register what valid packets look like
ac.firewall.registerSchema('PLAYER_MOVE', p => 
  typeof p.x === 'number' && p.x >= 0 && p.x <= 1000
);

// Protect a state object
ac.protect('player', player);

// In your game loop:
ac.tick();
if (ac.validateInput('JUMP')) { /* process */ }
if (ac.validateScoreGain(10)) { player.score += 10; ac.commit('player', player); }
