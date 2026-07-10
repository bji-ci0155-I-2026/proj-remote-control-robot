#!/usr/bin/env python3
"""
Rung 3 - seguimiento de objeto por color (vision clasica, sin LLM).

Cierra el lazo completo frames -> decision -> comando, usando OpenCV para
detectar un blob de color y mantenerlo centrado. Es el System 2 mas simple
posible: rapido, barato y que NO puede alucinar. Si todo lo demas se cae,
esta demo sigue siendo un demo de vision-IA completo y legitimo.

    camara (webcam o DroidCam)  ->  deteccion de color (HSV)
        ->  politica look-think-act  ->  comando HTTP al carrito

El freno ultrasonico local (System 1) sigue mandando por encima: aunque la
vision diga "adelante", si hay un obstaculo < 15 cm el carrito no avanza.

Politica de control (bang-bang, robusta):
    - objeto perdido        -> stop (tras unos frames en blanco)
    - objeto a la izquierda -> girar izquierda
    - objeto a la derecha   -> girar derecha
    - centrado y lejos      -> avanzar (blob chico = objeto lejos)
    - centrado y cerca      -> stop (blob grande = objeto cerca)

Uso:
    # 1) probar la VISION sola con la webcam, sin mover el carrito:
    python follow.py --source 0 --no-drive

    # 2) calibrar el color bajo tu luz (trackbars HSV; imprime los valores):
    python follow.py --source 0 --calibrate

    # 3) manejar de verdad con DroidCam montada en el carrito:
    python follow.py --source http://192.168.0.105:4747/video --ip 192.168.100.169

Requiere:  pip install opencv-python numpy requests
"""

import argparse
import sys
import threading
import time

import cv2
import numpy as np
import requests

CAR_IP = "192.168.0.169"   # IP fija del carrito (debe coincidir con el sketch)
TIMEOUT = 0.5               # s por request; corto para no trabar el lazo de vision

# Cada cuanto reenviar el comando activo para alimentar el watchdog del ESP32
# (COMMAND_TIMEOUT_MS = 600 ms). 150 ms deja margen aunque un frame tarde.
HEARTBEAT_S = 0.15

# --- Parametros de deteccion / control (ajustables) ---
MIN_AREA_FRAC = 0.004    # area minima del blob (fraccion del frame) para creerle
DEADZONE_FRAC = 0.15     # zona muerta central: |error| < esto => "centrado"
NEAR_AREA_FRAC = 0.18    # blob mas grande que esto => objeto cerca => stop
LOST_FRAMES = 5          # frames seguidos sin blob antes de declarar "perdido"

# Rangos HSV por color. El rojo cruza el 0 de H, por eso lleva dos rangos.
# Ajusta con --calibrate si tu iluminacion es distinta.
COLOR_RANGES = {
    "red": [((0, 120, 70), (10, 255, 255)), ((170, 120, 70), (180, 255, 255))],
    "green": [((36, 80, 70), (85, 255, 255))],
    "blue": [((90, 80, 70), (130, 255, 255))],
}


def open_source(source):
    """Abre webcam (indice) o stream (URL). Devuelve un VideoCapture listo."""
    if source.isdigit():
        # CAP_DSHOW: backend mas confiable para webcams en Windows
        cap = cv2.VideoCapture(int(source), cv2.CAP_DSHOW)
    else:
        cap = cv2.VideoCapture(source)
    # Buffer chico = menos latencia (queremos el frame mas reciente, no el mas viejo)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    return cap


def make_mask(hsv, color):
    """Mascara binaria del color pedido, con limpieza morfologica."""
    mask = None
    for lo, hi in COLOR_RANGES[color]:
        part = cv2.inRange(hsv, np.array(lo), np.array(hi))
        mask = part if mask is None else cv2.bitwise_or(mask, part)
    # abrir (quita ruido) y cerrar (rellena huecos) el blob
    kernel = np.ones((5, 5), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    return mask


def find_blob(mask):
    """Devuelve (cx, cy, area) del contorno mas grande, o None si no hay."""
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return None
    c = max(contours, key=cv2.contourArea)
    area = cv2.contourArea(c)
    M = cv2.moments(c)
    if M["m00"] == 0:
        return None
    cx = int(M["m10"] / M["m00"])
    cy = int(M["m01"] / M["m00"])
    return cx, cy, area


def decide(blob, frame_w, frame_h, lost_count):
    """Traduce la deteccion a un comando del carrito. Devuelve (path, etiqueta)."""
    frame_area = frame_w * frame_h
    if blob is None:
        # perdido: solo frenamos si lleva varios frames sin verse (evita parpadeos)
        if lost_count >= LOST_FRAMES:
            return "/stop", "perdido"
        return None, "buscando"

    cx, _, area = blob
    if area < MIN_AREA_FRAC * frame_area:
        return None, "muy chico"

    error = (cx - frame_w / 2) / (frame_w / 2)   # -1 (izq) .. +1 (der)

    if error < -DEADZONE_FRAC:
        return "/left", "izquierda"
    if error > DEADZONE_FRAC:
        return "/right", "derecha"
    # centrado: decidir por tamano (proxy de distancia)
    if area > NEAR_AREA_FRAC * frame_area:
        return "/stop", "cerca -> stop"
    return "/forward", "centrado -> adelante"


def draw_hud(frame, blob, label, path, driving, fps=0.0):
    """Dibuja el overlay de depuracion sobre el frame."""
    h, w = frame.shape[:2]
    cxm = w // 2
    dz = int(DEADZONE_FRAC * (w / 2))
    # zona muerta central
    cv2.line(frame, (cxm, 0), (cxm, h), (80, 80, 80), 1)
    cv2.rectangle(frame, (cxm - dz, 0), (cxm + dz, h), (60, 60, 60), 1)
    if blob is not None:
        cx, cy, area = blob
        cv2.circle(frame, (cx, cy), 8, (0, 255, 255), -1)
        cv2.line(frame, (cxm, cy), (cx, cy), (0, 255, 255), 2)
    mode = "DRIVE" if driving else "VISION-ONLY"
    txt = f"[{mode}] {label}  ->  {path or '-'}   {fps:4.0f} fps"
    cv2.putText(frame, txt, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7,
                (0, 0, 0), 4, cv2.LINE_AA)
    cv2.putText(frame, txt, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7,
                (0, 255, 0), 1, cv2.LINE_AA)


def send(session, ip, path):
    try:
        session.get(f"http://{ip}{path}", timeout=TIMEOUT)
        return True
    except requests.exceptions.RequestException:
        return False


# =========================================================
# Hilos para sacar el I/O bloqueante del lazo de vision.
# El problema: cap.read() sobre un stream MJPEG y requests.get() bloquean;
# si corren en el lazo principal, la vision se traba entre frames.
# Solucion: la camara y los comandos corren en hilos aparte, y el lazo
# principal solo toma el ultimo frame disponible y fija el ultimo comando.
# =========================================================
class FrameGrabber:
    """Lee la camara en un hilo y guarda solo el frame mas reciente.

    Asi se vacia el buffer del stream (el backend FFmpeg suele ignorar
    BUFFERSIZE=1) y el lazo de vision nunca espera a la red.
    """

    def __init__(self, cap, flip=False):
        self.cap = cap
        self.flip = flip
        self.lock = threading.Lock()
        self.frame = None
        self.ok = True
        self.stopped = False
        self.thread = threading.Thread(target=self._loop, daemon=True)

    def start(self):
        self.thread.start()
        return self

    def _loop(self):
        while not self.stopped:
            ok, frame = self.cap.read()
            if not ok:
                self.ok = False
                break
            if self.flip:
                frame = cv2.flip(frame, -1)
            with self.lock:
                self.frame = frame

    def read(self):
        """Devuelve (frame_o_None, ok). El frame es una copia segura."""
        with self.lock:
            frame = None if self.frame is None else self.frame.copy()
        return frame, self.ok

    def stop(self):
        self.stopped = True
        self.thread.join(timeout=1.0)
        self.cap.release()


class CommandSender:
    """Manda comandos al carrito desde un hilo propio.

    El lazo de vision solo llama set(path); este hilo reenvia el comando
    activo cada HEARTBEAT_S (para el watchdog del ESP32) y al instante cuando
    cambia. Ningun request bloquea la vision.
    """

    def __init__(self, ip):
        self.ip = ip
        self.session = requests.Session()
        self.lock = threading.Lock()
        self.desired = None       # path deseado o None
        self.last_path = None
        self.stopped = False
        self.thread = threading.Thread(target=self._loop, daemon=True)

    def start(self):
        self.thread.start()
        return self

    def set(self, path):
        with self.lock:
            self.desired = path

    def _loop(self):
        last_beat = 0.0
        while not self.stopped:
            with self.lock:
                path = self.desired
            now = time.time()
            if path is not None and (path != self.last_path or
                                     (now - last_beat) >= HEARTBEAT_S):
                send(self.session, self.ip, path)
                self.last_path = path
                last_beat = now
            time.sleep(0.02)

    def stop(self):
        self.stopped = True
        self.thread.join(timeout=1.0)
        send(self.session, self.ip, "/stop")   # STOP siempre al terminar


def run_calibrate(cap, color):
    """Modo calibracion: trackbars HSV en vivo; imprime el rango al salir."""
    win = "calibrate (ESC para salir)"
    cv2.namedWindow(win)
    # arranca desde el primer rango del color elegido
    lo, hi = COLOR_RANGES[color][0]
    names = ["Hmin", "Smin", "Vmin", "Hmax", "Smax", "Vmax"]
    init = [lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]]
    maxes = [180, 255, 255, 180, 255, 255]
    for n, v, m in zip(names, init, maxes):
        cv2.createTrackbar(n, win, v, m, lambda x: None)

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        vals = [cv2.getTrackbarPos(n, win) for n in names]
        mask = cv2.inRange(hsv, np.array(vals[:3]), np.array(vals[3:]))
        res = cv2.bitwise_and(frame, frame, mask=mask)
        cv2.imshow(win, np.hstack([frame, res]))
        if cv2.waitKey(1) & 0xFF == 27:  # ESC
            print("\n[calibrate] copia esto a COLOR_RANGES:")
            print(f'  "{color}": [(({vals[0]}, {vals[1]}, {vals[2]}), '
                  f'({vals[3]}, {vals[4]}, {vals[5]}))],')
            break
    cv2.destroyAllWindows()


def main():
    ap = argparse.ArgumentParser(description="Rung 3 - seguir objeto por color")
    ap.add_argument("--source", default="0",
                    help="0 = webcam, o URL de DroidCam (http://IP:4747/video)")
    ap.add_argument("--ip", default=CAR_IP, help="IP del carrito")
    ap.add_argument("--color", default="red", choices=list(COLOR_RANGES),
                    help="color a seguir")
    ap.add_argument("--no-drive", action="store_true",
                    help="solo ver la vision, no mandar comandos al carrito")
    ap.add_argument("--calibrate", action="store_true",
                    help="ajustar el rango HSV con trackbars y salir")
    ap.add_argument("--flip", action="store_true",
                    help="voltear la imagen 180 (si la camara va montada al reves)")
    args = ap.parse_args()

    cap = open_source(args.source)
    if not cap.isOpened():
        print(f"[ERROR] no se pudo abrir la fuente de video: {args.source}")
        print("  - webcam: probaste --source 0 (o 1)?")
        print("  - DroidCam: la URL es http://IP_DEL_TELEFONO:4747/video ?")
        print("  - el telefono esta en la misma red (router)?")
        sys.exit(1)

    if args.calibrate:
        run_calibrate(cap, args.color)
        cap.release()
        return

    driving = not args.no_drive

    if driving:
        print(f"[follow] verificando carrito en http://{args.ip} ...")
        if not send(requests.Session(), args.ip, "/status"):
            print("[ERROR] no se pudo alcanzar el carrito. Usa --no-drive para "
                  "probar solo la vision, o revisa la IP/red.")
            sys.exit(1)

    print(f"[follow] siguiendo '{args.color}'. "
          f"{'MANEJANDO' if driving else 'VISION-ONLY'}. ESC/q en la ventana para salir.\n")

    # La camara y los comandos corren en hilos aparte; el lazo no bloquea.
    grabber = FrameGrabber(cap, flip=args.flip).start()
    sender = CommandSender(args.ip).start() if driving else None

    lost_count = 0
    fps = 0.0
    last_t = time.time()

    try:
        while True:
            frame, ok = grabber.read()
            if not ok:
                print("[follow] se corto el video (camara/red). Frenando.")
                break
            if frame is None:
                # todavia no llego el primer frame del hilo de la camara
                if cv2.waitKey(10) & 0xFF in (27, ord("q")):
                    break
                continue

            h, w = frame.shape[:2]
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            mask = make_mask(hsv, args.color)
            blob = find_blob(mask)
            lost_count = lost_count + 1 if blob is None else 0

            path, label = decide(blob, w, h, lost_count)
            if sender is not None and path is not None:
                sender.set(path)

            now = time.time()
            fps = 0.9 * fps + 0.1 * (1.0 / max(now - last_t, 1e-3))
            last_t = now

            draw_hud(frame, blob, label, path, driving, fps)
            cv2.imshow("follow (ESC/q para salir)", frame)
            if cv2.waitKey(1) & 0xFF in (27, ord("q")):
                break
    except KeyboardInterrupt:
        pass
    finally:
        grabber.stop()
        if sender is not None:
            sender.stop()          # manda /stop al terminar
        cv2.destroyAllWindows()
        print("\n[follow] fin. Carrito detenido.")


if __name__ == "__main__":
    main()
