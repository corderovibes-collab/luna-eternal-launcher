# Luna Eternal — Launcher

Launcher oficial del servidor **PokeReport: Luna Eternal**. Instala el modpack,
trae su propio Java y mantiene todo al día sin que el jugador toque nada.

---

## Aviso: esto es una versión modificada

> **Este proyecto es un fork de [Freesm Launcher](https://github.com/FreesmTeam/FreesmLauncher)**,
> que a su vez es un fork de [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher),
> que lo es de PolyMC, que lo es de MultiMC.
>
> **No está avalado ni respaldado por ninguno de ellos**, y no tenemos relación
> con sus equipos. Cualquier fallo que encuentres aquí es nuestro, no suyo:
> repórtalo en **este** repositorio y no en los suyos.

El código se ha modificado respecto al original. Los cambios están en el
historial de git, y el punto de partida exacto es el commit `5114de67a`
(Freesm Launcher 2.2.2).

## Licencia

**GPL-3.0-only**, igual que el original. El texto completo está en
[LICENSE](LICENSE).

Los avisos de copyright de quienes vinieron antes **se conservan íntegros** —
tanto en las cabeceras de cada fichero como en la pantalla de «Acerca de»— y el
nuestro se añade encima:

```
© 2026 PokeReport: Luna Eternal
© 2024-2026 Freesm Launcher Contributors
© 2022-2026 Prism Launcher Contributors
© 2021-2022 PolyMC Contributors
© 2012-2021 MultiMC Contributors
```

### Sobre la marca y el arte

**El arte de Freesm y de Prism no se hereda: se ha sustituido.** Sus logotipos
llevan CC BY-SA 4.0 y acreditan a sus autores por nombre, así que renombrar el
fichero no los haría nuestros. Prism además pide expresamente a sus forks que no
usen su marca — y Freesm ya cumplió eso al forkear.

Los iconos de este launcher se generan del arte propio del servidor:

```bash
python program_info/genicons-luna.py
```

> ⚠️ **Queda uno sin sustituir:** `program_info/LunaEternal.icon` es el bundle de
> icono de macOS 26 y todavía contiene el arte de Freesm. Solo se usa al compilar
> para macOS, que hoy no se hace. **Hay que rehacerlo antes de publicar un
> binario de Mac.**

## Compilar

Windows, con el toolchain que prepara el proyecto principal:

```powershell
powershell tools/build-launcher.ps1     # desde d:\pokereportversionmejorada
```

Necesita Visual Studio Build Tools con la carga de C++, Qt 6.10.2
(`qtimageformats`, `qtnetworkauth`) y vcpkg. El script los busca en
`.toolchain/`, no en el sistema.

## Lo que todavía apunta fuera

| | |
|---|---|
| `Launcher_META_URL` | Sigue en `meta.freesmlauncher.org`. De ahí salen los metadatos de Minecraft y Fabric: **cada jugador depende de un servidor que no controlamos.** Prism sirve los mismos 13 paquetes que usamos; los 3 de diferencia son inyección de auth alternativa que no usamos. Lo que toca algún día es levantar el nuestro — el generador es libre |
| `Launcher_ELYBY_CLIENT_ID` | Registro de Ely.by de Freesm. No lo usamos: vamos con cuentas offline contra EasyAuth |

## Traer arreglos del original

El remoto original se conserva como `upstream`, que es lo que permite traerse
sus correcciones de seguridad sin rehacer el fork:

```bash
git fetch upstream
git merge upstream/develop
```

Los conflictos saldrán casi siempre en `program_info/CMakeLists.txt` y en el
`CMakeLists.txt` raíz, que es **exactamente donde se quiere decidir a mano**: son
los dos ficheros donde vive la marca.
