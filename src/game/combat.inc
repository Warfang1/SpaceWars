// Player and enemy weapons, damage, boss attacks, and power-ups.
void shoot() {
    const bool shotgunActive = timerActive(shotgunUntil, millis());
    const int8_t spreadSteps[3] = {-1, 0, 1};
    const int pelletCount = shotgunActive ? 3 : 1;
    bool fired = false;

    for (int pellet = 0; pellet < pelletCount; pellet++) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) continue;
            bullets[i].active = true;
            bullets[i].x = playerX + PLAYER_W;
            bullets[i].y = playerY + 4;
            bullets[i].yQ = bullets[i].y * PLAYER_BULLET_Y_SCALE;
            bullets[i].verticalStep = shotgunActive ? spreadSteps[pellet] : 0;
            fired = true;
            break;
        }
    }

    if (fired) {
        tone(BUZZER_PIN, NOTE_C5, 8, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
    }
}

void enemyShoot(const Enemy &enemy) {
    if (enemy.type == ENEMY_SCOUT) return;
    int centerY = enemyWeaponY(enemy.y);
    int playerAimY = playerY + 4;
    if (enemy.type == ENEMY_GUNNER) {
        addAimedEnemyBullet(enemy.x, centerY - 2, playerAimY - 3);
        addAimedEnemyBullet(enemy.x, centerY + 2, playerAimY + 3);
        return;
    }

    addAimedEnemyBullet(enemy.x, centerY, playerAimY);
}

void updateEnemyShooting(unsigned long now) {
    if (gameOver || levelVictory || bossIntroActive(now)) return;
    if (enemyIsBoss) {
        updateBossCombat(now);
        return;
    }
    unsigned long shotInterval = enemyShotInterval();
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        if (!enemies[i].active || enemies[i].type == ENEMY_SCOUT ||
            enemies[i].x >= SCREEN_W) continue;
        const unsigned long interval = enemies[i].type == ENEMY_GUNNER
                                           ? shotInterval * 5UL / 4UL : shotInterval;
        if (now - enemies[i].lastShot < interval) continue;
        enemies[i].lastShot = now;
        enemyShoot(enemies[i]);
    }
}

void addAimedEnemyBullet(int sourceX, int startY, int targetY) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) {
            int bulletX = sourceX - 2;
            int bulletY = constrain(startY, HUD_HEIGHT + 1, SCREEN_H - 3);
            int targetBulletY = constrain(targetY, HUD_HEIGHT + 1, SCREEN_H - 3);
            int horizontalDistance = max(2, bulletX - (playerX + PLAYER_W / 2));
            int flightFrames = max(1, (horizontalDistance + 1) / 2);

            enemyBullets[i].active = true;
            enemyBullets[i].x = bulletX;
            enemyBullets[i].yQ = bulletY * ENEMY_BULLET_Y_SCALE;
            enemyBullets[i].dyQ = ((targetBulletY - bulletY) * ENEMY_BULLET_Y_SCALE) /
                                  flightFrames;
            enemyBullets[i].speedX = 2;
            return;
        }
    }
}

void damagePlayer(unsigned long now) {
    if (gameOver || levelVictory || timerActive(shieldUntil, now) ||
        timerActive(invulnerableUntil, now) || timerActive(playerExplosionUntil, now)) return;
    lives--;
    comboKills = 0;
    invulnerableUntil = now + INVULNERABLE_DURATION;
    playerExplosionX = playerX + PLAYER_W / 2;
    playerExplosionY = playerY + PLAYER_H / 2;
    playerExplosionStartedAt = now;
    playerExplosionUntil = now + EXPLOSION_DURATION_MS;
    addExplosion(playerExplosionX, playerExplosionY, 1, now);
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    tone(BUZZER_PIN, NOTE_C3, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    if (lives <= 0) {
        gameOver = true;
        saveHighScore();
    }
}

void destroyMissile(int index, unsigned long now, bool awardScore) {
    HomingMissile &missile = homingMissiles[index];
    if (!missile.active) return;
    missile.active = false;
    addExplosion(missile.x + MISSILE_W / 2,
                 missile.yQ / ENEMY_BULLET_Y_SCALE, 1, now);
    if (awardScore) score += 2;
}

int activeMissileCount() {
    int count = 0;
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        if (homingMissiles[i].active) count++;
    }
    return count;
}

void launchHomingMissiles(unsigned long now) {
    int centerY = enemyWeaponY(enemies[0].y);
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        if (homingMissiles[i].active) continue;
        HomingMissile &missile = homingMissiles[i];
        missile.x = enemies[0].x - MISSILE_W - 2;
        int startY = constrain(centerY + (i == 0 ? -5 : 5),
                               HUD_HEIGHT + 3, SCREEN_H - 3);
        missile.yQ = startY * ENEMY_BULLET_Y_SCALE;
        missile.dyQ = 0;
        missile.bornAt = now;
        missile.lastSteerAt = now;
        missile.active = true;
        missile.speedRemainder = 0;
    }
}

void updateHomingMissiles(unsigned long now) {
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        HomingMissile &missile = homingMissiles[i];
        if (!missile.active) continue;
        if (now - missile.lastSteerAt >= MISSILE_STEER_INTERVAL_MS) {
            missile.lastSteerAt = now;
            int delta = playerY + PLAYER_H / 2 - missile.yQ / ENEMY_BULLET_Y_SCALE;
            int desiredSpeed = delta > 3 ? 8 : (delta < -3 ? -8 : 0);
            missile.dyQ += constrain(desiredSpeed - missile.dyQ, -2, 2);
        }
        // Preserve fractional speed: 2.55 pixels/tick, 15% below the previous 3.
        missile.speedRemainder += MISSILE_SPEED_MULTIPLIER * MISSILE_SPEED_PERCENT;
        const int steps = missile.speedRemainder / 100;
        missile.speedRemainder %= 100;
        missile.x -= steps;
        missile.yQ += missile.dyQ * steps;
        const int minYQ = (HUD_HEIGHT + 3) * ENEMY_BULLET_Y_SCALE;
        const int maxYQ = (SCREEN_H - 3) * ENEMY_BULLET_Y_SCALE;
        if (missile.yQ < minYQ) {
            missile.yQ = minYQ;
            missile.dyQ = 2;
        } else if (missile.yQ > maxYQ) {
            missile.yQ = maxYQ;
            missile.dyQ = -2;
        }
        if (missile.x + MISSILE_W <= 0 || now - missile.bornAt >= MISSILE_LIFETIME_MS) {
            missile.active = false;
            continue;
        }
        int missileY = missile.yQ / ENEMY_BULLET_Y_SCALE;
        if (missile.x <= playerX + PLAYER_W - 1 && missile.x + MISSILE_W - 1 >= playerX &&
            missileY + MISSILE_H / 2 >= playerY &&
            missileY - MISSILE_H / 2 <= playerY + PLAYER_H - 1) {
            destroyMissile(i, now, false);
            damagePlayer(now);
            if (gameOver) return;
        }
    }
}

void fireBossFan() {
    const int sourceX = enemies[0].x - 2;
    const int sourceY = enemyWeaponY(enemies[0].y);
    const int distance = max(1, sourceX - (playerX + PLAYER_W / 2));
    const int aimSpeed = (bossAttackAimY - sourceY) * ENEMY_BULLET_Y_SCALE / distance;
    for (int direction = -1; direction <= 1; direction++) {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemyBullets[i].active) continue;
            enemyBullets[i].active = true;
            enemyBullets[i].x = sourceX;
            enemyBullets[i].yQ = sourceY * ENEMY_BULLET_Y_SCALE;
            enemyBullets[i].dyQ = aimSpeed + direction * 5;
            enemyBullets[i].speedX = 1;
            break;
        }
    }
}

void fireBossAttack(unsigned long now) {
    bossCharging = false;
    enemies[0].lastShot = now;
    if (bossAttackKind == BOSS_ATTACK_MISSILES) {
        launchHomingMissiles(now);
    } else if (bossAttackKind == BOSS_ATTACK_LASER) {
        bossLaserFiring = true;
        bossLaserUntil = now + BOSS_LASER_DURATION_MS;
    } else {
        fireBossFan();
    }
    bossNextAttackAt = now + bossAttackInterval();
}

void updateBossCombat(unsigned long now) {
    if (!enemyIsBoss || !enemies[0].active || gameOver || levelVictory || bossIntroActive(now)) return;
    if (level == 5 && !bossEnraged && enemies[0].health * 2 < enemies[0].maxHealth) {
        bossEnraged = true;
        bossPhaseAnnouncementUntil = now + 1400;
        if (!bossCharging && !bossLaserFiring) bossNextAttackAt = now + 1000;
    }
    if (bossLaserFiring) {
        if (!timerActive(bossLaserUntil, now)) {
            bossLaserFiring = false;
        } else {
            int leftY = bossBeamYAt(playerX);
            int rightY = bossBeamYAt(playerX + PLAYER_W - 1);
            if (playerX < enemies[0].x && playerY <= max(leftY, rightY) + 1 &&
                playerY + PLAYER_H - 1 >= min(leftY, rightY) - 1) damagePlayer(now);
            return;
        }
    }
    if (bossCharging) {
        if (timerActive(bossWindupUntil, now)) return;
        fireBossAttack(now);
        return;
    }
    if (timerActive(bossNextAttackAt, now)) return;
    if (level == 5) {
        static const int attackCycle[3] = {
            BOSS_ATTACK_FAN, BOSS_ATTACK_MISSILES, BOSS_ATTACK_LASER
        };
        bossAttackKind = attackCycle[bossPatternIndex % 3];
    } else {
        bossAttackKind = level == 2 ? BOSS_ATTACK_MISSILES
                         : level == 4 ? BOSS_ATTACK_LASER : BOSS_ATTACK_FAN;
    }
    if (bossAttackKind == BOSS_ATTACK_MISSILES && activeMissileCount() >= MAX_HOMING_MISSILES) {
        bossNextAttackAt = now + 400;
        return;
    }
    bossPatternIndex++;
    bossAttackAimY = playerY + PLAYER_H / 2;
    // Lock a line through the player's current center, including at either screen edge.
    bossLaserY = bossAttackAimY;
    bossLaserSourceY = enemyWeaponY(enemies[0].y);
    bossLaserTargetX = playerX + PLAYER_W / 2;
    if (bossAttackKind == BOSS_ATTACK_FAN) {
        fireBossAttack(now);
        return;
    }
    unsigned long windup = bossAttackKind == BOSS_ATTACK_LASER ? 1500UL : 1200UL;
    if (level == 2) windup /= 2;
    bossCharging = true;
    bossWindupUntil = now + windup;
}

void maybeSpawnPowerUp(const Enemy &enemy) {
    if (powerUp.active || random(0, 4) != 0) return;

    powerUp.active = true;
    powerUp.x = enemy.x;
    powerUp.type = static_cast<PowerUpType>(random(POWERUP_EXTRA_LIFE,
                                                   POWERUP_LASER + 1));
    const int height = powerUpIconHeight();
    powerUp.y = constrain(enemy.y + enemyHeight() / 2 - height / 2,
                          HUD_HEIGHT + 1, SCREEN_H - height);
}

bool damageEnemy(int enemyIndex) {
    Enemy &enemy = enemies[enemyIndex];
    if (!enemy.active) return false;
    unsigned long now = millis();
    if (enemyIsBoss && (bossIntroActive(now) || bossShieldActive(now))) {
        bossShieldFlashUntil = now + 120;
        return false;
    }

    enemy.health--;
    enemy.hitFlashUntil = now + ENEMY_HIT_FLASH_MS;
    score++;
    if (enemy.health > 0) {
        maybeSpawnPowerUp(enemy);
        return false;
    }
    if (!enemyIsBoss) maybeSpawnPowerUp(enemy);

    addExplosion(enemy.x + enemyWidth() / 2,
                 enemy.y + enemyHeight() / 2, enemyScale(), now);
    registerEnemyKill();
    enemy.active = false;
    if (enemyIsBoss) {
        victoryExplosionUntil = now + EXPLOSION_DURATION_MS;
        clearObjects();
        levelVictory = true;
        victoryNeedsRelease = true;
        saveHighScore();
        tone(BUZZER_PIN, NOTE_E4, 90, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
    } else {
        regularEnemiesRemaining--;
        if (regularEnemiesRemaining <= 0) {
            spawnBoss();
            tone(BUZZER_PIN, NOTE_G5, 90, BUZZER_CHANNEL);
        } else {
            tone(BUZZER_PIN, NOTE_E4, 90, BUZZER_CHANNEL);
        }
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
    }
    return true;
}

void collectPowerUp(unsigned long now) {
    powerUpPopupType = powerUp.type;
    powerUpPopupUntil = now + POWERUP_POPUP_DURATION_MS;
    powerUpPopupAnchorX = powerUp.x + powerUpIconWidth() / 2;
    powerUpPopupAnchorY = powerUp.y;
    if (powerUp.type == POWERUP_EXTRA_LIFE) {
        if (lives < MAX_LIVES) lives++;
        else score += 5;
    } else if (powerUp.type == POWERUP_SHIELD) {
        shieldUntil = now + SHIELD_DURATION;
    } else if (powerUp.type == POWERUP_SHOTGUN) {
        shotgunUntil = now + SHOTGUN_DURATION;
    } else if (powerUp.type == POWERUP_LASER) {
        laserUntil = now + LASER_DURATION;
        lastLaserDamage = now;
    }
    powerUp.active = false;
    tone(BUZZER_PIN, NOTE_G5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}
