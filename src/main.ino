#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Tone32.h>
#include <Preferences.h>
#include <math.h>
#include <stdio.h>
// Game modules are included in one translation unit to share state safely.
#include "game/game_state.inc"
#include "game/game_prototypes.inc"
#include "game/game_world.inc"
#include "game/game_progression.inc"
#include "game/render_entities.inc"
#include "game/render_hud.inc"
#include "game/ui_screens.inc"
#include "game/combat.inc"
#include "game/game_updates.inc"

void setup() {
    pinMode(JOYSTICK_X_PIN, INPUT);
    pinMode(JOYSTICK_Y_PIN, INPUT);
    pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
    analogReadResolution(12);
    analogSetPinAttenuation(JOYSTICK_X_PIN, ADC_11db);
    analogSetPinAttenuation(JOYSTICK_Y_PIN, ADC_11db);
    calibrateJoystick();
    joystickButtonRaw = digitalRead(JOYSTICK_SW_PIN) == LOW;
    joystickButtonDown = joystickButtonRaw;
    joystickButtonChangedAt = millis();
    pinMode(BUZZER_PIN, OUTPUT);

    Wire.begin(OLED_SDA, OLED_SCL);
    oled.setI2CAddress(OLED_ADDRESS << 1);
    oled.begin();

    loadHighScore();
    randomSeed(micros());
    resetGame();
    drawStartScreen();
}

void loop() {
    updateJoystickButton();
    if (!gameStarted) {
        if (joystickClickEvent) {
            gameStarted = true;
            resetGame();
            delay(250);
        }
        return;
    }

    if (gameOver) {
        unsigned long now = millis();
        if (timerActive(playerExplosionUntil, now)) {
            oled.clearBuffer();
            drawStars();
            drawExplosionFrame(playerExplosionX, playerExplosionY, 1,
                               now - playerExplosionStartedAt);
            oled.sendBuffer();
        } else {
            drawGameOver();
        }
        if (!timerActive(playerExplosionUntil, now) && joystickClickEvent) {
            gameStarted = false;
            delay(300);
            drawStartScreen();
        }
        return;
    }

    if (levelVictory) {
        unsigned long now = millis();
        if (timerActive(victoryExplosionUntil, now)) {
            drawExplosionScene(now);
            return;
        }
        if (!endlessMode && level >= MAX_LEVELS) {
            int axisY = joystickDirection(readJoystickAxis(JOYSTICK_Y_PIN), joystickCenterY, INVERT_JOYSTICK_Y);
            chooseVictoryAction(axisY); // Rotated screen-horizontal input: left/right selection.
        }
        drawVictoryScreen();
        if (victoryNeedsRelease) {
            if (!joystickButtonDown) victoryNeedsRelease = false;
        } else if (joystickClickEvent) {
            confirmVictoryAction();
            delay(250);
        }
        return;
    }

    unsigned long now = millis();
    if (!timerActive(playerExplosionUntil, now) &&
        now - lastPlayerMove >= PLAYER_MOVE_DELAY) {
        lastPlayerMove = now;
        updatePlayer();
    }
    if (now - lastMove >= MOVE_DELAY) {
        lastMove = now;
        updateStars();
        if (!timerActive(playerExplosionUntil, now)) updatePowerUp(now);
        updateBullets();
        updateHomingMissiles(now);
        updateEnemyBullets();
        spawnPendingEnemies(now);
    }
    if (now - lastEnemyMove >= ENEMY_MOVE_DELAY) {
        lastEnemyMove = now;
        updateEnemies();
    }

    // Automatic fire leaves the joystick free for movement and menu clicks.
    if (!gameOver && !levelVictory && !bossIntroActive(now) && !timerActive(playerExplosionUntil, now) &&
        now - lastShot >= SHOT_DELAY) {
        lastShot = now;
        shoot();
    }

    updateLaser(now);
    updateEnemyShooting(now);

    drawGame();
    delay(8);
}
