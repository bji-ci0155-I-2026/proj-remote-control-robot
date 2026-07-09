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
// Motor 1: IO12 e IO14
// Motor 2: IO13 e IO15
// (IO10 evitado: reservado para la flash SPI en el ESP32 original)
// =======================
const int M1_A = 12;
const int M1_B = 14;
const int M2_A = 13;
const int M2_B = 15;

const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8;

// =======================
// Sensor ultrasonico HC-SR04 (al frente del carrito)
// OJO: el pin ECHO del HC-SR04 saca 5V. El ESP32 es de 3.3V.
//      Usar un divisor de voltaje (ej. 1kO + 2kO) o level shifter en ECHO.
// Se evitan los pines 4, 25, 26, 27 (ADC2, fallan como analogicos con WiFi activo).
// =======================
const int TRIG_PIN = 18;   // salida
const int ECHO_PIN = 34;   // entrada (34 es solo-entrada, ideal para un sensor)

// =======================
// LED indicador (reemplaza al buzzer que no tenemos)
// Se enciende cuando el sensor detecta un obstaculo cercano.
// Cambiar por el pin del LED integrado de tu placa si aplica.
// =======================
const int LED_PIN = 2;

// =======================
// Velocidades
// =======================
int speedForward = 180;
int speedTurn = 160;

// Cambia a true si algun motor gira al reves
bool invertMotor1 = false;
bool invertMotor2 = false;

// =======================
// Parametros de deteccion de obstaculos
// =======================
const int OBSTACLE_DISTANCE_CM = 15;   // umbral para frenar al avanzar
const unsigned long MEASURE_INTERVAL_MS = 60;  // cada cuanto medir
const unsigned long ECHO_TIMEOUT_US = 8000;    // ~1.3 m; mas alla se considera "libre"

// =======================
// Watchdog de conexion
// La pagina reenvia el comando activo cada 200ms (heartbeat). Si el ESP32
// deja de recibir comandos (WiFi caido, pestana cerrada, telefono bloqueado),
// se detiene solo tras COMMAND_TIMEOUT_MS en vez de seguir corriendo.
// =======================
const unsigned long COMMAND_TIMEOUT_MS = 600;  // > intervalo del heartbeat (200ms)

// =======================
// Estado del carrito
// Los handlers HTTP solo fijan el comando deseado; loop() decide que hacen
// realmente los motores (asi el freno de emergencia es centralizado).
// =======================
enum Command { CMD_STOP, CMD_FORWARD, CMD_BACKWARD, CMD_LEFT, CMD_RIGHT };
Command currentCommand = CMD_STOP;

long lastDistanceCm = 999;          // ultima medicion (999 = libre)
unsigned long lastMeasureTime = 0;  // control de cadencia de medicion
unsigned long lastCommandTime = 0;  // ultimo comando recibido (para el watchdog)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[DEBUG] setup() inicio");

  // Motores
  setupMotorPin(M1_A);
  setupMotorPin(M1_B);
  setupMotorPin(M2_A);
  setupMotorPin(M2_B);
  stopMotors();
  Serial.println("[DEBUG] motores OK");

  // Sensor ultrasonico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println("[DEBUG] HC-SR04 OK");

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Crear red WiFi propia
  bool apOk = WiFi.softAP(ssid, password);
  Serial.print("[DEBUG] WiFi.softAP() devolvio: ");
  Serial.println(apOk ? "true" : "false");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Rutas del servidor
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);   // devuelve la distancia para la UI

  server.begin();
  Serial.println("[DEBUG] setup() fin");
}

void loop() {
  server.handleClient();
  checkWatchdog();      // detiene el carrito si se perdio la conexion
  updateDistance();     // mide cada MEASURE_INTERVAL_MS
  applyMovement();      // aplica el comando respetando el freno de emergencia
  updateLed();          // enciende el LED si hay obstaculo cercano
}

// =======================
// Watchdog: si no llegan comandos, detener
// =======================
void checkWatchdog() {
  if (currentCommand == CMD_STOP) return;  // ya detenido, nada que vigilar
  if (millis() - lastCommandTime > COMMAND_TIMEOUT_MS) {
    currentCommand = CMD_STOP;
    Serial.println("[WATCHDOG] sin comandos, deteniendo");
  }
}

// =======================
// Medicion de distancia (no bloqueante por intervalo)
// =======================
void updateDistance() {
  unsigned long now = millis();
  if (now - lastMeasureTime < MEASURE_INTERVAL_MS) return;
  lastMeasureTime = now;

  // Pulso de disparo de 10us
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    // Sin eco dentro del timeout => nada cerca => libre
    lastDistanceCm = 999;
  } else {
    // distancia = (tiempo * velocidad del sonido) / 2
    lastDistanceCm = (long)(duration * 0.0343 / 2.0);
  }
}

bool obstacleAhead() {
  return lastDistanceCm <= OBSTACLE_DISTANCE_CM;
}

// =======================
// Aplicacion del movimiento + freno de emergencia
// Solo se bloquea AVANZAR. Retroceder y girar siguen permitidos
// para poder escapar del obstaculo.
// =======================
void applyMovement() {
  switch (currentCommand) {
    case CMD_FORWARD:
      if (obstacleAhead()) {
        stopMotors();               // freno autonomo
      } else {
        moveForward();
      }
      break;

    case CMD_BACKWARD:
      moveBackward();
      break;

    case CMD_LEFT:
      turnLeft();
      break;

    case CMD_RIGHT:
      turnRight();
      break;

    case CMD_STOP:
    default:
      stopMotors();
      break;
  }
}

void updateLed() {
  digitalWrite(LED_PIN, obstacleAhead() ? HIGH : LOW);
}

// =======================
// Pagina web
// =======================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Control Carrito ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f2f2f2;
      margin-top: 40px;
    }
    h1 { font-size: 28px; }
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
    .stop { background: #b00020; width: 160px; }
    .row { margin: 10px; }
    #dist { font-size: 20px; margin-top: 20px; }
    .alert { color: #b00020; font-weight: bold; }
  </style>
</head>
<body>
  <h1>Carrito ESP32</h1>

  <div class="row">
    <button class="button"
      ontouchstart="hold(event,'/forward')" ontouchend="release(event)"
      onmousedown="hold(event,'/forward')" onmouseup="release(event)"
      onmouseleave="release(event)">
      &uarr;
    </button>
  </div>

  <div class="row">
    <button class="button"
      ontouchstart="hold(event,'/left')" ontouchend="release(event)"
      onmousedown="hold(event,'/left')" onmouseup="release(event)"
      onmouseleave="release(event)">
      &larr;
    </button>

    <button class="button stop" onclick="release(event)">STOP</button>

    <button class="button"
      ontouchstart="hold(event,'/right')" ontouchend="release(event)"
      onmousedown="hold(event,'/right')" onmouseup="release(event)"
      onmouseleave="release(event)">
      &rarr;
    </button>
  </div>

  <div class="row">
    <button class="button"
      ontouchstart="hold(event,'/backward')" ontouchend="release(event)"
      onmousedown="hold(event,'/backward')" onmouseup="release(event)"
      onmouseleave="release(event)">
      &darr;
    </button>
  </div>

  <div id="dist">Distancia: -- cm</div>

  <script>
    var heldCommand = null;

    function sendCommand(command) {
      fetch(command).catch(error => console.log(error));
    }

    // Presionar: fija el comando y arranca el heartbeat
    function hold(e, command) {
      e.preventDefault();          // evita disparar mouse+touch a la vez
      heldCommand = command;
      sendCommand(command);
    }

    // Soltar: detiene y cancela el heartbeat
    function release(e) {
      if (e) e.preventDefault();
      heldCommand = null;
      sendCommand('/stop');
    }

    // Heartbeat: reenvia el comando activo para alimentar el watchdog del ESP32
    setInterval(function () {
      if (heldCommand) sendCommand(heldCommand);
    }, 200);

    // Sondeo de la distancia para monitoreo
    setInterval(function () {
      fetch('/status')
        .then(r => r.text())
        .then(function (cm) {
          var d = document.getElementById('dist');
          var n = parseInt(cm, 10);
          d.textContent = 'Distancia: ' + (n >= 999 ? 'libre' : (n + ' cm'));
          d.className = (n <= 15) ? 'alert' : '';
        })
        .catch(function () {});
    }, 300);
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// =======================
// Rutas de movimiento (solo fijan el estado deseado)
// =======================
// Cada comando de movimiento refresca el watchdog
void setCommand(Command cmd) {
  currentCommand = cmd;
  lastCommandTime = millis();
}

void handleForward() {
  setCommand(CMD_FORWARD);
  server.send(200, "text/plain", "Forward");
}

void handleBackward() {
  setCommand(CMD_BACKWARD);
  server.send(200, "text/plain", "Backward");
}

void handleLeft() {
  setCommand(CMD_LEFT);
  server.send(200, "text/plain", "Left");
}

void handleRight() {
  setCommand(CMD_RIGHT);
  server.send(200, "text/plain", "Right");
}

void handleStop() {
  setCommand(CMD_STOP);
  server.send(200, "text/plain", "Stop");
}

void handleStatus() {
  server.send(200, "text/plain", String(lastDistanceCm));
}

// =======================
// Configuracion de motores
// =======================
void setupMotorPin(int pin) {
  ledcAttach(pin, PWM_FREQ, PWM_RESOLUTION);
}

// speed positivo = adelante, negativo = atras, 0 = detener
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
