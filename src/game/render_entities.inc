// Drawing code for the player, enemies, projectiles, and laser beam.
void drawPlayerEngineFlame(int nozzleX, int centerY, int frame) {
    // Three-pixel-high exhaust: a solid core, ragged edges and drifting sparks.
    // Bit zero is nearest the nozzle; the flame tapers to the left.
    static const uint8_t flameFrames[4][3] = {
        {0x07, 0x1F, 0x03},
        {0x0B, 0x3F, 0x07},
        {0x03, 0x7F, 0x0B},
        {0x07, 0x3F, 0x05}
    };
    for (int row = 0; row < 3; row++) {
        for (int distance = 0; distance < 7; distance++) {
            int pixelX = nozzleX - 1 - distance;
            if (pixelX >= 0 && (flameFrames[frame][row] & (1U << distance))) {
                oled.drawPixel(pixelX, centerY + row - 1);
            }
        }
    }
    if (frame == 1 || frame == 3) {
        int sparkX = nozzleX - (frame == 1 ? 8 : 9);
        if (sparkX >= 0) {
            oled.drawPixel(sparkX, centerY + (frame == 1 ? -1 : 1));
        }
    }
}

void drawPlayer(int x, int y) {
    const unsigned long now = millis();
    const int frame = (now / 80UL) % 4;
    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    for (int row = 0; row < PLAYER_ART_H; row++) {
        for (int col = 0; col < PLAYER_ART_W; col++) {
            int pixelX = x + col - 2;
            int pixelY = y + row - 2;
            if (pixelX >= 0 && pixelX < SCREEN_W && pixelY > HUD_HEIGHT && pixelY < SCREEN_H &&
                (PLAYER_SPRITE[row] & (1U << (PLAYER_ART_W - 1 - col)))) {
                oled.drawPixel(pixelX, pixelY);
            }
        }
    }
    // Offset the two engine animations so they do not flicker in unison.
    drawPlayerEngineFlame(x - 1, y + 1, frame);
    drawPlayerEngineFlame(x - 1, y + 7, (frame + 2) % 4);
    // A small glint inside the dark cockpit, without filling its silhouette.
    oled.drawPixel(x + 6 + ((now / 320UL) % 2), y + 4);
    oled.setMaxClipWindow();
}

void drawOmegaPrime(int x, int y, unsigned long now) {
    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    for (int row = 0; row < OMEGA_PRIME_H; row++) {
        for (int col = 0; col < OMEGA_PRIME_W; col++) {
            int pixelX = x + col;
            int pixelY = y + row;
            if (pixelX >= 0 && pixelX < SCREEN_W && pixelY > HUD_HEIGHT && pixelY < SCREEN_H &&
                (OMEGA_PRIME_SPRITE[row] & (1UL << (OMEGA_PRIME_W - 1 - col)))) {
                oled.drawPixel(pixelX, pixelY);
            }
        }
    }
    // Blink both eyes together. Face animation never shares the reactor overlay.
    if (now % 3200UL < 100UL) {
        oled.drawBox(x + 10, y + 8, 3, 3);
        oled.drawBox(x + 16, y + 8, 3, 3);
        oled.setDrawColor(0);
        drawPlayfieldLine(x + 10, y + 9, x + 12, y + 9);
        drawPlayfieldLine(x + 16, y + 9, x + 18, y + 9);
        oled.setDrawColor(1);
    }
    oled.setDrawColor(0);
    bool mouthOpen = bossLaserFiring || (bossCharging && bossAttackKind == BOSS_ATTACK_LASER);
    oled.drawBox(x + 13, y + 14, 3, mouthOpen ? 2 : 1);
    oled.drawBox(x + 6, y + 17, 16, 1);
    oled.setDrawColor(1);
    // Running saucer lights accelerate in the final combat phase.
    int lampPhase = (now / (bossEnraged ? 90UL : 180UL)) % 3;
    for (int lamp = 0; lamp < 3; lamp++) {
        oled.drawBox(x + 7 + lamp * 6, y + 17, lamp == lampPhase ? 2 : 1, 1);
    }
    if ((now / 260UL) % 2 == 0) {
        oled.drawPixel(x + 7, y + 8);
        oled.drawPixel(x + 6, y + 9);
    }
    if (!bossIntroActive(now) && now - enemies[0].lastShot < 100UL) {
        int muzzleY = enemyWeaponY(y);
        drawPlayfieldLine(x - 4, muzzleY, x - 1, muzzleY);
    }
    oled.setMaxClipWindow();
}

void drawEnemyDamageEffect(const Enemy &enemy, unsigned long now) {
    if (!enemy.active || !timerActive(enemy.hitFlashUntil, now)) return;
    const unsigned long elapsed = ENEMY_HIT_FLASH_MS - (enemy.hitFlashUntil - now);
    if ((elapsed / 55UL) % 2 != 0) return;
    const int width = enemyWidth();
    const int height = enemyHeight();
    // Invert a rounded patch of the hull for two short impact pulses.
    // Signed, clipped pixels also work while ships enter or leave the screen.
    oled.setDrawColor(2);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            const int dx = col * 2 - (width - 1);
            const int dy = row * 2 - (height - 1);
            if (dx * dx * height * height + dy * dy * width * width >
                width * width * height * height) continue;
            const int px = enemy.x + col;
            const int py = enemy.y + row;
            if (px >= 0 && px < SCREEN_W && py > HUD_HEIGHT && py < SCREEN_H)
                oled.drawPixel(px, py);
        }
    }
    oled.setDrawColor(1);
}

uint16_t scoutSpriteRow(int row, unsigned long now, int phase) {
    uint16_t bits = SCOUT_SPRITE[row];
    if ((now / 200UL + phase) % 2 != 0) {
        if (row == 0 || row == 7) bits = 0b0000110100;
        if (row == 1 || row == 6) bits = 0b0001110010;
    }
    if ((row == 2 || row == 5) &&
        (now + static_cast<unsigned long>(phase) * 80UL) % 2200UL < 100UL)
        bits |= 0b0000100000; // Blink both rotated eyes without changing the nose.
    return bits;
}

void drawScout(const Enemy &enemy, unsigned long now) {
    for (int row = 0; row < ENEMY_H; row++) {
        const uint16_t bits = scoutSpriteRow(row, now, enemy.y / 3);
        for (int col = 0; col < ENEMY_W; col++) {
            if (bits & (1U << (ENEMY_W - 1 - col)))
                drawPlayfieldLine(enemy.x + col, enemy.y + row, enemy.x + col, enemy.y + row);
        }
    }
}

uint16_t standardSpriteRow(int row, unsigned long now, int phase) {
    const bool alternate = (now / 220UL + phase) % 2 != 0;
    if (row == 0 && alternate) return 0b01000000010;
    if (row == 7 && alternate) return 0b00110001100;
    if (row == 3 && (now + static_cast<unsigned long>(phase) * 80UL) % 2600UL < 100UL)
        return 0b01111111110; // Brief eye blink.
    return STANDARD_SPRITE[row];
}

void drawStandardEnemy(const Enemy &enemy, unsigned long now) {
    for (int row = 0; row < ENEMY_H; row++) {
        const uint16_t bits = standardSpriteRow(row, now, enemy.y / 3);
        for (int col = 0; col < STANDARD_ART_W; col++) {
            if (bits & (1U << (STANDARD_ART_W - 1 - col)))
                drawPlayfieldLine(enemy.x + col, enemy.y + row, enemy.x + col, enemy.y + row);
        }
    }
}

uint16_t gunnerSpriteRow(int row, unsigned long now, int phase) {
    uint16_t bits = GUNNER_SPRITE[row];
    // The pilot blinks briefly, independently of the rotating rim lights.
    if (row == 2 && (now + static_cast<unsigned long>(phase) * 80UL) % 2400UL < 110UL)
        bits = GUNNER_SPRITE[1];
    if (row == 7) {
        const int lamp = (now / 140UL + phase) % 4;
        bits |= 1U << (GUNNER_ART_W - 1 - (1 + lamp * 3));
    }
    return bits;
}

void drawGunner(const Enemy &enemy, unsigned long now) {
    // Center the art over the existing hitbox; no movement or collision changes.
    const int left = enemy.x - (GUNNER_ART_W - ENEMY_W) / 2;
    const int top = enemy.y - (GUNNER_ART_H - ENEMY_H) / 2;
    for (int row = 0; row < GUNNER_ART_H; row++) {
        const uint16_t bits = gunnerSpriteRow(row, now, enemy.y / 3);
        for (int col = 0; col < GUNNER_ART_W; col++) {
            if (bits & (1U << (GUNNER_ART_W - 1 - col)))
                drawPlayfieldLine(left + col, top + row, left + col, top + row);
        }
    }
}

void drawEnemy(const Enemy &enemy) {
    const int x = enemy.x;
    const int y = enemy.y;
    int spriteIndex = constrain(level - 1, 0, MAX_LEVELS - 1);
    int scale = enemyScale();
    const unsigned long now = millis();
    if (!enemyIsBoss && enemy.type == ENEMY_STANDARD) {
        drawStandardEnemy(enemy, now);
        drawEnemyDamageEffect(enemy, now);
        return;
    }
    if (!enemyIsBoss && enemy.type == ENEMY_SCOUT) {
        drawScout(enemy, now);
        drawEnemyDamageEffect(enemy, now);
        return;
    }
    if (!enemyIsBoss && enemy.type == ENEMY_GUNNER) {
        drawGunner(enemy, now);
        drawEnemyDamageEffect(enemy, now);
        return;
    }
    if (enemyIsBoss && level == 5) {
        drawOmegaPrime(x, y, now);
        drawEnemyDamageEffect(enemy, now);
        return;
    }
    const int frame = enemyIsBoss
                          ? (now / (140UL - spriteIndex * 10UL)) % 4
                          : (now / 160UL + static_cast<unsigned long>(y / 3)) % 4;
    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    for (int row = 0; row < ENEMY_H; row++) {
        uint16_t rowBits = enemyIsBoss && level == 4
                               ? STAR_WRAITH_BOSS_SPRITE[row] : ENEMY_SPRITES[spriteIndex][row];
        for (int col = 0; col < ENEMY_W; col++) {
            if (rowBits & (1U << (ENEMY_W - 1 - col))) {
                int animatedCol = col;
                if (frame >= 2 &&
                    (row < 2 || (row >= ENEMY_H - 2 && !(enemyIsBoss && level == 4)))) {
                    // Fold the upper and lower fins inward without moving the hull.
                    animatedCol = col < ENEMY_W / 2
                                       ? min(col + 1, ENEMY_W / 2 - 1)
                                       : max(col - 1, ENEMY_W / 2);
                }
                if (enemyIsBoss) {
                    oled.drawBox(x + animatedCol * scale, y + row * scale, scale, scale);
                } else {
                    drawPlayfieldLine(x + animatedCol, y + row, x + animatedCol, y + row);
                }
            }
        }
    }
    if (enemyIsBoss) {
        const int centerX = x + enemyWidth() / 2;
        const int centerY = y + enemyHeight() / 2;
        if (level == 4) {
            // Keep the core overlay off this face; animate both eyes together.
            oled.setDrawColor(0);
            oled.drawBox(x + 6, y + 6, 2, 2);
            oled.drawBox(x + 12, y + 6, 2, 2);
            oled.drawBox(x + 6, y + 12, 8, bossCharging || bossLaserFiring ? 3 : 2);
            oled.setDrawColor(1);
            if (frame == 1 || frame == 2) {
                oled.drawPixel(x + 7, y + 7);
                oled.drawPixel(x + 12, y + 7);
            }
        } else {
            // Pulse the energy core in four frames.
            oled.setDrawColor(0);
            oled.drawBox(centerX - 2, centerY - 2, 5, 5);
            oled.setDrawColor(1);
            oled.drawFrame(centerX - 2, centerY - 2, 5, 5);
            bool coreLit = bossShieldOpeningSoon(now) ? (now / 90UL) % 2 == 0
                           : (level == 3 && !bossShieldActive(now)) || frame == 1 || frame == 2;
            if (coreLit) {
                oled.drawBox(centerX - 1, centerY - 1, 3, 3);
            } else {
                oled.drawPixel(centerX, centerY);
            }
        }
        if (!bossIntroActive(now) && now - enemies[0].lastShot < 100UL) {
            oled.drawLine(max(0, x - 4), centerY, max(0, x - 1), centerY);
            oled.drawPixel(max(0, x - 2), centerY - 1);
            oled.drawPixel(max(0, x - 2), centerY + 1);
        }
    } else {
        // A scanning sensor blinks inside the small fighter's hull.
        oled.setDrawColor(0);
        oled.drawBox(x + 3, y + 3, 3, 2);
        oled.setDrawColor(1);
        if (frame != 3) {
            drawPlayfieldLine(x + 3 + frame, y + 3, x + 3 + frame, y + 3);
            if (frame == 1) drawPlayfieldLine(x + 4, y + 4, x + 4, y + 4);
        }
    }
    drawEnemyDamageEffect(enemy, now);
    oled.setMaxClipWindow();
}

void drawLaserBeam(unsigned long now) {
    const int startX = playerX + PLAYER_W;
    const int beamY = playerY + 4;
    const int frame = (now / 70UL) % 4;
    const int pulseOffset = (now / 35UL) % 12;
    const bool widePulse = frame == 1 || frame == 2;
    oled.setClipWindow(0, HUD_HEIGHT + 1, SCREEN_W, SCREEN_H);
    // A steady damage core with traveling energy on either side.
    oled.drawLine(startX, beamY, SCREEN_W - 1, beamY);
    for (int pulseX = startX + pulseOffset; pulseX < SCREEN_W; pulseX += 12) {
        oled.drawLine(pulseX, beamY - 1, min(pulseX + 3, SCREEN_W - 1), beamY - 1);
        if (widePulse && pulseX + 1 < SCREEN_W) {
            oled.drawPixel(pulseX + 1, beamY - 2);
        }
        int lowerX = pulseX - 6;
        if (lowerX >= startX) {
            oled.drawLine(lowerX, beamY + 1, min(lowerX + 3, SCREEN_W - 1), beamY + 1);
            if (widePulse) oled.drawPixel(lowerX + 2, beamY + 2);
        }
    }
    // The emitter pulses as the beam leaves the ship.
    oled.drawLine(startX, beamY - (widePulse ? 2 : 1),
                  startX, beamY + (widePulse ? 2 : 1));
    oled.setMaxClipWindow();
}
