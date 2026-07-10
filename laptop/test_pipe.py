#!/usr/bin/env python3
"""
Rung 0 - prueba del tubo laptop -> carrito.

Verifica que la laptop puede alcanzar al carrito en su IP fija y mandarle
comandos por HTTP. NO es todavia System 2 (no hay camara ni IA); solo confirma
que el canal de comandos funciona de punta a punta.

Uso:
    python test_pipe.py                # secuencia de prueba (con avance breve)
    python test_pipe.py --no-move      # solo lee /status, no mueve los motores
    python test_pipe.py --ip 10.1.2.3  # apunta a otra IP

Requiere:  pip install requests
"""

import argparse
import sys
import time

import requests

CAR_IP = "192.168.0.169"   # IP fija del carrito (debe coincidir con el sketch)
TIMEOUT = 3.0             # segundos por request; la red del hotspot puede tardar


def cmd(ip, path):
    """Manda un comando/consulta al carrito y devuelve el texto de respuesta."""
    url = f"http://{ip}{path}"
    t0 = time.time()
    r = requests.get(url, timeout=TIMEOUT)
    ms = (time.time() - t0) * 1000
    print(f"  GET {path:<10} -> {r.status_code} '{r.text.strip()}'  ({ms:.0f} ms)")
    r.raise_for_status()
    return r.text.strip()


def main():
    ap = argparse.ArgumentParser(description="Prueba del tubo laptop->carrito")
    ap.add_argument("--ip", default=CAR_IP, help="IP del carrito")
    ap.add_argument("--no-move", action="store_true",
                    help="no mover motores, solo leer distancia")
    args = ap.parse_args()

    print(f"[test] carrito en http://{args.ip}\n")

    # 1) Alcanzabilidad + lectura del sensor (System 1 sigue midiendo distancia)
    try:
        print("[1] leyendo /status (distancia en cm; 999 = libre)")
        for _ in range(3):
            cmd(args.ip, "/status")
            time.sleep(0.3)
    except requests.exceptions.RequestException as e:
        print(f"\n[ERROR] no se pudo alcanzar el carrito: {e}")
        print("  - la laptop esta en el hotspot POCO?")
        print("  - la IP coincide con la que imprime el serial?")
        print("  - el hotspot no se reinicio (subred cambia)?")
        sys.exit(1)

    if args.no_move:
        print("\n[ok] canal de comandos vivo (modo --no-move, sin mover).")
        return

    # 2) Camino de comandos: avanzar un instante y frenar.
    #    El freno ultrasonico local (System 1) sigue mandando: si hay un
    #    obstaculo < 15 cm al frente, el carrito NO avanzara aunque le llegue
    #    /forward. Eso es lo correcto.
    print("\n[2] secuencia de movimiento (avance breve -> stop)")
    print("    *** el carrito se va a mover ~0.6 s; tenelo en el piso, despejado ***")
    try:
        cmd(args.ip, "/forward")
        time.sleep(0.6)
    finally:
        # stop SIEMPRE, aunque algo falle a mitad de camino
        cmd(args.ip, "/stop")

    print("\n[ok] tubo laptop->carrito verificado de punta a punta.")


if __name__ == "__main__":
    main()
