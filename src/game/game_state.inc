// Constants, sprite data, types, and shared game state.

// 1.3" 128x64 I2C OLED, normalt med SH1106-driver.
const uint8_t OLED_SDA = 21;
const uint8_t OLED_SCL = 22;
const uint8_t OLED_ADDRESS = 0x3C;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
    U8G2_R0,
    U8X8_PIN_NONE
);

// HW-504: VCC -> 3V3, GND -> GND, VRx -> GPIO32, VRy -> GPIO33, SW -> GPIO25.
const uint8_t JOYSTICK_X_PIN = 32;
const uint8_t JOYSTICK_Y_PIN = 33;
const uint8_t JOYSTICK_SW_PIN = 25;
const int JOYSTICK_DEAD_ZONE = 600;
const bool INVERT_JOYSTICK_X = false;
const bool INVERT_JOYSTICK_Y = false;
const unsigned long JOYSTICK_DEBOUNCE_MS = 25;
int joystickCenterX = 2048;
int joystickCenterY = 2048;
bool joystickButtonRaw = false;
bool joystickButtonDown = false;
bool joystickClickEvent = false;
unsigned long joystickButtonChangedAt = 0;

const uint8_t BUZZER_PIN = 27;
const uint8_t BUZZER_CHANNEL = 0;

const int SCREEN_W = 128;
const int SCREEN_H = 64;
const int HUD_HEIGHT = 10;
const int PLAYER_W = 12;
const int PLAYER_H = 9;
const int MAX_LEVELS = 5;
const int START_LEVEL = 1;
const int ENEMY_W = 10;
const int ENEMY_H = 8;
const int BOSS_SCALE = 2;
const int OMEGA_PRIME_W = 28;
const int OMEGA_PRIME_H = 24;
const int OMEGA_PRIME_WEAPON_ROW = 18;
const int MAX_SIMULTANEOUS_ENEMIES = 3;
const int REGULAR_ENEMIES_BY_LEVEL[MAX_LEVELS] = {4, 6, 8, 10, 12};
const int SMALL_ENEMY_HEALTH = 1;
const int GUNNER_HEALTH = 3;
const unsigned long ENEMY_HIT_FLASH_MS = 220;
const int SCOUT_SPEED = 6;
const int BOSS_HEALTH_BY_LEVEL[MAX_LEVELS] = {24, 36, 36, 44, 54};
const int SMALL_ENEMY_SPEED_BY_LEVEL[MAX_LEVELS] = {1, 1, 2, 2, 3};
const int MAX_LIVES = 5;
const int MAX_BULLETS = 18;
const int MAX_ENEMY_BULLETS = 5;
const int MAX_HOMING_MISSILES = 2;
const int MISSILE_W = 7;
const int MISSILE_H = 5;
const int MISSILE_SPEED_MULTIPLIER = 3;
const int MISSILE_SPEED_PERCENT = 85;
const int MAX_STARS = 8;
const unsigned long STAR_MOVE_DELAY = 140;
const int STAR_PROJECTILE_MARGIN = 3;
const int MAX_EXPLOSIONS = MAX_SIMULTANEOUS_ENEMIES + 2;
const int PLAYER_BULLET_Y_SCALE = 2;
const int ENEMY_BULLET_Y_SCALE = 16;
const uint8_t HEART_ICON_ROWS[7] = {0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00};
// Keep the small HUD heart; pickups use larger, matching 13x14 silhouettes.
const int LARGE_POWERUP_W = 13;
const int LARGE_POWERUP_H = 14;
const uint32_t HEART_POWERUP_ROWS[LARGE_POWERUP_H] = {
    0x0UL, 0xF1EUL, 0x1FBFUL, 0x1FFFUL, 0x1FFFUL, 0x1FFFUL, 0x1FFFUL, 0xFFEUL, 0x7FCUL, 0x3F8UL, 0x1F0UL, 0xE0UL, 0x40UL, 0x0UL
};
const uint32_t LASER_ICON_ROWS[LARGE_POWERUP_H] = {
    0x1CUL, 0x38UL, 0x70UL, 0xE0UL, 0x1C0UL, 0x380UL, 0x7FCUL, 0x78UL, 0xF0UL, 0x1E0UL, 0x3C0UL, 0x700UL, 0xC00UL, 0x1000UL
};
const int SHIELD_ICON_W = 13;
const int SHIELD_ICON_H = 14;
// Hollow heraldic shield with raised center crest and a pointed bottom.
const uint32_t SHIELD_ICON_ROWS[SHIELD_ICON_H] = {
    0x0040, 0x00E0, 0x19B3, 0x171D, 0x1001, 0x1001, 0x1001,
    0x0802, 0x0802, 0x0404, 0x0208, 0x0110, 0x00A0, 0x0040
};
const int SHOTGUN_ICON_W = 18;
const int SHOTGUN_ICON_H = 8;
// Selected reference-inspired shotgun: full stock, long barrel, pump and trigger guard.
const uint32_t SHOTGUN_ICON_ROWS[SHOTGUN_ICON_H] = {
    0x0UL, 0x400UL, 0x1FFFUL, 0x7E01UL, 0x3FEFFUL, 0x3F500UL, 0x3C700UL, 0x30000UL
};

// Right-facing combat fighter: swept wings without side guns, separate
// engine nacelles, a narrow fuselage and a dark cockpit. The central 12x9
// collision hull stays unchanged; the decorative wings extend two pixels.
const int PLAYER_ART_W = 16;
const int PLAYER_ART_H = 13;
const int PLAYER_SHIELD_RADIUS = 11;
const uint16_t PLAYER_SPRITE[PLAYER_ART_H] = {
    0x0000, 0x1C00, 0x3800, 0x7C00, 0x4FE0, 0x3CFC, 0x1D1F,
    0x3CFC, 0x4FE0, 0x7C00, 0x3800, 0x1C00, 0x0000
};

// Legacy masks retained for boss designs; standard enemies use one shared alien.
const uint16_t ENEMY_SPRITES[MAX_LEVELS][ENEMY_H] = {
    {0x0FC, 0x1FE, 0x3B7, 0x3FF, 0x0DC, 0x1B6, 0x303, 0x102},
    {0x102, 0x084, 0x1FE, 0x36B, 0x3FF, 0x0FC, 0x16A, 0x285},
    {0x0FC, 0x1FE, 0x3FF, 0x333, 0x3FF, 0x1FE, 0x0FC, 0x16A},
    {0x222, 0x124, 0x0F8, 0x1FC, 0x36E, 0x3FF, 0x0F8, 0x1B6},
    {0x0FC, 0x1FE, 0x3FF, 0x357, 0x3FF, 0x2FD, 0x1B6, 0x303}
};

// Star Wraith's face: two separate eye sockets and a mouth below the cheek row.
const uint16_t STAR_WRAITH_BOSS_SPRITE[ENEMY_H] = {
    0x102, 0x0FC, 0x1FE, 0x1B6, 0x1FE, 0x0FC, 0x084, 0x078
};

// Reference-inspired UFO: glass dome, alien face, wide saucer and landing pods.
// Native 28x24 pixels keep both eyes and the low mouth distinct on the OLED.
const uint32_t OMEGA_PRIME_SPRITE[OMEGA_PRIME_H] = {
    0x0002000UL,
    0x0007000UL,
    0x0002000UL,
    0x003FE00UL,
    0x00C0180UL,
    0x0138040UL,
    0x02CF820UL,
    0x02BFE20UL,
    0x044F910UL,
    0x0457510UL,
    0x0467310UL,
    0x047FF10UL,
    0x043DE10UL,
    0x021FC20UL,
    0x0208820UL,
    0x01FFFC0UL,
    0x0FFFFF0UL,
    0x3C820BCUL,
    0x7FFFFFEUL,
    0xC7801E3UL,
    0x7FFFFFEUL,
    0x0FFFFF0UL,
    0x00FFF00UL,
    0x0060600UL
};

const char *BOSS_NAMES[MAX_LEVELS] = {
    "DARK COMET",
    "VOID STALKER",
    "NEBULA QUEEN",
    "STAR WRAITH",
    "OMEGA PRIME"
};

const unsigned long ENEMY_SHOT_INTERVAL_MS[MAX_LEVELS] = {
    3750UL, 3000UL, 2500UL, 2250UL, 2000UL
};

struct Bullet {
    int x;
    int y;
    int yQ;
    int8_t verticalStep;
    bool active;
};

struct EnemyBullet {
    int x;
    int yQ;
    int dyQ;
    int speedX;
    bool active;
};

struct HomingMissile {
    int x;
    int yQ;
    int dyQ;
    unsigned long bornAt;
    unsigned long lastSteerAt;
    bool active;
    int speedRemainder;
};

enum BossAttackKind {
    BOSS_ATTACK_FAN,
    BOSS_ATTACK_MISSILES,
    BOSS_ATTACK_LASER
};

struct Explosion {
    int x;
    int y;
    uint8_t scale;
    unsigned long startedAt;
    bool active;
};

enum EnemyType : uint8_t { ENEMY_STANDARD, ENEMY_SCOUT, ENEMY_GUNNER };

const int STANDARD_ART_W = 11;
const uint16_t STANDARD_SPRITE[ENEMY_H] = {
    0b00100000100, 0b00010001000, 0b00111111100, 0b01101110110,
    0b11111111111, 0b10111111101, 0b10100000101, 0b00011011000
};
// Hundredths of a pixel per movement update: a gentle increase every level.
const int STANDARD_SPEED_Q_BY_LEVEL[MAX_LEVELS] = {100, 125, 150, 175, 200};

const uint16_t SCOUT_SPRITE[ENEMY_H] = {
    // Reference-inspired alien rotated 90 degrees counterclockwise: nose left.
    0b0000110010, 0b0001110100, 0b0011011000, 0b0111110110, 0b0111110110, 0b0011011000, 0b0001110100, 0b0000110010
};
const int GUNNER_ART_W = 14;
const int GUNNER_ART_H = 10;
const uint16_t GUNNER_SPRITE[GUNNER_ART_H] = {
    0b00001111110000,
    0b00010000001000,
    0b00010100101000,
    0b00010000001000,
    0b00111111111100,
    0b01111111111110,
    0b11111111111111,
    0b10100100100101,
    0b01111111111110,
    0b00011111111000
};

struct Enemy {
    int x;
    int y;
    int health;
    int maxHealth;
    int direction;
    unsigned long lastShot;
    bool active;
    EnemyType type;
    int moveRemainder;
    unsigned long hitFlashUntil;
};

enum PowerUpType : uint8_t {
    POWERUP_NONE,
    POWERUP_EXTRA_LIFE,
    POWERUP_SHIELD,
    POWERUP_SHOTGUN,
    POWERUP_LASER
};

struct PowerUp {
    int x;
    int y;
    PowerUpType type;
    bool active;
};

Bullet bullets[MAX_BULLETS];
EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];
HomingMissile homingMissiles[MAX_HOMING_MISSILES];
Enemy enemies[MAX_SIMULTANEOUS_ENEMIES];
Explosion explosions[MAX_EXPLOSIONS];
PowerUp powerUp;
int starsX[MAX_STARS];
int starsY[MAX_STARS];

int playerX;
int playerY;
int playerMoveRemainder = 0;
int score;
int level;
bool endlessMode = false;
int endlessWave = 0;
bool victoryEndlessSelected = true;
int comboKills = 0;
int lives;
int highScore = 0;
int enemyShotPattern = 0;
int regularEnemiesRemaining = 0;
int regularEnemiesSpawned = 0;
unsigned long nextEnemySpawnAt = 0;
bool enemyIsBoss = false;

Preferences highScoreStore;

bool gameStarted = false;
bool gameOver = false;
bool levelVictory = false;
bool victoryNeedsRelease = false;
unsigned long lastMove = 0;
unsigned long lastStarMove = 0;
unsigned long lastEnemyMove = 0;
unsigned long lastPlayerMove = 0;
unsigned long lastShot = 0;
unsigned long playerExplosionUntil = 0;
unsigned long victoryExplosionUntil = 0;
unsigned long playerExplosionStartedAt = 0;
int playerExplosionX = 0;
int playerExplosionY = 0;
unsigned long invulnerableUntil = 0;
unsigned long shieldUntil = 0;
unsigned long shotgunUntil = 0;
unsigned long laserUntil = 0;
unsigned long lastLaserDamage = 0;
unsigned long bossIntroUntil = 0;
unsigned long bossFightStartedAt = 0;
unsigned long bossNextAttackAt = 0;
unsigned long bossWindupUntil = 0;
unsigned long bossLaserUntil = 0;
unsigned long bossShieldFlashUntil = 0;
unsigned long bossPhaseAnnouncementUntil = 0;
int bossAttackKind = BOSS_ATTACK_FAN;
int bossAttackAimY = 0;
int bossLaserY = 0;
int bossLaserSourceY = 0;
int bossLaserTargetX = 0;
int bossPatternIndex = 0;
bool bossCharging = false;
bool bossLaserFiring = false;
bool bossEnraged = false;
PowerUpType powerUpPopupType = POWERUP_NONE;
unsigned long powerUpPopupUntil = 0;
int powerUpPopupAnchorX = 0;
int powerUpPopupAnchorY = 0;

const unsigned long MOVE_DELAY = 35;
const unsigned long ENEMY_MOVE_DELAY = MOVE_DELAY * 2;
const unsigned long PLAYER_MOVE_DELAY = 25;
const unsigned long EXPLOSION_TIMELINE_MS = 480;
const unsigned long EXPLOSION_DURATION_MS = EXPLOSION_TIMELINE_MS * 2;
const unsigned long SHOT_DELAY = 560;
const unsigned long INVULNERABLE_DURATION = 3000;
const unsigned long SHIELD_DURATION = 6000;
const unsigned long SHOTGUN_DURATION = 8000;
const unsigned long LASER_DURATION = 4000;
const unsigned long LASER_DAMAGE_INTERVAL_MS = 180;
const unsigned long BOSS_INTRO_DURATION_MS = 3000;
const unsigned long BOSS_SHIELD_CLOSED_MS = 3000;
const unsigned long BOSS_SHIELD_OPEN_MS = 4400;
const unsigned long BOSS_SHIELD_CYCLE_MS = BOSS_SHIELD_CLOSED_MS + BOSS_SHIELD_OPEN_MS;
const unsigned long BOSS_SHIELD_WARNING_MS = 700;
const unsigned long BOSS_LASER_DURATION_MS = 500;
const unsigned long MISSILE_STEER_INTERVAL_MS = 140;
const unsigned long MISSILE_LIFETIME_MS = 6500;
const unsigned long POWERUP_POPUP_DURATION_MS = 950;
const unsigned long POWERUP_POPUP_WIPE_START_MS = 500;
const int ENEMY_ENTRY_MIN_MS = 1000;
const int ENEMY_ENTRY_MAX_MS = 3000;
