# PokeReport Linux y macOS

Version 0.2.4, mismo launcher y pack de Jugador/Constructor. Windows no se sustituye. Workflow pokereport-unix.yml permite construir todos o Linux/macOS por separado en GitHub Actions. Mantiene Java downloader y la política de archivos once: ajustes existentes no se reemplazan al actualizar o reparar.

## Linux x86_64

AppImage compilado en Ubuntu 24.04; requiere distribución con glibc 2.39 o superior. Descargar luna-launcher-linux-x64-0.2.4.AppImage, marcar como ejecutable y abrir. Si faltara FUSE, el sistema puede ejecutar AppImage con --appimage-extract-and-run. No se promete compatibilidad con Ubuntu 22.04.

## macOS

Paquetes separados Intel x86_64 y Apple Silicon arm64. Extraer el ZIP y copiar LunaEternal.app a Aplicaciones. Compilación y dependencias verificadas en macOS 15; sistemas anteriores necesitan validación. Firma ad-hoc; no hay certificado Developer ID ni notarización configurada, por lo que Gatekeeper puede requerir autorización manual en Privacidad y seguridad. No se desactiva Gatekeeper.

## Staff privado

PokeReport-Staff-0.2.4-<plataforma>.zip incluye el launcher completo y acceso PokeReport-Staff.sh en Linux / PokeReport-Staff.command en macOS. Extraer conservando todo junto y abrir el acceso. Activación mediante POKEREPORT_STAFF_TOOLS=1 solo para ese proceso; no modifica variables globales. Cerrar otro launcher antes de usar Staff. Los roles de Minecraft se conceden por separado: el paquete no da OP ni permisos de construcción.

Solo paquetes de Jugador se publican como descargas públicas. Staff se entrega como archivo privado al propietario, fuera de la web/release pública.

## Validación

CI ejecuta compilación, CTest, empaquetado y --version del binario distribuido. SHA256SUMS acompaña los paquetes. FFmpeg oficial instalado incluye archivos Linux, macOS y ARM64; no se distribuyen DLL de Windows como solución para estos sistemas. Audio real, vídeos, mods de construcción, login y rendimiento de Minecraft requieren QA en equipos reales; una compilación no certifica esas pruebas.

Builds verificados: macOS run 37175712451 (Intel y ARM64), Linux run 37176410162; 29 suites PASS por plataforma y --version del paquete PASS. Descargas de Jugador publicadas en https://pokereport.online/jugar/. Staff permanece privado.
