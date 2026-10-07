// World rules, boss timing, explosions, stars, and enemy spawning.
int enemyScale() {
    return enemyIsBoss ? BOSS_SCALE : 1;
}

int enemyWidth() {
    if (enemyIsBoss && level == 5) return OMEGA_PRIME_W;
    return ENEMY_W * enemyScale();
}

int enemyHeight() {
    if (enemyIsBoss && level == 5) return OMEGA_PRIME_H;
    return ENEMY_H * enemyScale();
}

int enemyWeaponY(int topY) {
    return topY + (enemyIsBoss && level == 5 ? OMEGA_PRIME_WEAPON_ROW : enemyHeight() / 2);
}

bool timerActive(unsigned long endTime, unsigned long now) {
    return static_cast<int32_t>(endTime - now) > 0;
}

bool bossIntroActive(unsigned long now) {
    return enemyIsBoss && timerActive(bossIntroUntil, now);
}

int powerUpIconWidth() {
    if (powerUp.type == POWERUP_SHOTGUN) return SHOTGUN_ICON_W;
    return powerUp.type == POWERUP_SHIELD ? SHIELD_ICON_W : LARGE_POWERUP_W;
}

int powerUpIconHeight() {
    if (powerUp.type == POWERUP_SHIELD) return SHIELD_ICON_H;
    return powerUp.type == POWERUP_SHOTGUN ? SHOTGUN_ICON_H : LARGE_POWERUP_H;
}

void resetBossCombat() {
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) homingMissiles[i].active = false;
    bossFightStartedAt = 0;
    bossNextAttackAt = 0;
    bossWindupUntil = 0;
    bossLaserUntil = 0;
    bossShieldFlashUntil = 0;
    bossPhaseAnnouncementUntil = 0;
    bossAttackKind = BOSS_ATTACK_FAN;
    bossAttackAimY = 0;
    bossLaserY = 0;
    bossLaserSourceY = 0;
    bossLaserTargetX = 0;
    bossPatternIndex = 0;
    bossCharging = false;
    bossLaserFiring = false;
    bossEnraged = false;
}

bool bossShieldActive(unsigned long now) {
    return enemyIsBoss && level == 3 && enemies[0].active &&
           (bossIntroActive(now) ||
            (now - bossFightStartedAt) % BOSS_SHIELD_CYCLE_MS < BOSS_SHIELD_CLOSED_MS);
}

bool bossShieldOpeningSoon(unsigned long now) {
    if (!bossShieldActive(now) || bossIntroActive(now)) return false;
    unsigned long phase = (now - bossFightStartedAt) % BOSS_SHIELD_CYCLE_MS;
    return phase >= BOSS_SHIELD_CLOSED_MS - BOSS_SHIELD_WARNING_MS;
}

int bossBeamYAt(int x) {
    int sourceX = enemies[0].x;
    int distance = max(1, sourceX - bossLaserTargetX);
    return bossLaserSourceY + (bossLaserY - bossLaserSourceY) * (sourceX - x) / distance;
}

int comboMultiplier() { return min(3, 1 + comboKills / 3); }

void registerEnemyKill() {
    comboKills = min(comboKills + 1, 6);
    score += comboMultiplier() - 1; // The hit that killed the enemy already awarded one point.
}

int waveEnemyCount() {
    return endlessMode ? 14 + min(endlessWave - 1, 23) * 2
                       : REGULAR_ENEMIES_BY_LEVEL[level - 1];
}

unsigned long enemyShotInterval() {
    return endlessMode ? 2000UL - static_cast<unsigned long>(min(endlessWave - 1, 11)) * 100UL
                       : ENEMY_SHOT_INTERVAL_MS[level - 1];
}

int regularEnemySpeed() {
    return endlessMode ? 3 + min((endlessWave - 1) / 3, 3)
                       : SMALL_ENEMY_SPEED_BY_LEVEL[level - 1];
}

int enemySpawnMinMs() { return endlessMode ? 1000 - min(endlessWave - 1, 10) * 50 : ENEMY_ENTRY_MIN_MS; }
int enemySpawnMaxMs() { return endlessMode ? 3000 - min(endlessWave - 1, 15) * 100 : ENEMY_ENTRY_MAX_MS; }

unsigned long bossAttackInterval() {
    if (level == 2) {
        // Campaign: 3.2 s cooldown + 0.6 s charge. Endless follows the same
        // proportional difficulty curve as other enemies, with a safe lower cap.
        return endlessMode ? 3200UL - static_cast<unsigned long>(min(endlessWave - 1, 11)) * 160UL
                           : 3200UL;
    }
    unsigned long interval = bossEnraged ? 1400UL : enemyShotInterval();
    if (endlessMode && bossEnraged) interval = min(interval, enemyShotInterval());
    return level <= 2 ? interval / 2 : interval;
}

void clearExplosions() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;
}

void addExplosion(int x, int y, int scale, unsigned long now) {
    int slot = 0;
    unsigned long oldestAge = 0;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active ||
            now - explosions[i].startedAt >= EXPLOSION_DURATION_MS) {
            slot = i;
            break;
        }

        unsigned long age = now - explosions[i].startedAt;
        if (age > oldestAge) {
            oldestAge = age;
            slot = i;
        }
    }

    explosions[slot].x = x;
    explosions[slot].y = y;
    explosions[slot].scale = constrain(scale, 1, BOSS_SCALE);
    explosions[slot].startedAt = now;
    explosions[slot].active = true;
}

void drawExplosionFrame(int x, int y, int scale, unsigned long elapsed) {
    if (elapsed >= EXPLOSION_DURATION_MS) return;
    // Stretch every phase and debris movement, keeping the original blast size.
    elapsed = elapsed * EXPLOSION_TIMELINE_MS / EXPLOSION_DURATION_MS;

    // A bright flash breaks into a ragged burst, then scattered embers.
    static const uint16_t coreFrames[3][9] = {
        {0x010, 0x054, 0x038, 0x0FE, 0x07C, 0x0FE, 0x038, 0x054, 0x010},
        {0x044, 0x112, 0x06C, 0x0D6, 0x1BB, 0x0EC, 0x052, 0x089, 0x024},
        {0x000, 0x040, 0x004, 0x028, 0x010, 0x082, 0x020, 0x002, 0x000}
    };
    static const int8_t debrisVelocity[16][2] = {
        {-8, -5}, {-3, -10}, {2, -9}, {8, -6},
        {11, -1}, {8, 4}, {4, 10}, {-2, 8},
        {-7, 7}, {-11, 2}, {-9, -2}, {6, 7},
        {1, -12}, {-6, -9}, {12, 4}, {-4, 12}
    };

    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    if (elapsed < 300) {
        int frame = elapsed < 100 ? 0 : (elapsed < 200 ? 1 : 2);
        for (int row = 0; row < 9; row++) {
            for (int col = 0; col < 9; col++) {
                if (coreFrames[frame][row] & (1U << (8 - col))) {
                    oled.drawBox(x + (col - 4) * scale,
                                 y + (row - 4) * scale, scale, scale);
                }
            }
        }
    }

    // Fixed trajectories keep fragments moving smoothly instead of jittering.
    int particleCount = scale == BOSS_SCALE ? 16 : 10;
    int seed = (x + 3 * y) & 15;
    for (int i = 0; i < particleCount; i++) {
        unsigned long lifetime = 280UL + ((i * 29 + seed * 7) % 200);
        if (elapsed < 70 || elapsed >= lifetime) continue;
        if (elapsed > lifetime - 80 && ((elapsed / 35 + i) & 1)) continue;

        int direction = (i * 5 + seed) & 15;
        int vx = debrisVelocity[direction][0] * scale;
        int vy = debrisVelocity[direction][1] * scale;
        int travel = static_cast<int>(elapsed);
        int px = x + vx * travel / static_cast<int>(EXPLOSION_TIMELINE_MS);
        int py = y + vy * travel / static_cast<int>(EXPLOSION_TIMELINE_MS);

        if (elapsed < 280) {
            int tailTime = travel - 55;
            drawPlayfieldLine(x + vx * tailTime / static_cast<int>(EXPLOSION_TIMELINE_MS),
                          y + vy * tailTime / static_cast<int>(EXPLOSION_TIMELINE_MS),
                          px, py);
            oled.drawBox(px, py, (i % 3 == 0) ? 2 : 1, 2);
        } else {
            oled.drawPixel(px, py);
        }
    }
    oled.setMaxClipWindow();
}

void drawExplosions(unsigned long now) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;

        unsigned long elapsed = now - explosions[i].startedAt;
        if (elapsed >= EXPLOSION_DURATION_MS) {
            explosions[i].active = false;
            continue;
        }
        drawExplosionFrame(explosions[i].x, explosions[i].y,
                           explosions[i].scale, elapsed);
    }
}

bool starNearEnemyProjectile(int x, int y) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        const EnemyBullet &bullet = enemyBullets[i];
        if (!bullet.active) continue;
        const int bulletY = bullet.yQ / ENEMY_BULLET_Y_SCALE;
        if (x >= bullet.x - STAR_PROJECTILE_MARGIN &&
            x <= bullet.x + 2 + STAR_PROJECTILE_MARGIN &&
            y >= bulletY - STAR_PROJECTILE_MARGIN &&
            y <= bulletY + 2 + STAR_PROJECTILE_MARGIN) return true;
    }
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        const HomingMissile &missile = homingMissiles[i];
        if (!missile.active) continue;
        const int centerY = missile.yQ / ENEMY_BULLET_Y_SCALE;
        // Include the animated exhaust, not just the missile body.
        if (x >= missile.x - STAR_PROJECTILE_MARGIN &&
            x <= missile.x + MISSILE_W + 2 + STAR_PROJECTILE_MARGIN &&
            y >= centerY - MISSILE_H / 2 - STAR_PROJECTILE_MARGIN &&
            y <= centerY + MISSILE_H / 2 + STAR_PROJECTILE_MARGIN) return true;
    }
    return false;
}

void drawStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        if (!starNearEnemyProjectile(starsX[i], starsY[i]))
            oled.drawPixel(starsX[i], starsY[i]);
    }
}

void drawExplosionScene(unsigned long now) {
    oled.clearBuffer();
    drawStars();
    drawExplosions(now);
    oled.sendBuffer();
}

void initializeEnemy(Enemy &enemy, bool boss) {
    enemy.type = boss ? ENEMY_STANDARD
                      : static_cast<EnemyType>((regularEnemiesSpawned + level - 1) % 3);
    enemy.moveRemainder = 0;
    enemy.hitFlashUntil = 0;
    int scale = boss ? BOSS_SCALE : 1;
    int height = boss && level == 5 ? OMEGA_PRIME_H : ENEMY_H * scale;
    enemy.x = SCREEN_W + 4;
    int minY = HUD_HEIGHT + 4;
    int maxY = SCREEN_H - height - 8;
    enemy.y = random(minY, maxY + 1);
    enemy.health = boss ? BOSS_HEALTH_BY_LEVEL[level - 1] + (endlessMode ? min(endlessWave, 50) * 4 : 0)
                        : (enemy.type == ENEMY_GUNNER ? GUNNER_HEALTH : SMALL_ENEMY_HEALTH);
    enemy.maxHealth = enemy.health;
    enemy.direction = 1;
    unsigned long shotInterval = enemyShotInterval();
    unsigned long now = millis();
    enemy.lastShot = boss ? now : now - random(0, shotInterval);
    enemy.active = true;
}

void spawnPendingEnemies(unsigned long now) {
    if (enemyIsBoss || regularEnemiesSpawned >= waveEnemyCount() ||
        timerActive(nextEnemySpawnAt, now)) return;

    int activeCount = 0;
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        if (enemies[i].active) activeCount++;
    }
    if (activeCount >= MAX_SIMULTANEOUS_ENEMIES) return;

    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        if (!enemies[i].active) {
            initializeEnemy(enemies[i], false);
            regularEnemiesSpawned++;
            nextEnemySpawnAt = now + static_cast<unsigned long>(
                random(enemySpawnMinMs(), enemySpawnMaxMs() + 1));
            return;
        }
    }
}

void spawnBoss() {
    resetBossCombat();
    enemyIsBoss = true;
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
    enemyShotPattern = 0;
    initializeEnemy(enemies[0], true);
    bossIntroUntil = millis() + BOSS_INTRO_DURATION_MS;
    bossFightStartedAt = bossIntroUntil;
    bossNextAttackAt = bossFightStartedAt + bossAttackInterval();
    // Start firing cooldowns after the banner, so neither side fires during entry.
    lastShot = bossIntroUntil;
    enemies[0].lastShot = bossIntroUntil;
    lastLaserDamage = bossIntroUntil;
}

void startLevelWave() {
    resetBossCombat();
    regularEnemiesRemaining = waveEnemyCount();
    regularEnemiesSpawned = 0;
    enemyIsBoss = false;
    bossIntroUntil = 0;
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) enemies[i].active = false;
    enemyShotPattern = 0;
    nextEnemySpawnAt = millis();
    spawnPendingEnemies(nextEnemySpawnAt);
}
