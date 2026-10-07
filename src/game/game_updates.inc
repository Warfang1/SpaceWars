// Per-frame movement, collision, and update routines.
void updateStars() {
    const unsigned long now = millis();
    if (static_cast<uint32_t>(now - lastStarMove) < STAR_MOVE_DELAY) return;
    lastStarMove = now;
    for (int i = 0; i < MAX_STARS; i++) {
        starsX[i]--;
        if (starsX[i] < 0) {
            starsX[i] = SCREEN_W - 1;
            starsY[i] = random(HUD_HEIGHT + 2, SCREEN_H);
        }
    }
}

void readPlayerMovementDirection(int &moveX, int &moveY) {
    int axisX = joystickDirection(readJoystickAxis(JOYSTICK_X_PIN),
                                  joystickCenterX, INVERT_JOYSTICK_X);
    int axisY = joystickDirection(readJoystickAxis(JOYSTICK_Y_PIN),
                                  joystickCenterY, INVERT_JOYSTICK_Y);
    // Rotate controls 90 degrees counterclockwise in screen coordinates (Y down).
    moveX = axisY;
    moveY = -axisX;
}

void activateDodgeDash(unsigned long now) {
    if (gameOver || levelVictory || bossIntroActive(now) ||
        timerActive(playerExplosionUntil, now) || timerActive(dodgeCooldownUntil, now)) return;

    int moveX = 0;
    int moveY = 0;
    readPlayerMovementDirection(moveX, moveY);
    if (moveX == 0 && moveY == 0) return;

    dashDirectionX = moveX;
    dashDirectionY = moveY;
    dashUntil = now + DASH_DURATION_MS;
    dashTrailUntil = now + DASH_TRAIL_DURATION_MS;
    dashTrailX = playerX;
    dashTrailY = playerY;
    playerMoveRemainder = 0;
    dodgeCooldownUntil = now + DODGE_COOLDOWN_MS;
}

void updatePlayer() {
    const unsigned long now = millis();
    if (timerActive(dashUntil, now)) {
        const int step = dashDirectionX != 0 && dashDirectionY != 0
                             ? DASH_DIAGONAL_STEP_PIXELS
                             : DASH_STEP_PIXELS;
        playerX = constrain(playerX + dashDirectionX * step, 2, 48);
        playerY = constrain(playerY + dashDirectionY * step,
                            HUD_HEIGHT + 3, SCREEN_H - 10);
        dashTrailX = playerX;
        dashTrailY = playerY;
        return;
    }

    int moveX = 0;
    int moveY = 0;
    readPlayerMovementDirection(moveX, moveY);
    if (moveX == 0 && moveY == 0) {
        playerMoveRemainder = 0;
        return;
    }
    // Six pixels per five movement ticks = exactly 20% more speed on each axis.
    playerMoveRemainder += PLAYER_MOVE_RATE;
    int steps = playerMoveRemainder / 5;
    playerMoveRemainder %= 5;
    playerX = constrain(playerX + moveX * steps, 2, 48);
    playerY = constrain(playerY + moveY * steps, HUD_HEIGHT + 3, SCREEN_H - 10);
}

void updateEnemies() {
    int minY = HUD_HEIGHT + 3;
    int maxY = SCREEN_H - enemyHeight() - 9;

    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        Enemy &enemy = enemies[i];
        if (!enemy.active) continue;

        if (enemyIsBoss) {
            int targetX = SCREEN_W - enemyWidth() - 6;
            if (enemy.x > targetX) {
                enemy.x -= min(BOSS_SCALE, enemy.x - targetX);
            }
            if (bossCharging || bossLaserFiring) continue;
            enemy.y += enemy.direction;
            if (enemy.y < minY) {
                enemy.y = minY;
                enemy.direction = 1;
            } else if (enemy.y > maxY) {
                enemy.y = maxY;
                enemy.direction = -1;
            }
            continue;
        }

        int speed = regularEnemySpeed();
        if (enemy.type == ENEMY_SCOUT) {
            const int deltaX = playerX + PLAYER_W / 2 - (enemy.x + ENEMY_W / 2);
            const int deltaY = playerY + PLAYER_H / 2 - (enemy.y + ENEMY_H / 2);
            const float distance = sqrtf(static_cast<float>(deltaX * deltaX + deltaY * deltaY));
            // Scouts keep the same pursuit speed on every level.
            const float travel = min(static_cast<float>(SCOUT_SPEED), distance);
            const int moveX = distance > 0 ? lroundf(deltaX * travel / distance) : 0;
            const int moveY = distance > 0 ? lroundf(deltaY * travel / distance) : 0;
            const int startX = enemy.x;
            const int startY = enemy.y;
            const int steps = max(1, max(abs(moveX), abs(moveY)));
            bool hitPlayer = false;
            // Check each pixel along the route so fast scouts cannot jump through the player.
            for (int step = 0; step <= steps; step++) {
                enemy.x = startX + moveX * step / steps;
                enemy.y = startY + moveY * step / steps;
                hitPlayer = enemy.x <= playerX + PLAYER_W - 1 &&
                            enemy.x + ENEMY_W - 1 >= playerX &&
                            enemy.y <= playerY + PLAYER_H - 1 &&
                            enemy.y + ENEMY_H - 1 >= playerY;
                if (hitPlayer) break;
            }
            if (hitPlayer || enemy.x + ENEMY_W <= 0) {
                const unsigned long now = millis();
                if (hitPlayer) {
                    addExplosion(enemy.x + ENEMY_W / 2, enemy.y + ENEMY_H / 2, 1, now);
                    damagePlayer(now);
                }
                enemy.active = false;
                regularEnemiesRemaining--; // Escapes/collisions award no kill points or drops.
                if (gameOver) return;
                if (regularEnemiesRemaining <= 0) {
                    spawnBoss();
                    return;
                }
            }
            continue;
        }
        else if (enemy.type == ENEMY_STANDARD) {
            const int speedQ = endlessMode ? speed * 100 : STANDARD_SPEED_Q_BY_LEVEL[level - 1];
            enemy.moveRemainder += speedQ;
            speed = enemy.moveRemainder / 100;
            enemy.moveRemainder %= 100;
        }
        else if (enemy.type == ENEMY_GUNNER) {
            enemy.moveRemainder += speed;
            speed = enemy.moveRemainder / 2;
            enemy.moveRemainder %= 2;
        }
        int targetX = playerX + 48;
        if (enemy.x > targetX) {
            enemy.x -= min(speed, enemy.x - targetX);
        }
    }
}

void updatePowerUp(unsigned long now) {
    if (!powerUp.active) return;

    const int width = powerUpIconWidth();
    const int height = powerUpIconHeight();
    powerUp.x--;
    if (powerUp.x + width <= 0) {
        powerUp.active = false;
        return;
    }

    bool overlapsPlayer = powerUp.x <= playerX + PLAYER_W - 1 &&
                          powerUp.x + width - 1 >= playerX &&
                          powerUp.y <= playerY + PLAYER_H - 1 &&
                          powerUp.y + height - 1 >= playerY;
    if (overlapsPlayer) collectPowerUp(now);
}

void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].x += 3;
        // Half-pixel vertical steps give the shotgun a narrower spread.
        bullets[i].yQ += bullets[i].verticalStep;
        bullets[i].y = bullets[i].yQ / PLAYER_BULLET_Y_SCALE;

        if (bullets[i].x > SCREEN_W || bullets[i].y <= HUD_HEIGHT ||
            bullets[i].y >= SCREEN_H - 1) {
            bullets[i].active = false;
            continue;
        }

        int bulletTipY = bullets[i].y + bullets[i].verticalStep;
        int bulletTop = min(bullets[i].y, bulletTipY);
        int bulletBottom = max(bullets[i].y, bulletTipY);
        for (int missileIndex = 0; missileIndex < MAX_HOMING_MISSILES; missileIndex++) {
            HomingMissile &missile = homingMissiles[missileIndex];
            int missileY = missile.yQ / ENEMY_BULLET_Y_SCALE;
            if (missile.active && bullets[i].x + 4 >= missile.x &&
                bullets[i].x <= missile.x + MISSILE_W - 1 &&
                bulletBottom >= missileY - MISSILE_H / 2 &&
                bulletTop <= missileY + MISSILE_H / 2) {
                bullets[i].active = false;
                destroyMissile(missileIndex, millis(), true);
                break;
            }
        }
        if (!bullets[i].active) continue;
        for (int j = 0; j < MAX_SIMULTANEOUS_ENEMIES; j++) {
            Enemy &enemy = enemies[j];
            if (!enemy.active || bullets[i].x + 4 < enemy.x ||
                bullets[i].x > enemy.x + enemyWidth() ||
                bulletBottom < enemy.y || bulletTop > enemy.y + enemyHeight()) {
                continue;
            }

            bullets[i].active = false;
            if (damageEnemy(j)) return;
            break;
        }
    }
}

void updateLaser(unsigned long now) {
    if (gameOver || levelVictory || bossIntroActive(now) || !timerActive(laserUntil, now) ||
        timerActive(playerExplosionUntil, now) ||
        now - lastLaserDamage < LASER_DAMAGE_INTERVAL_MS) return;

    lastLaserDamage = now;
    int laserY = playerY + 4;
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        HomingMissile &missile = homingMissiles[i];
        int missileY = missile.yQ / ENEMY_BULLET_Y_SCALE;
        if (missile.active && missile.x + MISSILE_W - 1 >= playerX + PLAYER_W &&
            laserY >= missileY - MISSILE_H / 2 && laserY <= missileY + MISSILE_H / 2) {
            destroyMissile(i, now, true);
        }
    }
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        Enemy &enemy = enemies[i];
        if (!enemy.active || enemy.x + enemyWidth() < playerX + PLAYER_W ||
            laserY < enemy.y || laserY > enemy.y + enemyHeight()) {
            continue;
        }

        damageEnemy(i);
        if (levelVictory) return;
    }
}

void updateEnemyBullets() {
    unsigned long now = millis();

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) continue;
        enemyBullets[i].x -= enemyBullets[i].speedX;
        enemyBullets[i].yQ += enemyBullets[i].dyQ;
        int bulletY = enemyBullets[i].yQ / ENEMY_BULLET_Y_SCALE;

        if (enemyBullets[i].x < -2 || bulletY < HUD_HEIGHT ||
            bulletY >= SCREEN_H - 1) {
            enemyBullets[i].active = false;
            continue;
        }

        if (enemyBullets[i].x + 2 >= playerX &&
            enemyBullets[i].x <= playerX + PLAYER_W - 1 &&
            bulletY + 2 >= playerY && bulletY <= playerY + PLAYER_H - 1) {
            enemyBullets[i].active = false;
            damagePlayer(now);
            if (gameOver) return;
        }
    }
}
