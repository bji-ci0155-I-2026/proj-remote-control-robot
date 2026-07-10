#!/usr/bin/env python3
"""
Rung 1 - teleoperacion manual laptop -> carrito.

Manejas el carrito con el teclado desde la terminal de la laptop. Es la mitad
inversa del viejo proyecto Just Dance: en vez de leer gestos, aqui mandas
comandos por HTTP a la IP fija del carrito y este mueve los motores.

El freno ultrasonico local (System 1) sigue mandando por encima: si hay un
obstaculo < 15 cm al frente, el carrito NO avanza aunque mantengas 'W'.

Controles:
    W / flecha arriba    avanzar
    S / flecha abajo     retroceder
    A / flecha izquierda girar izquierda
    D / flecha derecha   girar derecha
    H                    bocina (HONK, no detiene el movimiento)
    barra espaciadora    detener ya
    Q                    salir (deja el carrito detenido)

Manten la tecla presionada para seguir moviendote (igual que el press-and-hold
de la pagina web). Al soltar, el carrito se detiene solo.

Uso:
    python teleop.py                # usa la IP por defecto
    python teleop.py --ip 10.0.0.42 # otra IP

Requiere:  pip install requests    (msvcrt es estandar en Windows)
"""

import argparse
import sys
import time

import requests

try:
    import msvcrt  # solo Windows; este proyecto corre en Windows
except ImportError:
    print("[ERROR] teleop.py usa msvcrt (Windows). En otro SO habria que adaptarlo.")
    sys.exit(1)

CAR_IP = "192.168.0.169"      # IP fija del carrito (debe coincidir con el sketch)
TIMEOUT = 1.0                # s por request; corto para no trabar el manejo

# Cada cuanto reenviar el comando activo para alimentar el watchdog del ESP32
# (COMMAND_TIMEOUT_MS = 600 ms en el sketch). 150 ms deja buen margen.
HEARTBEAT_S = 0.15

# El auto-repeat del teclado reinyecta la tecla cada ~30 ms mientras la mantienes.
# Si pasan mas de KEY_HOLD_TIMEOUT_S sin repeticion, asumimos que soltaste -> stop.
KEY_HOLD_TIMEOUT_S = 0.25

# Mapa de teclas -> endpoint del carrito
KEY_TO_PATH = {
    "w": "/forward",
    "s": "/backward",
    "a": "/left",
    "d": "/right",
}
# Flechas: msvcrt las entrega como un prefijo (0x00 o 0xE0) + un segundo byte
ARROW_TO_PATH = {
    "H": "/forward",   # arriba
    "P": "/backward",  # abajo
    "K": "/left",      # izquierda
    "M": "/right",     # derecha
}


def send(session, ip, path):
    """Manda un comando al carrito; devuelve True si respondio bien."""
    try:
        session.get(f"http://{ip}{path}", timeout=TIMEOUT)
        return True
    except requests.exceptions.RequestException:
        return False


def read_keys():
    """Lee todas las teclas pendientes (no bloqueante). Devuelve lista de paths.

    'stop'/'quit' se devuelven como marcadores especiales.
    """
    events = []
    while msvcrt.kbhit():
        ch = msvcrt.getch()
        if ch in (b"\x00", b"\xe0"):          # prefijo de tecla especial (flechas)
            code = msvcrt.getch().decode("latin-1", "ignore")
            path = ARROW_TO_PATH.get(code)
            if path:
                events.append(path)
            continue
        c = ch.decode("latin-1", "ignore").lower()
        if c == "q":
            events.append("__quit__")
        elif c == " ":
            events.append("__stop__")
        elif c == "h":
            events.append("__honk__")
        elif c in KEY_TO_PATH:
            events.append(KEY_TO_PATH[c])
    return events


def main():
    ap = argparse.ArgumentParser(description="Teleoperacion laptop->carrito")
    ap.add_argument("--ip", default=CAR_IP, help="IP del carrito")
    args = ap.parse_args()

    session = requests.Session()

    # Chequeo rapido de alcanzabilidad antes de dar el control
    print(f"[teleop] conectando a http://{args.ip} ...")
    if not send(session, args.ip, "/status"):
        print("[ERROR] no se pudo alcanzar el carrito.")
        print("  - misma red que el carrito (router)?")
        print("  - la IP coincide con la del serial?")
        sys.exit(1)

    print("[teleop] listo. WASD/flechas=manejar, H=bocina, ESPACIO=stop, Q=salir.\n")

    current_path = None       # comando activo (endpoint) o None
    last_key_time = 0.0       # ultima vez que se vio la tecla del comando activo
    last_beat = 0.0           # ultimo heartbeat enviado

    try:
        while True:
            now = time.time()

            for ev in read_keys():
                if ev == "__quit__":
                    raise KeyboardInterrupt
                if ev == "__stop__":
                    current_path = None
                    send(session, args.ip, "/stop")
                    print("stop            ", end="\r")
                    continue
                if ev == "__honk__":
                    # bocina: tap suelto, no toca el comando de movimiento
                    send(session, args.ip, "/honk")
                    continue
                # comando de movimiento
                if ev != current_path:
                    current_path = ev
                    send(session, args.ip, ev)
                    print(f"{ev:<16}", end="\r")
                last_key_time = now

            # Se solto la tecla? (sin repeticiones por un rato) -> detener
            if current_path and (now - last_key_time) > KEY_HOLD_TIMEOUT_S:
                current_path = None
                send(session, args.ip, "/stop")
                print("stop            ", end="\r")

            # Heartbeat: reenvia el comando activo para el watchdog del ESP32
            if current_path and (now - last_beat) >= HEARTBEAT_S:
                send(session, args.ip, current_path)
                last_beat = now

            time.sleep(0.01)   # no quemar la CPU
    except KeyboardInterrupt:
        pass
    finally:
        # STOP siempre al salir, pase lo que pase
        send(session, args.ip, "/stop")
        print("\n[teleop] carrito detenido. Chau.")


if __name__ == "__main__":
    main()
