# Pruebas de Concepto y Viabilidad (Prototipo Real)

[◀ Volver al Índice Principal](./README.md)

---

## Componentes del prototipo (Utilizados y Descartados)

### Componentes Utilizados (Base del Prototipo Final)

Para la construcción y validación del prototipo físico del robot móvil, se utilizaron los siguientes elementos:

- **Placa de desarrollo CRCibernetica IdeaBoard:** Actúa como el núcleo de procesamiento, equipada con el microcontrolador ESP32-WROOM-32E.
- **Chasis Básico (Smart Robot Car Chassis):** Base de acrílico estructural que da soporte físico al robot.
- **2 Motores DC con Llantas:** Acoplados en la parte trasera del chasis para proveer la fuerza de tracción principal, conectados a los puentes H lógicos de la IdeaBoard.
- **Rueda Auxiliar del Frente (Rodillo/Caster Wheel):** Rueda loca frontal que sustituye las ruedas de dirección rígidas iniciales para permitir giros ágiles de radio cero sobre superficies planas.
- **Microservo (1 x Servomotor):** Utilizado para proveer el movimiento giratorio (barrido de escaneo) de la base del sensor ultrasónico.
- **Sensor Ultrasónico de Distancia (1 x HC-SR04):** Sensor utilizado para la detección frontal de objetos en el rango de proximidad.
- **Soportes Estructurales de Cartón:** Utilizados provisionalmente para construir la pieza de soporte que acopla el sensor ultrasónico sobre el cabezal giratorio del servo.
- **Conexiones Eléctricas y Cableado:** Jumpers y cables de distribución para interconectar el servo, sensor y la placa IdeaBoard.

### Componentes Descartados y Razón del Descarte

- **Sensor IMU (Adafruit LSM6DS3TR-C) e Interfaz I2C:** Se descartó temporalmente en la implementación física para reducir la sobrecarga de lectura en el microcontrolador y simplificar la lógica de control primaria enfocada en el enlace inalámbrico y la tracción.
- **Algoritmo / Inferencia TinyML:** Descartado en esta etapa debido a restricciones de memoria SRAM y a que las lecturas y la lógica de evasión primaria pueden resolverse eficientemente mediante reglas de umbral directo, sin el costo computacional de inferencia local.
- **Buzzer Activo/Pasivo:** No se montó en las pruebas del prototipo actual para mantener el hardware básico y enfocar el consumo de corriente únicamente en la tracción y servocontrol.
- **Alimentación Unificada (4xAA compartido):** La propuesta original de alimentar la placa ESP32 y los motores con un único portabaterías de 4xAA falló en las pruebas preliminares de corriente. Se identificó la necesidad de separar las fuentes (compra de una batería cuadrada para la electrónica y baterías AA para potencia) para prevenir el bloqueo del ESP32 por caídas de tensión (*brownout*).

---

## Tecnologías y Frameworks Aplicados

- **Plataforma de Desarrollo de Código:** Se utilizó el software **IdeaCode** y el lenguaje **CircuitPython (Python)** para el firmware inicial del robot, buscando agilizar el prototipado y la lectura directa de los periféricos locales como el servo y el sensor ultrasónico.
- **Entorno de Programación y C++:** Se empleó **Arduino IDE con C++** para compilar y probar la compatibilidad de la pila inalámbrica. Los códigos fuentes resultantes se guardaron en la carpeta [code](./code), destacando [code/dabble.ino](./code/dabble.ino) (prueba serial del receptor inalámbrico) y [code/mapped.ino](./code/mapped.ino) (control PWM completo del carro teleoperado).
- **Protocolo de Control Inalámbrico (Fallo de Compatibilidad):** Se intentó emplear la aplicación móvil **Dabble** y su protocolo Bluetooth Serial. No obstante, se determinó que **Dabble no es compatible con CircuitPython** bajo la placa IdeaBoard de manera directa, ya que sus bibliotecas principales (`DabbleESP32`) están construidas y optimizadas exclusivamente en C++ para Arduino IDE (por lo cual se crearon las pruebas en `.ino`).
- **Control PWM de Motores:** Regulación analógica de la velocidad y giros del carrito a través del circuito de control integrado en la IdeaBoard.

---

## Pruebas de concepto del sistema empotrado

### Descripción de las pruebas de concepto

El equipo llevó a cabo tres pruebas de concepto físicas para verificar la viabilidad mecánica y electrónica del robot:

1. **Prueba de Tracción y Movimiento Básico (Adelante/Atrás/Giro):** Validación de que los motores DC responden en sentido y velocidad al puente H, y que el chasis de tracción trasera con rueda loca delantera gira con fluidez sobre su eje.
2. **Prueba de Enlace Dabble y Control Inalámbrico (Fallida):** Validación del intento de vinculación y control del robot en tiempo real desde la aplicación de Gamepad virtual Dabble utilizando firmware basado en CircuitPython.
3. **Prueba Unitaria de Servo y Sensor Ultrasónico (En Casa):** Validación en laboratorio doméstico del acople del microservo con el sensor ultrasónico HC-SR04 sobre soporte de cartón, comprobando que el sensor rota físicamente de izquierda a derecha simulando un barrido de escaneo.

### Implementación de pruebas

#### Software de Simulación (Wokwi)

**No se realizaron simulaciones de software.** De acuerdo con los requerimientos acordados con el profesor para esta entrega, los estudiantes podían elegir libremente entre realizar simulaciones o pruebas físicas sobre el hardware real. El equipo optó por desarrollar y probar el sistema directamente sobre los componentes físicos reales en su totalidad.

#### Componentes Seleccionados (Montaje Físico)

- Se armó el chasis de acrílico acoplando los 2 motores de engranajes en el eje trasero.
- Se instaló la rueda loca frontal ("rodillo") atornillada al chasis.
- La placa IdeaBoard se alimentó y conectó a los motores a través de las bornas de potencia de los puentes H.
- El microservo se cableó a la placa de desarrollo para alimentarlo con 5V y controlar su ángulo vía PWM. El sensor HC-SR04 se montó sobre el servo mediante piezas de cartón hechas a mano y se conectó a pines GPIO del ESP32.
- Se cargó el firmware [code/mapped.ino](./code/mapped.ino) mediante el Arduino IDE para testear la respuesta física de los motores a los botones del Gamepad de Dabble.

---

### Resultados de las pruebas

Las pruebas realizadas arrojaron los siguientes resultados y evidencias:

#### Prueba de Tracción y Movimiento Básico
- **Resultado:** **Exitoso**. Los motores DC tienen fuerza suficiente para desplazar el chasis sobre superficies planas. El rodillo auxiliar delantero permitió que el robot cambiara de dirección de manera suave y diera vueltas con un radio de giro óptimo.
- **Evidencia en Video:**  
  ![Test de Movimiento Adelante/Atrás](D:/SomeCode/sistemas-empotrados/proj-remote-control-robot/media/test-funcional-carrito-adelante-atras.mp4)

#### Prueba de Enlace Dabble y Control Inalámbrico
- **Resultado:** **Fallido**. Se comprobó la incompatibilidad entre la app Dabble y el entorno CircuitPython del IdeaBoard. El enlace no pudo establecerse debido a la falta de drivers de Dabble adaptados a Python en el microcontrolador.
- **Evidencia en Video:**  
  ![Prueba de Control Remoto con Dabble Fallido](D:/SomeCode/sistemas-empotrados/proj-remote-control-robot/media/test-funcional-dabble-control-remoto-fallido.mp4)

#### Prueba de Servo y Sensor Ultrasónico
- **Resultado:** **Exitoso**. La prueba realizada por el equipo de forma independiente validó la respuesta del servomotor a las señales PWM de control, barriendo un rango angular completo de izquierda a derecha. El sensor ultrasónico HC-SR04 capturó distancias a objetos correctamente durante el giro mecánico sobre el soporte de cartón.

---

### Cambios al sistema empotrado propuesto

Con base en la experimentación directa sobre el prototipo físico, se aplican los siguientes cambios de diseño y planificación:

1. **Reevaluación del Lenguaje y Protocolo de Control:**
   - Para resolver el fallo de conectividad inalámbrica, el equipo evalúa dos alternativas técnicas antes de la entrega final:
     - *Opción A:* Migrar todo el firmware del robot de **CircuitPython a C++** utilizando Arduino IDE o PlatformIO, lo cual habilitará la librería nativa `DabbleESP32` y solucionará inmediatamente el enlace inalámbrico por Bluetooth.
     - *Opción B:* Continuar el desarrollo en CircuitPython e investigar o desarrollar una aplicación de control remoto genérica (tipo terminal de Bluetooth Serial) que sea compatible con librerías nativas de Python.
2. **Rediseño Físico de Soporte y Dirección:**
   - Confirmación del uso del rodillo auxiliar delantero (rueda loca) en lugar de una dirección rígida o tracción delantera, debido a su mayor soltura en giros cerrados.
   - Reemplazo futuro de los soportes estructurales de cartón del sensor/servo por piezas definitivas de plástico rígido o impresión 3D para mayor estabilidad.
3. **Modificación del Esquema de Alimentación:**
   - Implementar de forma definitiva un esquema de **fuentes de alimentación independientes**: se requiere comprar una batería cuadrada (9V) para alimentar de manera aislada la lógica del ESP32, manteniendo el portabaterías AA exclusivamente para proveer corriente a los motores DC y prevenir caídas de voltaje de control (*brownouts*).

---
[◀ Volver al Índice Principal](./README.md)
