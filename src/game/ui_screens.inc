// Start, victory, and game-over screen drawing.
void drawUiCentered(const char *text, int baseline) {
    oled.drawStr((SCREEN_W - oled.getStrWidth(text)) / 2, baseline, text);
}

void drawUiBackdrop() {
    oled.clearBuffer();
    oled.setDrawColor(1);
    // Small corner brackets, rather than a heavy frame around the entire screen.
    for (int side = 0; side < 2; side++) {
        const int x = side ? SCREEN_W - 2 : 1;
        const int endX = side ? SCREEN_W - 7 : 6;
        oled.drawLine(x, 1, endX, 1);
        oled.drawLine(x, 1, x, 6);
        oled.drawLine(x, SCREEN_H - 2, endX, SCREEN_H - 2);
        oled.drawLine(x, SCREEN_H - 7, x, SCREEN_H - 2);
    }
    // Decorative stars stay at the margins, away from all text and controls.
    const int starY = 17 + (millis() / 500UL) % 3;
    oled.drawPixel(3, starY);
    oled.drawPixel(124, 42 - (millis() / 500UL) % 3);
}

void drawUiButton(int x, int y, int width, const char *label, bool selected) {
    oled.setFont(u8g2_font_5x7_tr);
    if (selected) {
        oled.drawBox(x, y, width, 12);
        oled.setDrawColor(0);
    } else {
        oled.drawFrame(x, y, width, 12);
    }
    oled.drawStr(x + (width - oled.getStrWidth(label)) / 2, y + 9, label);
    oled.setDrawColor(1);
}

void drawUiPrompt(const char *label) {
    drawUiButton(14, 51, 100, label, true);
    // A tiny moving chevron makes the click affordance feel alive without
    // blinking the instruction itself off the screen.
    const int x = 5 + (millis() / 250UL) % 2;
    oled.drawLine(x, 54, x + 2, 56);
    oled.drawLine(x + 2, 56, x, 58);
}

void drawStartScreen() {
    drawUiBackdrop();
    oled.setFont(u8g2_font_4x6_tr);
    drawUiCentered("ESP32 SPACE ARCADE", 9);
    oled.setFont(u8g2_font_10x20_tr);
    drawUiCentered("SPACEWARS", 29);
    oled.drawLine(20, 32, 107, 32);
    char text[24];
    snprintf(text, sizeof(text), "BEST %d", highScore);
    oled.setFont(u8g2_font_4x6_tr);
    oled.drawStr(16, 44, text);
    const int shipY = 38 + (millis() / 350UL) % 2;
    drawPlayer(96, shipY);
    drawUiPrompt("CLICK TO START");
    oled.sendBuffer();
}

void drawGameOver() {
    drawUiBackdrop();
    oled.setFont(u8g2_font_4x6_tr);
    drawUiCentered("SHIP LOST / TRY AGAIN", 9);
    oled.setFont(u8g2_font_10x20_tr);
    drawUiCentered("GAME OVER", 28);
    const char *labels[2] = {"SCORE", "BEST"};
    const int values[2] = {score, highScore};
    for (int i = 0; i < 2; i++) {
        const int x = 8 + i * 58;
        oled.drawFrame(x, 32, 54, 16);
        oled.setFont(u8g2_font_4x6_tr);
        oled.drawStr(x + 3, 39, labels[i]);
        char value[16];
        snprintf(value, sizeof(value), "%d", values[i]);
        // Small font keeps even a full 32-bit score inside its card.
        oled.drawStr(x + 3, 46, value);
    }
    drawUiPrompt("CLICK FOR MENU");
    oled.sendBuffer();
}

void drawUiVictoryBadge() {
    // Trophy with alternating celebratory sparks, drawn above the content.
    oled.drawBox(9, 7, 7, 5);
    oled.drawFrame(6, 7, 3, 4);
    oled.drawFrame(16, 7, 3, 4);
    oled.drawLine(12, 12, 12, 15);
    oled.drawLine(9, 16, 15, 16);
    const bool sparkle = (millis() / 240UL) % 2 == 0;
    oled.drawPixel(sparkle ? 22 : 5, sparkle ? 5 : 14);
    oled.drawPixel(sparkle ? 106 : 120, sparkle ? 7 : 15);
}

void drawVictoryScreen() {
    drawUiBackdrop();
    drawUiVictoryBadge();
    char text[32];
    if (!endlessMode && level >= MAX_LEVELS) {
        oled.setFont(u8g2_font_10x20_tr);
        drawUiCentered("YOU WIN!", 30);
        oled.setFont(u8g2_font_4x6_tr);
        drawUiCentered("ALL 5 LEVELS CLEARED", 38);
        drawUiButton(6, 41, 56, "ENDLESS", victoryEndlessSelected);
        drawUiButton(66, 41, 56, "RESTART", !victoryEndlessSelected);
        oled.setFont(u8g2_font_4x6_tr);
        drawUiCentered("LEFT/RIGHT + CLICK", 61);
    } else {
        oled.setFont(u8g2_font_5x7_tr);
        drawUiCentered("VICTORY!", 14);
        oled.drawLine(28, 18, 99, 18);
        oled.setFont(u8g2_font_4x6_tr);
        drawUiCentered(BOSS_NAMES[level - 1], 28);
        snprintf(text, sizeof(text), "%s %d CLEARED", endlessMode ? "WAVE" : "LEVEL",
                 endlessMode ? endlessWave : level);
        oled.setFont(u8g2_font_5x7_tr);
        drawUiCentered(text, 38);
        snprintf(text, sizeof(text), "SCORE %d", score);
        oled.setFont(u8g2_font_4x6_tr);
        drawUiCentered(text, 47);
        drawUiPrompt("CLICK FOR NEXT");
    }
    oled.sendBuffer();
}
