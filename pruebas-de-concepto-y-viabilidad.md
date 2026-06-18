# Pruebas de Concepto y Viabilidad (Prototipo Real)

[◀ Volver al Índice Principal](./README.md)

---

## Componentes del prototipo (Utilizados y Descartados)

### Componentes Utilizados (Base del Prototipo Final)

Para la construcción y validación del prototipo físico del robot móvil, se utilizaron los siguientes elementos:

- **Placa de desarrollo CRCibernetica IdeaBoard:** Actúa como el núcleo de procesamiento, equipada con el microcontrolador ESP32-WROOM-32E.
- **Chasis Básico (Smart Robot Car Chassis):** Base de acrílico estructural que da soporte físico al robot.
- **2 Motores DC con Llantas:** Acoplados en la parte trasera del chasis para la tracción.
- **Rueda Auxiliar del Frente (Rodillo/Caster Wheel sin tracción):** Rueda loca frontal que sustituye las ruedas de dirección rígidas iniciales para permitir giros ágiles de radio cero y facilitar la construcción.
- **Microservo (1 x Servomotor):** Utilizado para el movimiento giratorio (barrido de escaneo) del sensor ultrasónico.
- **Sensor Ultrasónico de Distancia (1 x HC-SR04):** Sensor utilizado para la detección frontal de objetos en el rango de proximidad.
- **Batería Cuadrada de 9V con su Clip:** Provee energía autónoma al robot car, evitando que dependa de estar conectado físicamente a una computadora o enchufe de pared.
- **Módulo de Fuente de Poder (Mini PSU / Power Supply de prototipado):** Cuenta con puerto de barril, puerto USB-A y múltiples pines de salida. Distribuye el voltaje de la batería de 9V de forma estable hacia los motores y la placa IdeaBoard.
- **Soportes Estructurales de Cartón:** Prototipos provisionales utilizados para montar el sensor ultrasónico en el servo y para separar/aislar físicamente la placa IdeaBoard de la fuente de poder (mini PSU) y sus conexiones.
- **Conexiones Eléctricas y Cableado:** Jumpers y cables de distribución para interconectar el hardware.

### Componentes Descartados y Razón del Descarte

- **Sensor IMU (Adafruit LSM6DS3TR-C) e Interfaz I2C:** Se descartó temporalmente para simplificar la lógica de control primaria enfocada en el enlace inalámbrico y la tracción.
- **Algoritmo / Inferencia TinyML:** Descartado en esta etapa debido al tiempo de la entrega.
- **Buzzer Activo/Pasivo:** No se montó en las pruebas del prototipo actual para mantener el hardware básico y enfocar el consumo de corriente únicamente en la tracción y servocontrol.

---

## Tecnologías y Frameworks Aplicados

- **Plataforma de Desarrollo de Código y Lenguaje (Pruebas Realizadas):** Se utilizó el **Arduino IDE** y el lenguaje **C++** para compilar, cargar y ejecutar las pruebas físicas actuales del prototipo. Los códigos fuente desarrollados se guardaron en la carpeta [code](./code), destacando [code/dabble.ino](./code/dabble.ino) (prueba serial de recepción del control inalámbrico) y [code/mapped.ino](./code/mapped.ino) (control PWM completo y mapeo de tracción del robot).
- **Entorno Evaluado (IdeaCode y CircuitPython):** Se probó y evaluó el entorno **IdeaCode** (IDE propietario de la placa IdeaBoard) utilizando **CircuitPython**. Aunque se desea utilizar este entorno para el firmware final, se identificaron limitaciones de compatibilidad inicial para comunicar la librería estándar de Dabble vía Bluetooth Classic, por lo cual las pruebas funcionales actuales se realizaron bajo C++ (`.ino`).
- **Protocolo de Control Inalámbrico:** Se utilizó la aplicación móvil **Dabble** (módulo Gamepad) mediante Bluetooth Classic para la teleoperación del robot car, procesada a través de la librería `DabbleESP32` en el código de C++ (`.ino`).
- **Control PWM de Motores:** Regulación analógica de la velocidad y giros de las ruedas a través del circuito de control integrado en la IdeaBoard.

---

## Pruebas de concepto del sistema empotrado

### Descripción de las pruebas de concepto

El equipo llevó a cabo cuatro pruebas de concepto físicas para verificar la viabilidad mecánica y electrónica del robot:

1. **Prueba de Tracción y Movimiento Básico (Adelante/Atrás/Giro):** Validación de que los motores DC responden en sentido y velocidad al puente H, y que el chasis de tracción trasera con rueda loca delantera gira con fluidez sobre su eje.
2. **Prueba de Enlace Dabble y Control Inalámbrico (Fallida):** Validación del intento de vinculación y control del robot en tiempo real desde la aplicación de Gamepad virtual Dabble utilizando firmware basado en CircuitPython.
3. **Prueba Unitaria de Servo y Sensor Ultrasónico (En Casa):** Validación en laboratorio doméstico del acople del microservo con el sensor ultrasónico HC-SR04 sobre soporte de cartón, comprobando que el sensor rota físicamente de izquierda a derecha simulando un barrido de escaneo.
4. **Prueba de Aislamiento y Soporte Estructural (Cartón):** Validación de que el montaje con piezas de cartón provisionales separa de forma segura la placa IdeaBoard del módulo de fuente de poder (mini PSU) y sus conexiones eléctricas para evitar fallas o cortocircuitos.

### Implementación de pruebas

#### Software de Simulación (Wokwi)

**No se realizaron simulaciones de software.** De acuerdo con los requerimientos acordados con el profesor para esta entrega, los estudiantes podían elegir libremente entre realizar simulaciones o pruebas físicas sobre el hardware real. El equipo optó por desarrollar y probar el sistema directamente sobre los componentes físicos reales en su totalidad.

#### Componentes Seleccionados (Montaje Físico)

- Se armó el chasis de acrílico acoplando los 2 motores en el eje trasero.
- Se instaló la rueda loca frontal ("rodillo") atornillada al chasis.
- La placa IdeaBoard se montó y conectó a los motores a través de las bornas de potencia de los puentes H.
- El microservo se cableó a la placa de desarrollo para alimentarlo con 5V y controlar su ángulo vía PWM. El sensor HC-SR04 se montó sobre el servo mediante piezas de cartón hechas a mano y se conectó a pines GPIO del ESP32.
- Se instaló la batería de 9V con su respectivo clip conectada al módulo de fuente de poder (mini PSU) para otorgar autonomía energética al prototipo.
- Las piezas de cartón provisionales se colocaron para separar físicamente la placa de control IdeaBoard de las conexiones eléctricas de la fuente de poder (mini PSU).
- Se cargó el firmware [code/mapped.ino](./code/mapped.ino) mediante el Arduino IDE para testear la respuesta física de los motores a los botones del Gamepad de Dabble.

---

### Resultados de las pruebas

Las pruebas realizadas arrojaron los siguientes resultados y evidencias:

#### Prueba de Tracción y Movimiento Básico

- **Resultado:** **Exitoso**. Los motores DC tienen fuerza suficiente para desplazar el chasis sobre superficies planas. El rodillo auxiliar delantero permitió que el robot cambiara de dirección de manera suave y diera vueltas con un radio de giro óptimo.
- **Evidencia en Video:**  
  [Test de Movimiento Adelante/Atrás](./media/test-funcional-carrito-adelante-atras.mp4)

#### Prueba de Enlace Dabble y Control Inalámbrico

- **Resultado:** **Fallido**. Se comprobó la incompatibilidad entre la app Dabble y el entorno CircuitPython del IdeaBoard. El enlace no pudo establecerse debido a la falta de drivers de Dabble adaptados a Python en el microcontrolador.

- **Evidencia en Video:**  
  [Prueba de Control Remoto con Dabble Fallido](./media/test-funcional-dabble-control-remoto-fallido.mp4)

#### Prueba de Servo y Sensor Ultrasónico

- **Resultado:** **Exitoso**. La prueba realizada por el equipo de forma independiente validó la respuesta del servomotor a las señales PWM de control, barriendo un rango angular completo de izquierda a derecha. El sensor ultrasónico HC-SR04 capturó distancias a objetos correctamente durante el giro mecánico sobre el soporte de cartón.

#### Prueba de Aislamiento y Soporte Estructural (Cartón)

- **Resultado:** **Exitoso**. Las piezas provisionales de cartón sirvieron adecuadamente como aislante físico entre la placa IdeaBoard y el módulo de la fuente de poder (mini PSU). Lograron separar las conexiones expuestas para evitar cortocircuitos eléctricos durante el movimiento del carrito. El prototipo cumplió su función estructural, aunque se requerirá fabricarlo en otro material más estable en la versión final.

---

### Cambios al sistema empotrado propuesto

Con base en la experimentación directa sobre el prototipo físico, se aplican los siguientes cambios de diseño y planificación:

1. **Migración Futura a IdeaCode y CircuitPython:**
   - Aunque las pruebas funcionales de teleoperación actuales se resolvieron en C++ (`.ino`) a través del Arduino IDE por la compatibilidad directa de la librería de Dabble, el equipo planea a futuro migrar todo el desarrollo del robot a **IdeaCode y CircuitPython** para aprovechar el entorno de programación propietario de la IdeaBoard. Para lograr esto, se investigará y evaluará el uso de una aplicación de control inalámbrico alternativa o el desarrollo de un módulo de comunicación personalizado que funcione nativamente con CircuitPython en el ESP32.
2. **Rediseño Físico de Soporte y Dirección:**
   - Confirmación del uso del rodillo auxiliar delantero (rueda sin tracción) en lugar de una dirección rígida o tracción delantera, debido a su mayor facilidad de construcción.
   - Reemplazo futuro de los soportes estructurales de cartón del sensor/servo por piezas definitivas de plástico rígido o impresión 3D para mayor estabilidad.
3. **Autonomía y Uso de Fuente de Poder (Mini PSU):**
   - Se implementó el uso de la batería de 9V con clip junto con el módulo de fuente de poder (mini PSU) para alimentar la placa de control IdeaBoard y los motores de forma segura.

### Opciones de Control Remoto y Bluetooth en Evaluación

Como parte del rediseño para superar la incompatibilidad de Dabble en CircuitPython y explorar mejores alternativas de control remoto, el equipo evalúa las siguientes opciones según el entorno de desarrollo definitivo:

**CircuitPython:**

- **Adafruit Bluefruit LE Connect:** Uso de la librería oficial `adafruit_ble` para levantar un servicio UART por BLE.
- **dabble-circuitpython:** Adaptación de la librería de Dabble desarrollada por la comunidad (Eric Zundel).
- **Bluepad32 para CircuitPython:** Port para vincular gamepads y mandos físicos inalámbricos directamente al microcontrolador.

**Arduino IDE (C++):**

- **Blynk / Blynk IoT:** Plataforma móvil para crear interfaces gráficas personalizadas con botones y sliders de control.
- **ESP32 BLE Gamepad:** Biblioteca para emular un mando inalámbrico estándar reconocido por cualquier dispositivo.
- **Bluetooth Classic Serial:** Control serial simple a través de la librería nativa `BluetoothSerial.h` y aplicaciones de terminal de consola Bluetooth.

---
[◀ Volver al Índice Principal](./README.md)
