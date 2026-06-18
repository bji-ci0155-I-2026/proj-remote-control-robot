#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <DabbleESP32.h>

// Motor 1: asumimos que es el izquierdo
const int M1_A = 10;
const int M1_B = 15;

// Motor 2: asumimos que es el derecho
const int M2_A = 12;
const int M2_B = 14;

// Canales PWM
const int CH_M1_A = 0;
const int CH_M1_B = 1;
const int CH_M2_A = 2;
const int CH_M2_B = 3;

const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8; // 0 a 255

int speedForward = 180;
int speedTurn = 160;

// Cambia estos a true si algÃºn motor gira al revÃ©s
bool invertMotor1 = false;
bool invertMotor2 = false;

void setup() {
  Serial.begin(115200);

  setupMotorPin(M1_A, CH_M1_A);
  setupMotorPin(M1_B, CH_M1_B);
  setupMotorPin(M2_A, CH_M2_A);
  setupMotorPin(M2_B, CH_M2_B);

  Dabble.begin("ESP32_Robot");

  stopMotors();

  Serial.println("Listo. Conecta Dabble y usa el Gamepad.");
}

void loop() {
  Dabble.processInput();

  if (GamePad.isUpPressed()) {
    Serial.println("Avanzar");
    moveForward();
  }
  else if (GamePad.isDownPressed()) {
    Serial.println("Retroceder");
    moveBackward();
  }
  else if (GamePad.isLeftPressed()) {
    Serial.println("Izquierda");
    turnLeft();
  }
  else if (GamePad.isRightPressed()) {
    Serial.println("Derecha");
    turnRight();
  }
  else {
    stopMotors();
  }
}

void setupMotorPin(int pin, int channel) {
  ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(pin, channel);
}

// speed positivo: adelante
// speed negativo: atrÃ¡s
// speed 0: detenido
void setMotor(int channelA, int channelB, int speed, bool inverted) {
  if (inverted) {
    speed = -speed;
  }

  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    ledcWrite(channelA, speed);
    ledcWrite(channelB, 0);
  }
  else if (speed < 0) {
    ledcWrite(channelA, 0);
    ledcWrite(channelB, -speed);
  }
  else {
    ledcWrite(channelA, 0);
    ledcWrite(channelB, 0);
  }
}

void motor1(int speed) {
  setMotor(CH_M1_A, CH_M1_B, speed, invertMotor1);
}

void motor2(int speed) {
  setMotor(CH_M2_A, CH_M2_B, speed, invertMotor2);
}

void moveForward() {
  motor1(speedForward);
  motor2(speedForward);
}

void moveBackward() {
  motor1(-speedForward);
  motor2(-speedForward);
}

void turnLeft() {
  motor1(-speedTurn);
  motor2(speedTurn);
}

void turnRight() {
  motor1(speedTurn);
  motor2(-speedTurn);
}

void stopMotors() {
  motor1(0);
  motor2(0);
}