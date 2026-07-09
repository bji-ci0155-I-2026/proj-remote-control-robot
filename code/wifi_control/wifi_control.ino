#include <WiFi.h>
#include <WebServer.h>

// =======================
// Red WiFi creada por el ESP32
// =======================
const char* ssid = "ESP32_Carrito";
const char* password = "12345678";

WebServer server(80);

// =======================
// Pines de motores
// Motor 1: IO10 e IO15
// Motor 2: IO12 e IO14
// =======================
const int M1_A = 12;
const int M1_B = 14;
const int M2_A = 13;
const int M2_B = 15;

const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8;

// Velocidades
int speedForward = 180;
int speedTurn = 160;

// Cambia a true si algún motor gira al revés
bool invertMotor1 = false;
bool invertMotor2 = false;

void setup() {
  Serial.begin(115200);
  delay(1000); // da tiempo a abrir el Monitor Serie antes de imprimir
  Serial.println("[DEBUG] setup() inicio");

  Serial.println("[DEBUG] configurando pin M1_A...");
  setupMotorPin(M1_A);
  Serial.println("[DEBUG] configurando pin M1_B...");
  setupMotorPin(M1_B);
  Serial.println("[DEBUG] configurando pin M2_A...");
  setupMotorPin(M2_A);
  Serial.println("[DEBUG] configurando pin M2_B...");
  setupMotorPin(M2_B);
  Serial.println("[DEBUG] pines de motor configurados OK");

  stopMotors();
  Serial.println("[DEBUG] motores detenidos OK");

  // Crear red WiFi propia
  Serial.println("[DEBUG] llamando WiFi.softAP()...");
  bool apOk = WiFi.softAP(ssid, password);
  Serial.print("[DEBUG] WiFi.softAP() devolvio: ");
  Serial.println(apOk ? "true" : "false");

  Serial.println("Red WiFi creada");
  Serial.print("Nombre: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Rutas del servidor
  Serial.println("[DEBUG] registrando rutas del servidor...");
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);

  server.begin();
  Serial.println("Servidor web iniciado");
  Serial.println("[DEBUG] setup() fin");
}

void loop() {
  server.handleClient();
}

// =======================
// Página web
// =======================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Control Carrito ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f2f2f2;
      margin-top: 40px;
    }

    h1 {
      font-size: 28px;
    }

    .button {
      width: 130px;
      height: 70px;
      font-size: 22px;
      margin: 8px;
      border: none;
      border-radius: 12px;
      background: #333;
      color: white;
    }

    .stop {
      background: #b00020;
      width: 160px;
    }

    .row {
      margin: 10px;
    }
  </style>
</head>
<body>
  <h1>Carrito ESP32</h1>

  <div class="row">
    <button class="button" 
      ontouchstart="sendCommand('/forward')" 
      ontouchend="sendCommand('/stop')"
      onmousedown="sendCommand('/forward')" 
      onmouseup="sendCommand('/stop')">
      ↑
    </button>
  </div>

  <div class="row">
    <button class="button" 
      ontouchstart="sendCommand('/left')" 
      ontouchend="sendCommand('/stop')"
      onmousedown="sendCommand('/left')" 
      onmouseup="sendCommand('/stop')">
      ←
    </button>

    <button class="button stop" 
      onclick="sendCommand('/stop')">
      STOP
    </button>

    <button class="button" 
      ontouchstart="sendCommand('/right')" 
      ontouchend="sendCommand('/stop')"
      onmousedown="sendCommand('/right')" 
      onmouseup="sendCommand('/stop')">
      →
    </button>
  </div>

  <div class="row">
    <button class="button" 
      ontouchstart="sendCommand('/backward')" 
      ontouchend="sendCommand('/stop')"
      onmousedown="sendCommand('/backward')" 
      onmouseup="sendCommand('/stop')">
      ↓
    </button>
  </div>

  <script>
    function sendCommand(command) {
      fetch(command).catch(error => console.log(error));
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// =======================
// Rutas de movimiento
// =======================

void handleForward() {
  Serial.println("Avanzar");
  moveForward();
  server.send(200, "text/plain", "Forward");
}

void handleBackward() {
  Serial.println("Retroceder");
  moveBackward();
  server.send(200, "text/plain", "Backward");
}

void handleLeft() {
  Serial.println("Izquierda");
  turnLeft();
  server.send(200, "text/plain", "Left");
}

void handleRight() {
  Serial.println("Derecha");
  turnRight();
  server.send(200, "text/plain", "Right");
}

void handleStop() {
  Serial.println("Stop");
  stopMotors();
  server.send(200, "text/plain", "Stop");
}

// =======================
// Configuración de motores
// =======================

void setupMotorPin(int pin) {
  ledcAttach(pin, PWM_FREQ, PWM_RESOLUTION);
}

// speed positivo = adelante
// speed negativo = atrás
// speed 0 = detener
void setMotor(int pinA, int pinB, int speed, bool inverted) {
  if (inverted) {
    speed = -speed;
  }

  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    ledcWrite(pinA, speed);
    ledcWrite(pinB, 0);
  }
  else if (speed < 0) {
    ledcWrite(pinA, 0);
    ledcWrite(pinB, -speed);
  }
  else {
    ledcWrite(pinA, 0);
    ledcWrite(pinB, 0);
  }
}

void motor1(int speed) {
  setMotor(M1_A, M1_B, speed, invertMotor1);
}

void motor2(int speed) {
  setMotor(M2_A, M2_B, speed, invertMotor2);
}

// =======================
// Movimientos
// =======================

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