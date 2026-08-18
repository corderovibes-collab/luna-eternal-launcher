# Luna Eternal — información del programa

Aquí vive **toda la marca** del launcher:

- Nombre, identificador y logotipo
- URL y puntos finales de API
- Fichero `.desktop`, metainfo de tiendas de Linux y tipos MIME
- Iconos de Windows, macOS y Linux

## Los dos ficheros que importan

| | |
|---|---|
| `CMakeLists.txt` (este directorio) | Nombre visible, AppID, dominio, copyright, autores |
| `../CMakeLists.txt` (raíz) | Nombre del binario y todas las URL externas |

**Son los dos sitios donde chocará un `git merge upstream/develop`**, y es
justo lo que se quiere: la marca no debe fusionarse sola.

## Iconos

Todos salen de una sola imagen, `LunaEternal.source.png` (1024×1024 RGBA):

```bash
python program_info/genicons-luna.py
```

Genera el `.ico` de Windows con las siete resoluciones dentro, el `.icns` de
macOS, el PNG de 256 para Linux y el SVG que se incrusta en el recurso de Qt.

> ⚠️ **El arte de Freesm y de Prism no se reutiliza.** Sus logotipos llevan
> CC BY-SA 4.0 y acreditan a sus autores por nombre, así que renombrar el
> fichero no los haría nuestros. Se sustituyeron; ver el README de la raíz.

> ⚠️ **`LunaEternal.icon` es la excepción pendiente.** Es el bundle de icono de
> macOS 26 y todavía contiene arte de Freesm. Solo se usa al compilar para Mac,
> que hoy no se hace — **hay que rehacerlo antes de publicar un binario de Mac**.
