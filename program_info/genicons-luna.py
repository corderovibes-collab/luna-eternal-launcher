#!/usr/bin/env python3
"""Genera TODOS los iconos del launcher a partir de una sola imagen.

    python program_info/genicons-luna.py

Fuente: `program_info/LunaEternal.source.png` (1024x1024 RGBA).

POR QUE ESTO EXISTE Y NO SE REUTILIZA EL ARTE DE FREESM

Los logos que traia el fork son suyos: llevan CC BY-SA 4.0 en los metadatos y
acreditan a sus autores por nombre. **Renombrar el fichero no los hace
nuestros** -- es la misma trampa que descarto CobbleVerse (D-006) y la que
obligo a repartir los shaders por su canal oficial (D-030). Ademas Prism pide
expresamente a sus forks que no usen su marca, y usar el logo de otro como
identidad propia es un problema aparte del de la licencia.

Asi que el arte se sustituye, no se hereda.
"""
from pathlib import Path
import base64
from PIL import Image

AQUI = Path(__file__).resolve().parent
FUENTE = AQUI / "LunaEternal.source.png"
APPID = "net.pokereport.LunaEternal"
BINARIO = "lunaeternal"

original = Image.open(FUENTE).convert("RGBA")

# --- Recortar el margen transparente ---------------------------------------
# El arte suele venir con aire alrededor. Sin recortarlo, el icono se ve
# pequeño dentro de su casilla -- y en la barra de tareas, donde son 16
# pixeles, esa diferencia se nota mucho.
caja = original.getbbox()
recortada = original.crop(caja) if caja else original

# --- Cuadrar sin deformar ---------------------------------------------------
# ⚠ NADA DE REDIMENSIONAR A LA FUERZA. Estirar un logo de 1258x1296 a un
#   cuadrado lo deja aplastado, y en un icono pequeño se percibe como "algo no
#   encaja" aunque no se sepa decir por que. Se centra en un lienzo cuadrado
#   transparente del lado mayor.
lado = max(recortada.size)
lienzo = Image.new("RGBA", (lado, lado), (0, 0, 0, 0))
lienzo.paste(recortada, ((lado - recortada.size[0]) // 2, (lado - recortada.size[1]) // 2))

src = lienzo.resize((1024, 1024), Image.LANCZOS)
print(f"  fuente {original.size} -> recortada {recortada.size} -> 1024x1024")

# --- Windows: un solo .ico con todas las resoluciones dentro --------------
# Sin las pequeñas, Windows escala la de 256 para la barra de tareas y sale
# emborronada justo en el sitio donde mas se mira.
ico = AQUI / f"{BINARIO}.ico"
src.save(ico, format="ICO", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])

# --- Linux: PNG de 256 ----------------------------------------------------
png = AQUI / f"{APPID}_256.png"
src.resize((256, 256), Image.LANCZOS).save(png, format="PNG")

# --- macOS ----------------------------------------------------------------
icns = AQUI / f"{BINARIO}.icns"
src.save(icns, format="ICNS")

# --- El SVG que se incrusta en el recurso de Qt ---------------------------
#
# El arte de partida es un PNG, asi que aqui NO hay vectorizacion: se envuelve
# tal cual en un SVG con la imagen como data URI. Es honesto --no finge ser un
# vector-- y Qt lo pinta igual de bien en cualquier tamaño de los que usa el
# launcher.
b64 = base64.b64encode(FUENTE.read_bytes()).decode("ascii")
(AQUI / f"{APPID}.svg").write_text(
    '<?xml version="1.0" encoding="UTF-8"?>\n'
    '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"\n'
    '     width="1024" height="1024" viewBox="0 0 1024 1024">\n'
    '  <title>Luna Eternal</title>\n'
    f'  <image width="1024" height="1024" xlink:href="data:image/png;base64,{b64}"/>\n'
    '</svg>\n', encoding="utf-8")

for f in (ico, png, icns, AQUI / f"{APPID}.svg"):
    print(f"  {f.name:44} {f.stat().st_size // 1024:6} KB")
