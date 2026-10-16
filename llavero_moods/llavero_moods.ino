#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
#define MPU_ADDR      0x68
#define PIN_SDA       8
#define PIN_SCL       9
#define PIN_BUTTON    4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ---------------------------------------------------------------
// Moods
// ---------------------------------------------------------------
enum Mood { M_NORMAL, M_HAPPY, M_BORED, M_SLEEPY, M_SURPRISED, M_ANGRY, M_DIZZY };

Mood mood = M_NORMAL;
Mood moodAfterSurprise = M_NORMAL;
unsigned long moodStart = 0;
unsigned long lastInteraction = 0;
unsigned long lastConfused = 0;

// Tiempos (ms) - ajustalos a tu gusto
const unsigned long T_BORED      = 20000;  // sin tocarlo -> se aburre
const unsigned long T_SLEEP      = 45000;  // sigue ignorado -> se duerme
const unsigned long T_HAPPY      = 4000;
const unsigned long T_SURPRISED  = 1500;
const unsigned long T_ANGRY      = 4000;
const unsigned long T_DIZZY      = 3000;
const unsigned long LONG_PRESS   = 800;

void resetEyes() {
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);
  roboEyes.setSweat(OFF);
  roboEyes.setCuriosity(OFF);
  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  roboEyes.setBorderradius(8, 8);
  roboEyes.setSpacebetween(10);
  roboEyes.open();
}

void setMood(Mood m) {
  mood = m;
  moodStart = millis();
  resetEyes();

  switch (m) {
    case M_NORMAL:
      roboEyes.setCuriosity(ON);
      break;

    case M_HAPPY:
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      break;

    case M_BORED:
      
      roboEyes.setHeight(18, 18);
      roboEyes.setBorderradius(4, 4);
      roboEyes.setIdleMode(ON, 4, 3);
      roboEyes.setAutoblinker(ON, 6, 3);
      break;

    case M_SLEEPY:
      // Se dibuja aparte en drawSleep()
      break;

    case M_SURPRISED:
      roboEyes.setWidth(44, 44);
      roboEyes.setHeight(50, 50);
      roboEyes.setBorderradius(14, 14);
      roboEyes.setAutoblinker(OFF, 0, 0);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setSweat(ON);
      break;

    case M_ANGRY:
      roboEyes.setMood(ANGRY);
      roboEyes.setHFlicker(ON, 2);
      roboEyes.setIdleMode(OFF, 0, 0);
      break;

    case M_DIZZY:
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setAutoblinker(OFF, 0, 0);
      roboEyes.setSweat(ON);
      roboEyes.anim_confused();
      lastConfused = millis();
      break;
  }
}

void interact() {
  lastInteraction = millis();
}

void drawSleep() {
  static unsigned long lastFrame = 0;
  unsigned long now = millis();
  if (now - lastFrame < 100) return;   
  lastFrame = now;

  display.clearDisplay();

  int breath = ((now / 900) % 2);       // 0 o 1: cambia el grosor
  int h = 4 + breath;
  display.fillRoundRect(6,  40, 36, h, 2, SSD1306_WHITE);
  display.fillRoundRect(48, 40, 36, h, 2, SSD1306_WHITE);

  // zzz
  int count = (now / 600) % 4;          
  display.setTextColor(SSD1306_WHITE);
  if (count >= 1) { display.setTextSize(1); display.setCursor(92, 34); display.print("z"); }
  if (count >= 2) { display.setTextSize(2); display.setCursor(101, 18); display.print("z"); }
  if (count >= 3) { display.setTextSize(3); display.setCursor(108, 0);  display.print("Z"); }

  display.display();
}

// ---------------------------------------------------------------
// Acelerometro
// ---------------------------------------------------------------
bool hasAccel = false;
int16_t lastAx = 0, lastAy = 0, lastAz = 0;
unsigned long lastAccelRead = 0;
unsigned long lastShakeHit = 0;
unsigned long lastShakeEvent = 0;
int shakeHits = 0;

const int SHAKE_THRESHOLD = 12000;      // sensibilidad: bajalo si cuesta activarlo
const int SHAKE_HITS_NEEDED = 4;        // sacudidas seguidas para contar como "shake"
const unsigned long SHAKE_WINDOW = 600; // ms maximos entre sacudidas
const unsigned long SHAKE_COOLDOWN = 1500;

void initAccel() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);   // registro PWR_MGMT_1
  Wire.write(0x00);   // despertar el sensor
  hasAccel = (Wire.endTransmission() == 0);
  Serial.println(hasAccel ? "Acelerometro detectado" : "Sin acelerometro (modo sin sacudidas)");
}

bool readAccel(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);   // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) < 6) return false;
  x = (Wire.read() << 8) | Wire.read();
  y = (Wire.read() << 8) | Wire.read();
  z = (Wire.read() << 8) | Wire.read();
  return true;
}

void onShake() {
  interact();
  if (mood == M_SLEEPY) {
    moodAfterSurprise = M_DIZZY;   // se despierta de golpe y queda mareado
    setMood(M_SURPRISED);
  } else if (mood != M_DIZZY && mood != M_SURPRISED) {
    setMood(M_DIZZY);
  }
}

void handleAccel() {
  if (!hasAccel) return;
  unsigned long now = millis();
  if (now - lastAccelRead < 30) return;
  lastAccelRead = now;

  int16_t ax, ay, az;
  if (!readAccel(ax, ay, az)) return;

  long delta = abs(ax - lastAx) + abs(ay - lastAy) + abs(az - lastAz);
  lastAx = ax; lastAy = ay; lastAz = az;

  if (delta > SHAKE_THRESHOLD) {
    if (now - lastShakeHit > SHAKE_WINDOW) shakeHits = 0;
    shakeHits++;
    lastShakeHit = now;
    if (shakeHits >= SHAKE_HITS_NEEDED && now - lastShakeEvent > SHAKE_COOLDOWN) {
      lastShakeEvent = now;
      shakeHits = 0;
      onShake();
    }
  }
}

// ---------------------------------------------------------------
// Interacciones con el boton
// ---------------------------------------------------------------
void onPet() {
  Mood prev = mood;
  interact();
  if (prev == M_SLEEPY) {
    moodAfterSurprise = M_HAPPY;   // se despierta de golpe y luego se alegra
    setMood(M_SURPRISED);
  } else {
    setMood(M_HAPPY);
  }
}

void onPoke() {
  interact();
  setMood(M_ANGRY);
}

bool btnDown = false;
bool longFired = false;
unsigned long btnChange = 0;
unsigned long btnPressedAt = 0;

void handleButton() {
  bool pressed = (digitalRead(PIN_BUTTON) == LOW);
  unsigned long now = millis();

  if (pressed != btnDown && now - btnChange > 30) {
    btnChange = now;
    btnDown = pressed;
    if (pressed) {
      btnPressedAt = now;
      longFired = false;
    } else if (!longFired) {
      onPet();
    }
  }

  if (btnDown && !longFired && now - btnPressedAt > LONG_PRESS) {
    longFired = true;
    onPoke();
  }
}

// ---------------------------------------------------------------
// Comandos por serie (para probar moods sin boton)
// ---------------------------------------------------------------
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c < '1' || c > '7') continue;   // ignora saltos de linea, etc.
    interact();
    switch (c) {
      case '1': setMood(M_NORMAL); break;
      case '2': setMood(M_HAPPY); break;
      case '3': setMood(M_BORED); break;
      case '4': setMood(M_SLEEPY); break;
      case '5': moodAfterSurprise = M_NORMAL; setMood(M_SURPRISED); break;
      case '6': setMood(M_ANGRY); break;
      case '7': setMood(M_DIZZY); break;
    }
  }
}

// ---------------------------------------------------------------
// Transiciones automaticas
// ---------------------------------------------------------------
void updateMoods() {
  unsigned long now = millis();
  unsigned long inMood = now - moodStart;
  unsigned long idle = now - lastInteraction;

  switch (mood) {
    case M_NORMAL:
      if (idle > T_BORED) setMood(M_BORED);
      break;
    case M_HAPPY:
      if (inMood > T_HAPPY) setMood(M_NORMAL);
      break;
    case M_BORED:
      if (idle > T_SLEEP) setMood(M_SLEEPY);
      break;
    case M_SLEEPY:
      break;  // duerme hasta que lo toques o lo muevas
    case M_SURPRISED:
      if (inMood > T_SURPRISED) setMood(moodAfterSurprise);
      break;
    case M_ANGRY:
      if (inMood > T_ANGRY) setMood(M_NORMAL);
      break;
    case M_DIZZY:
      if (now - lastConfused > 700) {   // repite el temblor mientras esta mareado
        roboEyes.anim_confused();
        lastConfused = now;
      }
      if (inMood > T_DIZZY) setMood(M_NORMAL);
      break;
  }
}

// ---------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("No encuentro la OLED");
    for (;;);
  }

  initAccel();

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  lastInteraction = millis();
  setMood(M_NORMAL);

  Serial.println("Moods por serie: 1 normal, 2 feliz, 3 aburrido, 4 dormido, 5 sorprendido, 6 enfadado, 7 mareado");
}

void loop() {
  if (mood == M_SLEEPY) drawSleep();
  else roboEyes.update();

  handleButton();
  handleAccel();
  handleSerial();
  updateMoods();
}
