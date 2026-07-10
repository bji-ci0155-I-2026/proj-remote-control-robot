#include <WiFi.h>
#include <WebServer.h>

// =========================================================
// Modo estacion (station): el carrito se UNE a una red 2.4 GHz
// existente (router) como un cliente mas, junto con la laptop
// (System 2) y el telefono-camara. Ya no crea su propia softAP.
// Toma una IP fija para que la laptop siempre sepa a donde apuntar.
//
// System 1 (motores + freno ultrasonico + watchdog) queda intacto:
// el MVP funciona aunque la laptop este apagada. Los endpoints HTTP
// (/forward, /left, ...) reciben un comando a la vez, siempre sujeto
// al freno local.
// =========================================================

// Red a la que se une el ESP32. >>> EDITAR con tu router 2.4 GHz <<<
const char* ssid = "Lab-3-5";
const char* password = "Cata2960!";

// IP fija del carrito. Debe estar en la subred REAL del host y fuera
// del rango DHCP. Para descubrirla: pon USE_STATIC_IP=false, flashea,
// y lee la IP/gateway del serial; luego copia esa subred aqui.
const bool USE_STATIC_IP = true;

IPAddress local_IP(192, 168, 0, 169);  // IP fija del carrito
IPAddress gateway(192, 168, 0, 1);     // IP del router
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 0, 1);

const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;  // reintenta hasta 20 s

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
// Buzzer pasivo (bocina / HONK)
// Modulo pasivo de 3 pines: S -> IO19, "-" -> GND, pin central sin conexion.
// Al ser pasivo se genera el tono por PWM (ledcWriteTone).
// =======================
const int BUZZER_PIN = 19;
const int HONK_FREQ_HZ = 440;          // tono tipo bocina
const unsigned long HONK_DURATION_MS = 400;  // duracion de cada bocinazo

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
// La laptop (o la pagina) reenvia el comando activo periodicamente
// (heartbeat). Si el ESP32 deja de recibir comandos (WiFi caido,
// laptop apagada, pestana cerrada), se detiene solo tras
// COMMAND_TIMEOUT_MS en vez de seguir corriendo.
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

bool honkActive = false;            // hay un bocinazo sonando
unsigned long honkUntil = 0;        // millis() en que se apaga el buzzer

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

  // Buzzer pasivo (bocina). Se usa PWM como con los motores.
  ledcAttach(BUZZER_PIN, HONK_FREQ_HZ, 8);
  ledcWriteTone(BUZZER_PIN, 0);  // silencio

  // Unirse a la red del host (modo estacion) con IP fija
  connectWiFi();

  // Rutas del servidor
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/honk", handleHonk);       // suena la bocina
  server.on("/status", handleStatus);   // devuelve la distancia para la UI/laptop

  server.begin();
  Serial.println("[DEBUG] setup() fin");
}

void loop() {
  server.handleClient();
  checkWatchdog();      // detiene el carrito si se perdio la conexion
  updateDistance();     // mide cada MEASURE_INTERVAL_MS
  applyMovement();      // aplica el comando respetando el freno de emergencia
  updateLed();          // enciende el LED si hay obstaculo cercano
  updateHonk();         // apaga la bocina cuando termina el bocinazo
}

// =======================
// Bocina no bloqueante: handleHonk() la enciende, esto la apaga sola
// al pasar HONK_DURATION_MS, sin frenar el manejo de comandos.
// =======================
void updateHonk() {
  if (honkActive && millis() >= honkUntil) {
    ledcWriteTone(BUZZER_PIN, 0);
    honkActive = false;
  }
}

// =======================
// Conexion WiFi en modo estacion (station)
// =======================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);   // evita micro-cortes de energia que tumban la asociacion

  // IP fija solo si USE_STATIC_IP; si no, DHCP para DESCUBRIR la subred real.
  if (USE_STATIC_IP) {
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
      Serial.println("[WIFI] WiFi.config() fallo, se usara DHCP");
    }
  } else {
    Serial.println("[WIFI] USE_STATIC_IP=false -> DHCP (modo descubrimiento).");
    Serial.println("[WIFI] Anota la IP y el gateway de abajo para pinearlos luego.");
  }

  Serial.print("[WIFI] Conectando a la red \"");
  Serial.print(ssid);
  Serial.println("\" ...");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Conectado.");
    Serial.print("[WIFI] IP del carrito: ");
    Serial.println(WiFi.localIP());     // <-- la laptop apunta AQUI
    Serial.print("[WIFI] Gateway (host): ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[WIFI] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    // No abortamos: el System 1 (motores + freno) igual funciona sin red.
    // Para Rung 0 basta avisar; para reintentar, resetear la placa.
    Serial.println("[WIFI] NO se pudo conectar. Revisa SSID/clave/subred.");
    Serial.println("[WIFI] El carrito funciona local, pero sin comandos remotos.");
  }
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
// Sirve para probar el carrito desde cualquier dispositivo de la red
// (incluida la laptop) apuntando a la IP fija del carrito.
// =======================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
  <title>Control Carrito ESP32</title>
  <style>
    :root {
      --bg: #10131a;
      --panel: #1b2030;
      --btn: #2a3350;
      --btn-active: #3a4a7a;
      --stop: #b00020;
      --stop-active: #d81b3f;
      --honk: #e8a400;
      --honk-active: #ffbf1f;
      --text: #f2f4f8;
      --muted: #9aa4bd;
      --ok: #35c46b;
      --alert: #ff3b4e;
    }
    * { box-sizing: border-box; }
    html, body { height: 100%; margin: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-between;
      min-height: 100%;
      padding: calc(env(safe-area-inset-top) + 12px) 12px calc(env(safe-area-inset-bottom) + 16px);
      -webkit-user-select: none;
      user-select: none;
      -webkit-touch-callout: none;
      -webkit-tap-highlight-color: transparent;
      touch-action: manipulation;
      overflow: hidden;
    }
    header { text-align: center; padding-top: 4px; }
    h1 { font-size: 20px; font-weight: 600; margin: 0; letter-spacing: .3px; }
    .sub { font-size: 12px; color: var(--muted); margin-top: 2px; }

    .status {
      width: 100%;
      max-width: 420px;
      background: var(--panel);
      border-radius: 14px;
      padding: 14px 16px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      font-size: 18px;
      border: 2px solid transparent;
      transition: border-color .15s, background .15s, color .15s;
    }
    .status .label { color: var(--muted); font-size: 14px; }
    .status .value { font-weight: 700; }
    .status.free .value { color: var(--ok); }
    .status.alert {
      border-color: var(--alert);
      background: #2a1620;
      color: var(--alert);
      animation: pulse .6s ease-in-out infinite alternate;
    }
    .status.alert .value, .status.alert .label { color: var(--alert); }
    @keyframes pulse {
      from { box-shadow: 0 0 0 0 rgba(255,59,78,0); }
      to   { box-shadow: 0 0 0 6px rgba(255,59,78,.25); }
    }

    .pad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      grid-template-rows: repeat(3, 1fr);
      gap: 12px;
      width: 100%;
      max-width: 420px;
      aspect-ratio: 1 / 1;
      margin: 8px 0;
    }
    .button {
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 34px;
      border: none;
      border-radius: 18px;
      background: var(--btn);
      color: var(--text);
      min-height: 72px;
      -webkit-user-select: none;
      user-select: none;
      touch-action: none;
      transition: transform .05s, background .1s;
    }
    .button:active, .button.pressed { background: var(--btn-active); transform: scale(.96); }
    .up    { grid-column: 2; grid-row: 1; }
    .left  { grid-column: 1; grid-row: 2; }
    .honk  { grid-column: 2; grid-row: 2; background: var(--honk); color: #201500; font-size: 20px; font-weight: 800; letter-spacing: .5px; }
    .honk:active, .honk.pressed { background: var(--honk-active); }
    .right { grid-column: 3; grid-row: 2; }
    .down  { grid-column: 2; grid-row: 3; }

    footer { font-size: 11px; color: var(--muted); text-align: center; }
  </style>
</head>
<body>
  <header>
    <h1>Carrito ESP32</h1>
    <div class="sub">Mantene presionado para mover</div>
  </header>

  <div id="status" class="status">
    <span class="label">Distancia:</span>
    <span class="value" id="statusValue">-- cm</span>
  </div>

  <div class="pad">
    <button class="button up"    data-cmd="/forward">&uarr;</button>
    <button class="button left"  data-cmd="/left">&larr;</button>
    <button class="button honk"  data-honk>HONK</button>
    <button class="button right" data-cmd="/right">&rarr;</button>
    <button class="button down"  data-cmd="/backward">&darr;</button>
  </div>

  <footer>Carrito unido a la red del host (modo estacion)</footer>

  <script>
    // Umbral de obstaculo. Debe coincidir con OBSTACLE_DISTANCE_CM en el sketch.
    var OBSTACLE_CM = 15;
    var ALERT_REPEAT_MS = 1000;  // re-alertar mientras siga bloqueado

    var heldCommand = null;

    function sendCommand(command) {
      fetch(command).catch(function (e) { console.log(e); });
    }

    // ---- Audio (Web Audio API) ----
    // El navegador bloquea el audio hasta que el usuario interactua; la primera
    // pulsacion de un boton lo desbloquea. iOS ademas exige resume() en ese gesto.
    var audioCtx = null;
    function unlockAudio() {
      try {
        if (!audioCtx) {
          var AC = window.AudioContext || window.webkitAudioContext;
          if (AC) audioCtx = new AC();
        }
        if (audioCtx && audioCtx.state === 'suspended') audioCtx.resume();
      } catch (e) { /* sin audio */ }
    }
    function beep() {
      if (!audioCtx) return;
      try {
        var osc = audioCtx.createOscillator();
        var gain = audioCtx.createGain();
        var t = audioCtx.currentTime;
        osc.type = 'square';
        osc.frequency.value = 880;
        gain.gain.setValueAtTime(0.0001, t);
        gain.gain.exponentialRampToValueAtTime(0.2, t + 0.01);
        gain.gain.exponentialRampToValueAtTime(0.0001, t + 0.15);
        osc.connect(gain); gain.connect(audioCtx.destination);
        osc.start(t); osc.stop(t + 0.16);
      } catch (e) { /* sin audio */ }
    }

    // ---- Alerta de obstaculo (beep + vibracion) ----
    // navigator.vibrate solo funciona en Android; iOS lo ignora sin error.
    var wasObstacle = false;
    var lastAlertTime = 0;
    function fireAlert() {
      beep();
      if (navigator.vibrate) navigator.vibrate(200);
    }

    // ---- Controles: press-and-hold para mover, tap para bocina ----
    // HONK no es un comando de movimiento: no fija heldCommand ni frena, para
    // que se pueda tocar la bocina sin detener el carrito en marcha.
    function press(btn) {
      unlockAudio();               // primer gesto desbloquea el audio
      btn.classList.add('pressed');
      if (btn.hasAttribute('data-honk')) { sendCommand('/honk'); return; }
      heldCommand = btn.getAttribute('data-cmd');
      sendCommand(heldCommand);
    }
    function release(btn) {
      btn.classList.remove('pressed');
      if (btn.hasAttribute('data-honk')) return;  // la bocina no afecta el movimiento
      heldCommand = null;
      sendCommand('/stop');
    }

    var buttons = document.querySelectorAll('.button');
    for (var i = 0; i < buttons.length; i++) {
      (function (btn) {
        btn.addEventListener('touchstart', function (e) { e.preventDefault(); press(btn); }, { passive: false });
        btn.addEventListener('touchend',   function (e) { e.preventDefault(); release(btn); }, { passive: false });
        btn.addEventListener('touchcancel',function (e) { e.preventDefault(); release(btn); }, { passive: false });
        btn.addEventListener('mousedown',  function (e) { e.preventDefault(); press(btn); });
        btn.addEventListener('mouseup',    function (e) { e.preventDefault(); release(btn); });
        btn.addEventListener('mouseleave', function () { if (btn.classList.contains('pressed')) release(btn); });
      })(buttons[i]);
    }

    // Heartbeat: reenvia el comando activo para alimentar el watchdog del ESP32
    setInterval(function () { if (heldCommand) sendCommand(heldCommand); }, 200);

    // Sondeo de distancia + motor de alertas
    var statusEl = document.getElementById('status');
    var valueEl = document.getElementById('statusValue');
    setInterval(function () {
      fetch('/status')
        .then(function (r) { return r.text(); })
        .then(function (cm) {
          var n = parseInt(cm, 10);
          var free = !(n < 999);
          valueEl.textContent = free ? 'libre' : (n + ' cm');
          var isObstacle = !free && n <= OBSTACLE_CM;

          statusEl.classList.toggle('alert', isObstacle);
          statusEl.classList.toggle('free', free);

          var now = Date.now();
          if (isObstacle && (!wasObstacle || now - lastAlertTime >= ALERT_REPEAT_MS)) {
            fireAlert();
            lastAlertTime = now;
          }
          wasObstacle = isObstacle;
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

// La bocina no toca el estado del movimiento: solo enciende el buzzer y
// deja que updateHonk() lo apague. Asi se puede tocar en marcha.
void handleHonk() {
  ledcWriteTone(BUZZER_PIN, HONK_FREQ_HZ);
  honkActive = true;
  honkUntil = millis() + HONK_DURATION_MS;
  server.send(200, "text/plain", "Honk");
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
