# Robot Móvil Teleoperado con Conducción Asistida y Visión IA

Prototipo de robot car sobre **ESP32** que se maneja a distancia por WiFi y que,
en su etapa final, **sigue objetos de forma autónoma** usando visión por
computadora corrida en una laptop. Todo el procesamiento es **local**: no depende
de la nube.

La idea central es una arquitectura de **dos sistemas** inspirada en el
pensamiento rápido/lento:

- **System 1**: rápido y local, sobre el ESP32: control de motores + freno
  ultrasónico de emergencia. Corre a velocidad de hardware y **siempre tiene la
  última palabra** sobre la IA.
- **System 2**: lento y remoto, sobre la laptop: la cámara alimenta una política
  de visión que decide y manda **un comando a la vez** al carrito, siempre sujeto
  al freno local.

> El MVP (System 1) funciona aunque la laptop esté apagada. La visión (System 2)
> es una capa de asistencia que se monta encima sin tocar el lazo de seguridad.

---

**Universidad de Costa Rica - Sistemas Empotrados de Tiempo Real (CI-0155)**
Prof. Ariel Mora Jiménez

**Integrantes:**
- Isabella Rodríguez Sánchez (C26701)
- Esteban Isaac Baires Cerdas (C10844)
- Jorge Ricardo Díaz Sagot (C12565)

---

## Demostraciones

| # | Demostración | Etapa | Enlace |
|---|--------------|-------|--------|
| 1 | Tracción básica adelante/atrás | Pruebas de concepto | [Ver video](./media/test-funcional-carrito-adelante-atras.mp4) |
| 2 | Control remoto con Dabble (fallido, incompatibilidad CircuitPython) | Pruebas de concepto | [Ver video](./media/test-funcional-dabble-control-remoto-fallido.mp4) |
| 3 | Primer control WiFi: UI web simple, sin bocina, **carrito 100 % autónomo en energía** | Teleoperación WiFi | [Ver video](./media/test_control_wifi_simple.mp4) |
| 4 | Control WiFi + ultrasónico: interfaz web, bocina (honk) y movimiento | Teleoperación WiFi | [Ver video](./media/demo-wifi-control-ultrasonic.mp4) |
| 5 | Interfaz web final (captura de pantalla) | Teleoperación WiFi | [Ver captura](./media/interfaz_web.png) |
| 6 | `teleop.py`: manejo del carrito con el teclado de la laptop | Visión IA / System 2 | [Ver video](./media/demo-teleop.mp4) |
| 7 | `follow.py`: el carrito sigue solo una pelota roja (webcam de la laptop) | Visión IA / System 2 | [Ver video](./media/demo-follow.mp4) |

---

## Etapas de desarrollo

El proyecto avanzó por checkpoints incrementales; cada uno es funcional y
demostrable por sí solo.

### Etapa 0: Propuesta teórica

Planeamiento inicial: componentes estimados, arquitectura de hardware/software y
público meta.

Documento: [Propuesta inicial](./propuesta-inicial.md)

### Etapa 1: Pruebas de concepto físicas

Validación del hardware real (no simulación): tracción y giro con rueda loca,
barrido del servo con el sensor ultrasónico HC-SR04, y aislamiento estructural.
Aquí se descartó **Dabble/Bluetooth** por incompatibilidad con CircuitPython, lo
que motivó el giro hacia control por **WiFi + HTTP**.

Documento: [Pruebas de concepto y viabilidad](./pruebas-de-concepto-y-viabilidad.md).
Demos 1 y 2. Código: [`code/dabble.ino`](./code/dabble.ino), [`code/mapped.ino`](./code/mapped.ino)

### Etapa 2: Teleoperación WiFi (MVP)

El ESP32 levanta su propia red (softAP) y sirve una **página web de control** con
botones press-and-hold. Se agregó el **sensor ultrasónico** como freno de
emergencia y alerta, una **bocina (honk)** no bloqueante, y una interfaz apta para
móvil. En esta etapa el carrito ya es **autónomo en energía** (batería 9V + mini
PSU), sin cables a la computadora.

Demos 3, 4 y 5. Código: [`code/wifi_control/`](./code/wifi_control/) (versión simple), [`code/wifi_control_ultrasonic/`](./code/wifi_control_ultrasonic/) (versión completa)

### Etapa 3: Independencia de red (modo estación)

El carrito deja de crear su propia red y se **une como cliente** a un router 2.4 GHz,
donde también viven la laptop y el teléfono-cámara. Toma una **IP fija** para que
la laptop siempre sepa a dónde mandar comandos. Esto habilita el pipeline de
visión sin tocar System 1.

Código: [`code/wifi_control_station/`](./code/wifi_control_station/).
Prueba: [`laptop/test_pipe.py`](./laptop/test_pipe.py) (verifica el canal laptop a carrito)

### Etapa 4: Visión IA (System 2)

La laptop cierra el lazo **cámara, decisión, comando**:

- **Teleoperación manual**: [`laptop/teleop.py`](./laptop/teleop.py): manejo del
  carrito con el teclado (WASD/flechas + bocina). Prueba la pila de comunicación
  completa. Demo 6.
- **Seguimiento autónomo por color**: [`laptop/follow.py`](./laptop/follow.py):
  visión clásica con OpenCV (blob HSV) que mantiene un objeto centrado y persigue
  la pelota roja. Es un System 2 que **no puede alucinar**, rápido y barato.
  Demo 7.

En ambos, el **freno ultrasónico local sigue mandando**: aunque la visión diga
"adelante", si hay un obstáculo a <15 cm el carrito no avanza.

---

## Entregable final

Un carrito que se demuestra en **tres modos**, de menor a mayor autonomía:

1. **Teleoperación web** desde el celular (solo el ESP32, sin laptop).
2. **Teleoperación por teclado** desde la laptop (`teleop.py`).
3. **Seguimiento autónomo** de un objeto por color (`follow.py`).

En los tres, el freno ultrasónico de emergencia del ESP32 nunca se apaga.

---

## Mapa del repositorio

```text
code/                         Firmware del ESP32 (Arduino / C++)
  dabble.ino                  PoC: recepción del gamepad Dabble por Bluetooth
  mapped.ino                  PoC: control PWM y mapeo de tracción
  wifi_control/               Control WiFi (softAP), UI web simple
  wifi_control_ultrasonic/    Control WiFi + freno ultrasónico + bocina
  wifi_control_station/       Firmware final, modo estación, IP fija
laptop/                       Scripts de la laptop (System 2, Python)
  test_pipe.py                Verifica el canal laptop a carrito
  teleop.py                   Teleoperación por teclado
  follow.py                   Seguimiento autónomo por color (OpenCV)
media/                        Videos y capturas de las demostraciones
propuesta-inicial.md          Documentación: propuesta teórica
pruebas-de-concepto-y-viabilidad.md   Documentación: prototipo real
```

## Cómo correr los scripts de la laptop

```bash
pip install opencv-python numpy requests

python laptop/test_pipe.py                 # ¿alcanza al carrito?
python laptop/teleop.py                     # manejar con el teclado
python laptop/follow.py --source 0          # seguir la webcam (solo visión)
python laptop/follow.py --no-drive          # visión sin mover el carrito
```

La IP fija del carrito debe coincidir entre el sketch y los scripts (`192.168.0.169`
por defecto). Todos los dispositivos deben estar en la **misma red 2.4 GHz de un
router** (un hotspot de teléfono con aislamiento de clientes rompe el enlace
entre la laptop y el carrito).
