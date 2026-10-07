// High score storage, joystick input, resets, and level progression.
void loadHighScore() {
    highScoreStore.begin("spacewars", true);
    highScore = highScoreStore.getInt("best", 0);
    highScoreStore.end();
}

void saveHighScore() {
    if (score <= highScore) return;
    highScore = score;
    highScoreStore.begin("spacewars", false);
    highScoreStore.putInt("best", highScore);
    highScoreStore.end();
}

void calibrateJoystick() {
    // Leave the stick centered during startup. Average samples to reject noise.
    int sumX = 0;
    int sumY = 0;
    for (int sample = 0; sample < 32; sample++) {
        sumX += analogRead(JOYSTICK_X_PIN);
        sumY += analogRead(JOYSTICK_Y_PIN);
        delay(2);
    }
    int centerX = sumX / 32;
    int centerY = sumY / 32;
    // Do not learn an extreme position if the stick is held during startup.
    joystickCenterX = centerX >= 1024 && centerX <= 3072 ? centerX : 2048;
    joystickCenterY = centerY >= 1024 && centerY <= 3072 ? centerY : 2048;
}

int readJoystickAxis(uint8_t pin) {
    int sum = 0;
    for (int sample = 0; sample < 4; sample++) sum += analogRead(pin);
    return sum / 4;
}

int joystickDirection(int value, int center, bool inverted) {
    int delta = value - center;
    int direction = delta > JOYSTICK_DEAD_ZONE ? 1
                    : delta < -JOYSTICK_DEAD_ZONE ? -1 : 0;
    return inverted ? -direction : direction;
}

void updateJoystickButton() {
    unsigned long now = millis();
    bool rawPressed = digitalRead(JOYSTICK_SW_PIN) == LOW;
    joystickClickEvent = false;
    if (rawPressed != joystickButtonRaw) {
        joystickButtonRaw = rawPressed;
        joystickButtonChangedAt = now;
    }
    if (now - joystickButtonChangedAt >= JOYSTICK_DEBOUNCE_MS &&
        joystickButtonDown != joystickButtonRaw) {
        joystickButtonDown = joystickButtonRaw;
        joystickClickEvent = joystickButtonDown;
    }
}

void clearObjects() {
    resetBossCombat();
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i].active = false;
    powerUp.active = false;
}

void resetStars() {
    lastStarMove = millis();
    for (int i = 0; i < MAX_STARS; i++) {
        starsX[i] = random(0, SCREEN_W);
        starsY[i] = random(HUD_HEIGHT + 2, SCREEN_H);
    }
}

void resetGame() {
    endlessMode = false;
    endlessWave = 0;
    victoryEndlessSelected = true;
    comboKills = 0;
    playerX = 12;
    playerY = 34;
    playerMoveRemainder = 0;
    score = 0;
    level = START_LEVEL;
    lives = 3;
    startLevelWave();
    clearObjects();
    resetStars();
    clearExplosions();
    lastMove = millis();
    lastEnemyMove = millis();
    lastPlayerMove = millis();
    lastShot = millis();
    playerExplosionUntil = 0;
    victoryExplosionUntil = 0;
    dashUntil = 0;
    dashInvulnerableUntil = 0;
    dashTrailUntil = 0;
    dodgeCooldownUntil = 0;
    gameOver = false;
    levelVictory = false;
    victoryNeedsRelease = false;
    invulnerableUntil = 0;
    shieldUntil = 0;
    shotgunUntil = 0;
    laserUntil = 0;
    lastLaserDamage = 0;
}

void startNextLevel() {
    if (endlessMode) {
        endlessWave = min(endlessWave + 1, 1000000);
        level = (endlessWave - 1) % MAX_LEVELS + 1;
    } else {
        if (level >= MAX_LEVELS) return;
        level++;
    }
    playerX = 12;
    playerY = 34;
    playerMoveRemainder = 0;
    startLevelWave();
    clearObjects();
    resetStars();
    clearExplosions();
    lastMove = millis();
    lastEnemyMove = millis();
    lastPlayerMove = millis();
    lastShot = millis();
    playerExplosionUntil = 0;
    victoryExplosionUntil = 0;
    dashUntil = 0;
    dashInvulnerableUntil = 0;
    dashTrailUntil = 0;
    levelVictory = false;
    victoryNeedsRelease = false;
    invulnerableUntil = 0;
    shieldUntil = 0;
    shotgunUntil = 0;
    laserUntil = 0;
    lastLaserDamage = 0;
}

void startEndlessMode() {
    endlessMode = true;
    endlessWave = 0;
    startNextLevel();
}

void chooseVictoryAction(int moveX) {
    if (moveX < 0) victoryEndlessSelected = true;
    if (moveX > 0) victoryEndlessSelected = false;
}

void confirmVictoryAction() {
    if (endlessMode || level < MAX_LEVELS) startNextLevel();
    else if (victoryEndlessSelected) startEndlessMode();
    else resetGame();
}
