# De dónde salen estos ficheros

**Ninguno se dibuja a mano.** Los tres se derivan del arte de la marca, que vive
en `program_info/`:

| Origen | Qué es |
|---|---|
| `program_info/LunaEternal.source.png` | La Poké Ball en llamas (1258×1296). Es también el origen del `.ico` y del `.icns` |
| `program_info/PokeReportNetwork.wordmark.source.png` | El rótulo POKEREPORT NETWORK (1180×364) |

## Los derivados

| Fichero | Qué es | Cómo se hace |
|---|---|---|
| `letras.png` · `letras@2x.png` | El rótulo de la barra principal, a 34 y 68 px de alto | redimensionar por ALTURA con Lanczos |
| `../backgrounds/pokereport.png` | El fondo de la ventana: ball arriba, rótulo abajo, lienzo 900×820 transparente, **alfa al 40 %** | componer y bajar el alfa |

```python
from PIL import Image
ball   = Image.open("program_info/LunaEternal.source.png").convert("RGBA")
letras = Image.open("program_info/PokeReportNetwork.wordmark.source.png").convert("RGBA")

# el rótulo de la barra, x1 y x2
for alto, nombre in [(34, "letras.png"), (68, "letras@2x.png")]:
    ancho = round(letras.width * alto / letras.height)
    letras.resize((ancho, alto), Image.LANCZOS).save(nombre, optimize=True)

# el fondo
W, H = 900, 820
fondo = Image.new("RGBA", (W, H), (0, 0, 0, 0))
b = ball.resize((500, round(ball.height * 500 / ball.width)), Image.LANCZOS)
fondo.paste(b, ((W - 500) // 2, 40), b)
l = letras.resize((380, round(letras.height * 380 / letras.width)), Image.LANCZOS)
fondo.paste(l, ((W - 380) // 2, H - l.height - 90), l)
fondo.putalpha(fondo.getchannel("A").point(lambda v: int(v * 0.40)))
fondo.quantize(colors=255, method=Image.FASTOCTREE).save("pokereport.png", optimize=True)
```

## Tres cosas que no son obvias

⚠️ **El rótulo se escala por ALTURA, no por anchura.** Va dentro de una barra de
herramientas: lo que tiene que encajar es el alto.

⚠️ **`letras@2x.png` no es un lujo.** `MainWindow` lo carga y le pone
`setDevicePixelRatio(2)`. En una pantalla al 150 % Qt dibuja el widget a 1,5×, y
un rótulo guardado al tamaño exacto se vería pastoso; escalar hacia **abajo**
desde el doble sale limpio en cualquier escala. Y sin el `devicePixelRatio` el
rótulo saldría del **doble** de grande, porque Qt trataría los 68 px como 68
puntos de interfaz.

⚠️ **El fondo lleva margen abajo a propósito.** `CatPainter` lo ancla **abajo a
la derecha**, así que lo que esté pegado al borde del lienzo queda pegado al
borde de la ventana. Con 20 px de margen el rótulo salía **cortado**; son 90.
