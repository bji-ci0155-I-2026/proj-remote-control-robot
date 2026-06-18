#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

#include <DabbleESP32.h>

void setup() {
  Serial.begin(115200);

  Dabble.begin("ESP32_Robot");  // Nombre Bluetooth que verá el celular
  Serial.println("Dabble listo. Busca ESP32_Robot en la app.");
}

void loop() {
  Dabble.processInput();

  if (GamePad.isUpPressed()) {
    Serial.println("Arriba");
  }

  if (GamePad.isDownPressed()) {
    Serial.println("Abajo");
  }

  if (GamePad.isLeftPressed()) {
    Serial.println("Izquierda");
  }

  if (GamePad.isRightPressed()) {
    Serial.println("Derecha");
  }
}