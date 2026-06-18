# Pruebas de Concepto y Viabilidad (Prototipo Real)

[◀ Volver al Índice Principal](./README.md)

---

## Componentes del prototipo (Utilizados y Descartados)

Para la implementación del prototipo físico, se seleccionaron los componentes que demostraron viabilidad y rendimiento óptimo en la práctica. Asimismo, varios componentes del planeamiento inicial fueron descartados o modificados por razones técnicas y de estabilidad eléctrica:

### Componentes Utilizados (Base del Prototipo Final):

- **Placa de desarrollo CRCibernetica Ideaboard:** Núcleo de control del robot.
  - **Microcontrolador:** ESP32-WROOM-32E (Xtensa dual-core de 32 bits LX6 a 240 MHz). Su coprocesador de conectividad y núcleos de ejecución fueron indispensables para correr concurrentemente la recepción de paquetes de control inalámbrico y el bucle de sensores no bloqueantes.
  - **Controlador de Motores Integrado:** Puentes H duales (Dual H-Bridges) integrados en la placa IdeaBoard. Permitieron controlar el sentido de giro y la velocidad de los motores DC mediante modulación por ancho de pulsos (PWM) con una corriente de hasta 800 mA.
- **Sistema de Movimiento (Chasis 2WD):**
  - **Smart Robot Car Chassis:** Se configuró para tracción trasera utilizando **2 x Motores de engranajes DC (Gear Motors)** y llantas acopladas en la parte trasera, controladas de manera independiente por los puentes H para permitir giros sobre su propio eje.
  - **Apoyo y dirección:** Se utilizaron ruedas locas delanteras en lugar de tracción delantera fija para reducir la fricción y mejorar la maniobrabilidad en giros cerrados sobre superficies planas.
- **Sensores de Distancia (2 x HC-SR04):**
  - Sensores ultrasónicos colocados estratégicamente: uno en la parte frontal y otro en la trasera. Operan mediante pines GPIO digitales (**TRIG** y **ECHO**). Miden de forma continua la distancia a los obstáculos. Su lógica fue adaptada de llamadas bloqueantes (`pulseIn()`) a lecturas controladas por temporizador no bloqueante para no interferir con la recepción de comandos de movimiento del control remoto.
- **Actuador de Alerta (1 x Buzzer pasivo/activo):**
  - Conectado a un pin GPIO digital de la IdeaBoard. Emite alertas sonoras con una frecuencia e intervalo proporcionales a la cercanía del objeto detectado, creando una alerta acústica de proximidad que previene al operario.
- **Sistema de Alimentación Independiente (Rediseñado):**
  - **1x Power Bank USB (5V):** Utilizado para alimentar exclusivamente la placa IdeaBoard y el microcontrolador ESP32 por medio del puerto micro-USB, garantizando una corriente estable y libre de caídas de voltaje.
  - **1x Portabaterías (4xAA - 6V nominales):** Utilizado para alimentar de manera exclusiva la línea de potencia de los motores a través del puente H, aislando el ruido eléctrico y la demanda de corriente de los motores del circuito de control.

### Componentes Descartados y Razón del Descarte:

- **Sensor IMU (Adafruit LSM6DS3TR-C) e Interfaz I2C:** Se descartó para el prototipo físico. La lógica heurística basada en los sensores ultrasónicos demostró ser suficiente para las tareas de frenado de emergencia y asistencia a la conducción, reduciendo la complejidad del cableado y evitando la sobrecarga de datos en el bus I2C.
- **Modelo TinyML (Edge AI) / Frameworks de Inferencia:** Se descartó debido a las restricciones de memoria SRAM e inestabilidades de compilación al integrar de forma simultánea Bluetooth Classic (Dabble), control de motores y modelos de TensorFlow Lite Micro. El algoritmo de umbral de distancia no bloqueante implementado en C++ resolvió la asistencia de conducción con menor consumo y retardo nulo.
- **Microservo y Base Giratoria para Ultrasonido:** Se descartó para simplificar el hardware y evitar retrasos en el ciclo de barrido. La presencia de sensores ultrasónicos fijos dedicados en ambos extremos (delantero y trasero) ofreció una cobertura de detección más rápida y confiable que un único sensor rotativo con servo.
- **Sensores de Odometría (Track Sensor & Rotary Sensor):** Descartados al priorizar la teleoperación manual asistida sobre la navegación autónoma.
- **Sensores Ambientales (DHT11, Lux Sensor VEML770, Touch Sensors):** Descartados por no aportar funcionalidad directa al MVP enfocado en conducción preventiva y evasión de obstáculos.

---

## Tecnologías y Frameworks Aplicados

Para el desarrollo del prototipo de robot móvil asistido se aplicaron las siguientes tecnologías:

- **Protocolo de Comunicación Inalámbrica (Bluetooth Classic):**
  - Se utilizó la pila de comunicación Bluetooth integrada del ESP32 mediante la librería de C++ **DabbleESP32**. Esto permitió establecer un canal serial virtual muy robusto con la aplicación móvil **Dabble** instalada en el dispositivo móvil del operario.
  - Se descartó el protocolo Wi-Fi debido a que introducía mayor latencia en la transmisión de comandos del Gamepad y consumía más memoria de programa, lo cual dificultaba la ejecución estable del firmware.
- **Interfaz Gráfica de Control (Dabble Mobile App):**
  - Se implementó el módulo **GamePad** de la aplicación móvil Dabble, que proporciona un joystick virtual analógico y botones de acción en el teléfono celular, permitiendo enviar datos de dirección y velocidad en paquetes estructurados de forma inmediata.
- **Control de Actuación por Modulación por Ancho de Pulsos (PWM):**
  - Se utilizó PWM a través de las APIs nativas del ESP32 para dosificar la energía entregada a los motores a través de los puentes H lógicos de la IdeaBoard, regulando con precisión la velocidad angular de las ruedas del robot.
- **Entorno y Lenguaje de Programación:**
  - El firmware se escribió en **C++ bajo el IDE de Arduino**, aprovechando las librerías optimizadas para el ESP32. Se evaluó el uso de CircuitPython, pero se descartó debido a que no ofrecía soporte nativo para la librería DabbleESP32 y presentaba latencias considerables en el procesamiento de entradas de sensores en tiempo real.
- **Software de Simulación de Circuitos (Wokwi):**
  - Se utilizó Wokwi para construir y simular el esquema lógico de la placa IdeaBoard/ESP32, la conexión del buzzer, el cableado de los pines TRIG y ECHO del sensor ultrasónico y la emulación del puente H con motores. Ayudó a validar la lógica de control y lectura antes de transferir el firmware a los componentes físicos.

---

## Pruebas de concepto del sistema empotrado

### Descripción de las pruebas de concepto

Para garantizar la viabilidad del robot móvil teleoperado y asistido, se definieron e implementaron cuatro pruebas de concepto esenciales:

1. **Prueba de Enlace Inalámbrico y Teleoperación:**
   - *Objetivo:* Verificar la vinculación Bluetooth Classic entre la placa IdeaBoard (ESP32) y la aplicación Dabble en el teléfono móvil, asegurando que las pulsaciones del joystick virtual se reciban correctamente en el microcontrolador y se traduzcan en comandos de dirección (Avanzar, Retroceder, Girar a la Izquierda, Girar a la Derecha, Detener).
2. **Prueba de Lectura no Bloqueante de Sensores Ultrasónicos:**
   - *Objetivo:* Validar la medición de distancias en los sensores frontal y trasero simultáneamente. La prueba consistió en acercar objetos a los sensores HC-SR04 y verificar en la consola serial que las distancias medidas coincidieran con la realidad, garantizando que el ciclo de lectura no congelara el procesador (evitando retrasar la recepción de comandos Bluetooth).
3. **Prueba de Alarma Acústica Progresiva:**
   - *Objetivo:* Probar el funcionamiento del Buzzer. Se programó una señal para emitir pitidos intermitentes cuya frecuencia aumentara a medida que un obstáculo se aproximara a cualquiera de los sensores, culminando en un sonido continuo cuando el obstáculo ingresara a la zona crítica de colisión (< 15 cm).
4. **Prueba de Integración de Conducción Asistida (Evasión de Obstáculos):**
   - *Objetivo:* Integrar los sensores y el movimiento. El robot debía responder libremente al control manual del usuario, pero si avanzaba en dirección a un obstáculo y la distancia medida por el sensor ultrasónico respectivo caía por debajo de 15 cm, el sistema debía anular de inmediato la orden de tracción en ese sentido (forzando un frenado automático) permitiendo al usuario únicamente retroceder o girar para salir del peligro.

---

### Implementación de pruebas

#### Software de Simulación (Wokwi)

Antes de integrar el hardware físico, se implementó un circuito virtual en el simulador online **Wokwi** para validar la lógica del código y el mapeo de pines del ESP32. 

**Características de la Simulación:**
- Se configuró un microcontrolador ESP32 simulando la placa IdeaBoard.
- Se conectaron dos sensores ultrasónicos HC-SR04 al microcontrolador compartiendo pines de alimentación pero con pines de control independientes (`TRIG` y `ECHO`).
- Los puentes H integrados de la placa física se simularon mediante dos motores DC virtuales conectados a salidas PWM del ESP32, permitiendo ver el sentido de giro y la velocidad en la animación digital.
- Se integró un Buzzer piezoeléctrico virtual para emitir tonos de frecuencia según la lectura de distancia de los sensores.

*Lógica del firmware simulado (C++):*
Se diseñó un bucle principal no bloqueante utilizando temporizadores virtuales (`millis()`) en lugar de `delay()`. Esto permitió que la lectura del sensor HC-SR04 se realizara cada 50 ms sin bloquear la ejecución general. Al reducir virtualmente la distancia en el sensor ultrasónico en la interfaz interactiva de Wokwi, se pudo comprobar que:
1. Las salidas PWM a los motores se ponían a cero instantáneamente al cruzar el umbral crítico de 15 cm.
2. El Buzzer respondía incrementando el tono de la alarma.

Esta prueba digital demostró la viabilidad de la lógica de evasión y aseguró que no existieran conflictos de asignación de pines GPIO antes del montaje físico.

#### Componentes Seleccionados (Montaje Físico)

Para la implementación física del prototipo, se ensamblaron los componentes en el chasis del robot siguiendo el siguiente mapeo y estructura de hardware:

1. **Ensamblado Mecánico:** Se montaron los motores traseros acoplados al chasis de acrílico. Se instalaron las dos ruedas locas delanteras para soporte y giros rápidos. La placa IdeaBoard se atornilló en el nivel superior del chasis para facilitar el acceso a la antena y los conectores.
2. **Conexiones Eléctricas:**
   - **Motores DC:** Se conectaron a los terminales de salida del puente H integrado (Bornas de tornillo de la IdeaBoard).
   - **Sensor Ultrasónico Frontal (HC-SR04-F):** TRIG conectado a GPIO 12 y ECHO a GPIO 13.
   - **Sensor Ultrasónico Trasero (HC-SR04-R):** TRIG conectado a GPIO 14 y ECHO a GPIO 27.
   - **Buzzer:** Conectado a GPIO 26.
3. **Distribución de Alimentación (Rediseño Crítico):**
   - El portabaterías de 4xAA (aprox. 6V) se cableó directamente a la borna de alimentación de potencia de los motores en la IdeaBoard.
   - Se conectó un Power Bank portátil de 5V al puerto micro-USB de la placa mediante un cable USB corto y flexible.
   - Ambos circuitos compartieron la masa común a través del plano de tierra interna de la placa de desarrollo IdeaBoard, aislando los ruidos eléctricos inducidos por los arranques de los motores y estabilizando el procesador ESP32.

---

### Resultados de las pruebas

Las pruebas de concepto arrojaron los siguientes resultados de rendimiento sobre el prototipo:

| Prueba de Concepto | Estado de Viabilidad | Observaciones / Diagnóstico de Funcionamiento |
| :--- | :--- | :--- |
| **1. Enlace Inalámbrico y Teleoperación** | **Exitoso** | La comunicación Bluetooth Classic con Dabble fue estable en un rango de hasta 10 metros. La latencia de comandos fue imperceptible, permitiendo una maniobrabilidad precisa y fluida del robot móvil. |
| **2. Lectura de Sensores Ultrasónicos** | **Exitoso (con corrección)** | Las lecturas iniciales causaban una interrupción del movimiento del robot. Al investigar, se determinó que la función `pulseIn()` bloqueaba el microcontrolador hasta 30 ms por lectura si no había obstáculo detectado. Se modificó el código para realizar lecturas asíncronas no bloqueantes por temporización, logrando lecturas estables cada 45 ms sin afectar la tracción ni la teleoperación. |
| **3. Alarma Acústica Progresiva** | **Exitoso** | El buzzer funcionó conforme a la lógica. Se logró una escala sonora progresiva (pitido espaciado a 30 cm, pitido rápido a 20 cm, y pitido continuo al entrar en el umbral de detención automática de 15 cm). |
| **4. Integración de Conducción Asistida** | **Exitoso** | La fusión de sensores lógicos de proximidad y control manual funcionó perfectamente. Si el operario intentaba avanzar de frente a un muro, el robot frenaba a 15 cm de distancia y anulaba la tracción delantera. Al presionar el joystick hacia atrás, el robot retrocedía libremente alejándose del obstáculo, permitiendo retomar la conducción segura. |

---

### Cambios al sistema empotrado propuesto

A partir de los resultados experimentales y las pruebas de concepto, se realizaron modificaciones clave sobre la propuesta teórica original para asegurar la viabilidad del prototipo funcional:

1. **Aislamiento Eléctrico de Alimentación (Doble Circuito):**
   - *Propuesta Original:* Alimentar todo el sistema (ESP32 y motores) con un único portabaterías de 4xAA.
   - *Cambio Aplicado:* Se independizó la alimentación. El portabaterías AA alimentó exclusivamente la potencia de los motores, mientras que la lógica y el ESP32 se alimentaron desde un Power Bank USB externo de 5V. Este cambio evitó los reinicios espontáneos (*brownouts*) del ESP32 por caídas de tensión que ocurrían cada vez que los motores iniciaban el movimiento desde cero.
2. **Reemplazo de TinyML por Heurísticas de Reglas:**
   - *Propuesta Original:* Correr un modelo de TinyML (TensorFlow Lite) localmente para predecir colisiones basándose en datos del sensor IMU y ultrasonidos.
   - *Cambio Aplicado:* Se eliminó la IMU y el framework de TinyML. El modelo teórico requería demasiada memoria flash y restaba tiempo de CPU a la comunicación Bluetooth en tiempo real. Se sustituyó exitosamente por un algoritmo no bloqueante de umbralización basado en reglas en C++, logrando la evasión de colisiones de forma instantánea y con un consumo de recursos mínimo.
3. **Optimización del Bucle de Control (Lecturas No Bloqueantes):**
   - *Propuesta Original:* Lectura directa de sensores mediante funciones estándar de Arduino.
   - *Cambio Aplicado:* Se implementó un programador de tareas simple basado en temporizadores virtuales (`millis()`) en el firmware de control para consultar los sensores HC-SR04 en intervalos discretos, erradicando las demoras producidas por llamadas bloqueantes y asegurando una respuesta instantánea a los comandos del joystick Bluetooth del operario.
4. **Configuración del Chasis 2WD Efectivo:**
   - *Propuesta Original:* Chasis 4WD adaptado a 2WD trasero con 2 ruedas delanteras libres acopladas sin motores.
   - *Cambio Aplicado:* Se confirmó la estabilidad del chasis de dos motores traseros y rueda loca delantera, ofreciendo giros de radio cero muy eficientes en entornos planos e interiores, óptimo para pruebas académicas en laboratorios.

---
[◀ Volver al Índice Principal](./README.md)
