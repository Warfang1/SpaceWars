// Power-up timers, HUD, playfield, boss effects, and frame rendering.
bool powerUpTimersActive(unsigned long now) {
    return timerActive(shieldUntil, now) || timerActive(shotgunUntil, now) || timerActive(laserUntil, now);
}

int powerUpTimerFill(unsigned long until, unsigned long duration, unsigned long now) {
    if (!timerActive(until, now)) return 0;
    const unsigned long remaining = min(duration, static_cast<unsigned long>(static_cast<uint32_t>(until - now)));
    return (remaining * 27UL + duration - 1UL) / duration;
}

void drawPowerUpTimers(unsigned long now) {
    if (!powerUpTimersActive(now)) return;
    const unsigned long ends[3] = {shieldUntil, shotgunUntil, laserUntil};
    const unsigned long durations[3] = {SHIELD_DURATION, SHOTGUN_DURATION, LASER_DURATION};
    const char *labels[3] = {"SH", "SG", "L"};
    oled.setDrawColor(0);
    oled.drawBox(0, 57, SCREEN_W, 7);
    oled.setDrawColor(1);
    oled.setFont(u8g2_font_4x6_tr);
    for (int i = 0; i < 3; i++) {
        const int fill = powerUpTimerFill(ends[i], durations[i], now);
        if (!fill) continue;
        const int x = i * 42 + 2;
        oled.drawStr(x, 63, labels[i]);
        oled.drawFrame(x + 10, 59, 29, 4);
        oled.drawBox(x + 11, 60, fill, 2);
    }
}

void drawHud() {
    oled.setFont(u8g2_font_5x7_tr);
    oled.setCursor(0, 7);
    oled.print("S:");
    oled.print(score);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 7; col++) {
            if (HEART_ICON_ROWS[row] & (1U << (6 - col))) {
                oled.drawPixel(43 + col, row);
            }
        }
    }
    oled.setCursor(52, 7);
    oled.print(lives);
    oled.setCursor(91, 7);
    oled.print(endlessMode ? "W" : "LV:");
    if (endlessMode && endlessWave > 999) oled.print("999+");
    else oled.print(endlessMode ? endlessWave : level);
    oled.setCursor(67, 7);
    if (enemyIsBoss) {
        oled.print("BOSS");
    } else {
        oled.print("E:");
        oled.print(regularEnemiesRemaining);
    }
    if (comboMultiplier() > 1) {
        oled.setCursor(118, 7);
        oled.print("x");
        oled.print(comboMultiplier());
    }
    oled.drawLine(0, HUD_HEIGHT, SCREEN_W - 1, HUD_HEIGHT);
}

void drawPlayfieldLine(int x1, int y1, int x2, int y2) {
    // Keep coordinates signed: U8g2's unsigned line endpoints wrap below zero.
    // Rasterize lines and submit only visible playfield pixels.
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int stepX = x1 < x2 ? 1 : -1;
    int stepY = y1 < y2 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        if (x1 >= 0 && x1 < SCREEN_W && y1 > HUD_HEIGHT && y1 < SCREEN_H) {
            oled.drawPixel(x1, y1);
        }
        if (x1 == x2 && y1 == y2) break;
        int doubledError = 2 * error;
        if (doubledError >= dy) {
            error += dy;
            x1 += stepX;
        }
        if (doubledError <= dx) {
            error += dx;
            y1 += stepY;
        }
    }
}

void drawHomingMissiles(unsigned long now) {
    static const uint8_t missileRows[MISSILE_H] = {0x06, 0x3E, 0x7F, 0x3E, 0x06};
    for (int i = 0; i < MAX_HOMING_MISSILES; i++) {
        const HomingMissile &missile = homingMissiles[i];
        if (!missile.active) continue;
        int centerY = missile.yQ / ENEMY_BULLET_Y_SCALE;
        for (int row = 0; row < MISSILE_H; row++) {
            for (int col = 0; col < MISSILE_W; col++) {
                int pixelX = missile.x + col;
                int pixelY = centerY + row - MISSILE_H / 2;
                if (pixelX >= 0 && pixelX < SCREEN_W && pixelY > HUD_HEIGHT && pixelY < SCREEN_H &&
                    (missileRows[row] & (1U << (MISSILE_W - 1 - col)))) {
                    oled.drawPixel(pixelX, pixelY);
                }
            }
        }
        int trailX = missile.x + MISSILE_W + (now / 100UL + i) % 3;
        if (trailX >= 0 && trailX < SCREEN_W) oled.drawPixel(trailX, centerY);
    }
}

void drawBossCombat(unsigned long now) {
    if (!enemyIsBoss || !enemies[0].active || bossIntroActive(now)) return;
    const Enemy &boss = enemies[0];
    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    if (level == 3 || level == 5) {
        bool shieldUp = bossShieldActive(now);
        bool openingSoon = bossShieldOpeningSoon(now);
        if (shieldUp && (!openingSoon || (now / 90UL) % 2 == 0)) {
            const int shieldX = boss.x + enemyWidth() / 2;
            const int shieldY = boss.y + enemyHeight() / 2;
            const int shieldRadius = max(enemyWidth(), enemyHeight()) / 2 + 5;
            oled.drawCircle(shieldX, shieldY, shieldRadius);
            if (timerActive(bossShieldFlashUntil, now)) {
                oled.drawCircle(shieldX, shieldY, shieldRadius + 1);
            }
        }
    }
    if (bossCharging) {
        if (bossAttackKind == BOSS_ATTACK_LASER) {
            // The broken beam segments already telegraph a laser; no flashing boss frame needed.
            for (int x = 0; x < boss.x; x += 6) {
                if ((now / 120UL) % 2 == 0) {
                    int endX = min(x + 2, boss.x - 1);
                    drawPlayfieldLine(x, bossBeamYAt(x), endX, bossBeamYAt(endX));
                }
            }
        } else if (bossAttackKind == BOSS_ATTACK_MISSILES) {
            if ((now / 120UL) % 2 == 0) {
                const char *warning = "MISSILES!";
                oled.setFont(u8g2_font_4x6_tr);
                const int warningWidth = oled.getStrWidth(warning);
                const int warningX = constrain(playerX + PLAYER_W / 2 - warningWidth / 2,
                                                1, SCREEN_W - warningWidth - 1);
                int warningY = playerY - 3;
                if (warningY < HUD_HEIGHT + 7) warningY = playerY + PLAYER_H + 7;
                warningY = constrain(warningY, HUD_HEIGHT + 7, SCREEN_H - 8);
                oled.drawStr(warningX, warningY, warning);
                oled.setFont(u8g2_font_5x7_tr);
            }
        }
    }
    if (bossLaserFiring) {
        for (int offset = -1; offset <= 1; offset++) {
            drawPlayfieldLine(0, bossBeamYAt(0) + offset,
                              boss.x - 1, bossBeamYAt(boss.x - 1) + offset);
        }
        for (int x = (now / 35UL) % 10; x < boss.x; x += 10) {
            int beamY = bossBeamYAt(x);
            if (beamY - 2 > HUD_HEIGHT && beamY - 2 < SCREEN_H) oled.drawPixel(x, beamY - 2);
            if (beamY + 2 > HUD_HEIGHT && beamY + 2 < SCREEN_H) oled.drawPixel(x, beamY + 2);
        }
    }
    oled.setMaxClipWindow();
}

void drawGame() {
    oled.clearBuffer();
    drawHud();

    drawStars();
    unsigned long now = millis();
    bool playerExploding = timerActive(playerExplosionUntil, now);
    if (!playerExploding &&
        (!timerActive(invulnerableUntil, now) || ((now / 100) % 2 == 0))) {
        drawPlayer(playerX, playerY);
    }
    if (!playerExploding && timerActive(shieldUntil, now)) {
        // Larger bubble around the visual hull. Explicit clipping prevents
        // negative coordinates from wrapping when flying near screen edges.
        const int centerX = playerX + PLAYER_W / 2;
        const int centerY = playerY + PLAYER_H / 2;
        for (int dy = -PLAYER_SHIELD_RADIUS; dy <= PLAYER_SHIELD_RADIUS; dy++) {
            for (int dx = -PLAYER_SHIELD_RADIUS; dx <= PLAYER_SHIELD_RADIUS; dx++) {
                const int distanceSquared = dx * dx + dy * dy;
                const int pixelX = centerX + dx;
                const int pixelY = centerY + dy;
                if (distanceSquared <= PLAYER_SHIELD_RADIUS * PLAYER_SHIELD_RADIUS &&
                    distanceSquared > (PLAYER_SHIELD_RADIUS - 1) * (PLAYER_SHIELD_RADIUS - 1) &&
                    pixelX >= 0 && pixelX < SCREEN_W && pixelY > HUD_HEIGHT && pixelY < SCREEN_H) {
                    oled.drawPixel(pixelX, pixelY);
                }
            }
        }
    }
    for (int i = 0; i < MAX_SIMULTANEOUS_ENEMIES; i++) {
        if (enemies[i].active) drawEnemy(enemies[i]);
    }
    if (timerActive(laserUntil, now) && !bossIntroActive(now) && !playerExploding) {
        drawLaserBeam(now);
    }
    drawBossCombat(now);
    drawHomingMissiles(now);

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        if (bullets[i].verticalStep == 0) {
            oled.drawBox(bullets[i].x, bullets[i].y, 5, 1);
        } else {
            oled.drawLine(bullets[i].x, bullets[i].y,
                          bullets[i].x + 4, bullets[i].y + bullets[i].verticalStep);
        }
    }
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemyBullets[i].active) {
            int x = enemyBullets[i].x;
            int y = enemyBullets[i].yQ / ENEMY_BULLET_Y_SCALE;
            oled.drawPixel(x + 1, y);
            oled.drawPixel(x, y + 1);
            oled.drawPixel(x + 2, y + 1);
            oled.drawPixel(x + 1, y + 2);
        }
    }
    if (powerUp.active) {
        const uint32_t *iconRows = nullptr;
        if (powerUp.type == POWERUP_EXTRA_LIFE) iconRows = HEART_POWERUP_ROWS;
        else if (powerUp.type == POWERUP_SHIELD) iconRows = SHIELD_ICON_ROWS;
        else if (powerUp.type == POWERUP_SHOTGUN) iconRows = SHOTGUN_ICON_ROWS;
        else if (powerUp.type == POWERUP_LASER) iconRows = LASER_ICON_ROWS;
        if (iconRows) {
            const int width = powerUpIconWidth();
            const int height = powerUpIconHeight();
            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    int pixelX = powerUp.x + col;
                    int pixelY = powerUp.y + row;
                    if (pixelX >= 0 && pixelX < SCREEN_W && pixelY > HUD_HEIGHT && pixelY < SCREEN_H &&
                        (iconRows[row] & (1UL << (width - 1 - col)))) {
                        oled.drawPixel(pixelX, pixelY);
                    }
                }
            }
        }
    }

    drawExplosions(now);

    if (enemyIsBoss && enemies[0].active) {
        const int healthY = powerUpTimersActive(now) ? 49 : 55;
        oled.drawFrame(90, healthY, 30, 5);
        int healthWidth = map(enemies[0].health, 0, enemies[0].maxHealth, 0, 28);
        if (healthWidth > 0) oled.drawBox(91, healthY + 1, healthWidth, 3);
    }

    if (powerUpPopupType != POWERUP_NONE && timerActive(powerUpPopupUntil, now)) {
        const char *message = "";
        if (powerUpPopupType == POWERUP_EXTRA_LIFE) message = "EXTRA LIFE";
        else if (powerUpPopupType == POWERUP_SHIELD) message = "SHIELD";
        else if (powerUpPopupType == POWERUP_SHOTGUN) message = "SHOTGUN";
        else if (powerUpPopupType == POWERUP_LASER) message = "LASER!";

        const unsigned long elapsed = POWERUP_POPUP_DURATION_MS - (powerUpPopupUntil - now);
        const int rise = static_cast<int>(elapsed * 8 / POWERUP_POPUP_DURATION_MS);
        oled.setFont(u8g2_font_4x6_tr);
        int textWidth = oled.getStrWidth(message);
        const int textX = constrain(powerUpPopupAnchorX - textWidth / 2, 0, SCREEN_W - textWidth);
        const int baselineY = constrain(powerUpPopupAnchorY - 2 - rise, HUD_HEIGHT + 7, SCREEN_H - 1);
        int wipeX = textX;
        if (elapsed > POWERUP_POPUP_WIPE_START_MS) {
            const unsigned long wipeElapsed = elapsed - POWERUP_POPUP_WIPE_START_MS;
            const unsigned long wipeDuration = POWERUP_POPUP_DURATION_MS - POWERUP_POPUP_WIPE_START_MS;
            wipeX += textWidth * wipeElapsed / wipeDuration;
        }
        if (wipeX < textX + textWidth) {
            oled.setClipWindow(wipeX, baselineY - 6, textX + textWidth + 1, baselineY + 1);
            oled.drawStr(textX, baselineY, message);
            oled.setMaxClipWindow();
        }
    }

    drawPowerUpTimers(now);
    if (bossIntroActive(now)) {
        const char *title = "BOSS BATTLE";
        const char *name = BOSS_NAMES[level - 1];
        oled.setFont(u8g2_font_5x7_tr);
        int titleWidth = oled.getStrWidth(title);
        int nameWidth = oled.getStrWidth(name);
        int boxWidth = max(titleWidth, nameWidth) + 8;
        int boxX = (SCREEN_W - boxWidth) / 2;
        int boxY = 18;
        oled.drawBox(boxX, boxY, boxWidth, 20);
        oled.setDrawColor(0);
        oled.drawStr((SCREEN_W - titleWidth) / 2, boxY + 8, title);
        oled.drawStr((SCREEN_W - nameWidth) / 2, boxY + 17, name);
        oled.setDrawColor(1);
    }

    oled.sendBuffer();
}
