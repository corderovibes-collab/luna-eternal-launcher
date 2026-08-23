// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PokeReport Network - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport Network
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "luna/LunaDiagnostico.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QRegularExpression>

namespace Luna {
namespace {

struct Regla {
    const char* patron;
    const char* titulo;
    const char* detalle;
    Accion accion;
};

// ⚠⚠ CADA FILA ES UN FALLO REAL. El orden importa: se devuelve la primera que
//    case, asi que lo especifico va antes que lo general.
const Regla REGLAS[] = {
    { R"(invalid LOC header|Unexpected end of ZLIB|zip END header not found|Invalid CEN header)",
      QT_TRANSLATE_NOOP("Luna", "Hay un fichero del pack corrupto"),
      QT_TRANSLATE_NOOP("Luna",
                        "Una descarga se corto a medias y el jar quedo roto. El tamaño puede parecer correcto, asi que no se nota "
                        "hasta que el juego intenta abrirlo."),
      Accion::Reparar },

    { R"(OutOfMemoryError|Could not reserve enough space for object heap)",
      QT_TRANSLATE_NOOP("Luna", "El juego se quedo sin memoria"),
      QT_TRANSLATE_NOOP("Luna",
                        "Sube la RAM asignada en Ajustes. Si ya esta al maximo que permite tu equipo, cierra el navegador antes de "
                        "jugar: suele ser lo que se la come."),
      Accion::Memoria },

    { R"(Incompatible mods found|requires version .* of mod|Mod resolution (failed|encountered an incompatible))",
      QT_TRANSLATE_NOOP("Luna", "Las versiones de los mods no encajan"),
      QT_TRANSLATE_NOOP("Luna",
                        "Tu instalacion quedo a medio actualizar. Repararla vuelve a dejarla exactamente igual que la del servidor."),
      Accion::Reparar },

    { R"(UnsupportedClassVersionError|class file version \d+\.\d+)",
      QT_TRANSLATE_NOOP("Luna", "Java no es el que toca"),
      QT_TRANSLATE_NOOP("Luna",
                        "El launcher instala su propio Java y no usa el del sistema, asi que esto suele significar que su copia quedo "
                        "a medias. Repara y vuelve a probar."),
      Accion::Reparar },

    { R"(Mixin apply failed|MixinApplyError|MixinTransformerError)",
      QT_TRANSLATE_NOOP("Luna", "Dos mods se pisan entre ellos"),
      QT_TRANSLATE_NOOP("Luna",
                        "Casi siempre es un mod añadido a mano en la carpeta del juego. Repara para dejar solo los del servidor."),
      Accion::Reparar },

    // ⚠⚠ AQUI HABIA `OpenGL 3\.2` A SECAS Y SALTABA EN CADA ARRANQUE CORRECTO.
    //
    //    Minecraft escribe una linea de EXITO que lo contiene:
    //      GPU: Intel(R) UHD Graphics 620 (Supports OpenGL 3.2.0 - Build 31.0…)
    //
    //    Un diagnostico que se equivoca siempre es PEOR que no tenerlo: enseña
    //    a la gente a ignorar los avisos, y el dia que haya uno de verdad
    //    tampoco lo van a leer. Ahora solo casan frases que unicamente aparecen
    //    al fallar.
    { R"(Failed to create window|GLFW error|Pixel format not accelerated|No OpenGL context|requires? OpenGL|OpenGL \d\.\d[^)]{0,24}(or (higher|later|newer)|required)|does ?n[o']?t support OpenGL)",
      QT_TRANSLATE_NOOP("Luna", "La tarjeta grafica no arranca el juego"),
      QT_TRANSLATE_NOOP("Luna",
                        "Actualiza el controlador de la grafica desde la web de NVIDIA, AMD o Intel. Minecraft 1.21 necesita OpenGL "
                        "3.2 o superior."),
      Accion::Ninguna },

    { R"(Connection refused|UnknownHostException|Failed to download|ETIMEDOUT|ENOTFOUND)",
      QT_TRANSLATE_NOOP("Luna", "Se corto la conexion durante la descarga"),
      QT_TRANSLATE_NOOP("Luna",
                        "Comprueba internet y dale otra vez. Lo ya descargado no se pierde: el launcher sigue por donde iba."),
      Accion::Ninguna },
};

/** Codigos de salida que significan algo concreto, no un error del juego. */
struct Codigo {
    int codigo;
    const char* titulo;
    const char* detalle;
    Accion accion;
};

// ⚠ ESTOS NO DEJAN NI UNA LINEA EN EL LOG, y por eso hay que mirarlos: el
//   proceso muere de golpe dentro del controlador de la grafica o del propio
//   Windows, sin que Java llegue a escribir nada.
const Codigo CODIGOS[] = {
    { -1073741819,  // 0xC0000005 · violacion de acceso
      QT_TRANSLATE_NOOP("Luna", "El juego se cerro de golpe (violacion de acceso)"),
      QT_TRANSLATE_NOOP("Luna",
                        "Casi siempre es el controlador de la grafica. Actualizalo; si acabas de hacerlo, reinicia el equipo antes de "
                        "volver a probar."),
      Accion::Ninguna },

    { -1073740791,  // 0xC0000409 · stack buffer overrun
      QT_TRANSLATE_NOOP("Luna", "El juego se cerro de golpe"),
      QT_TRANSLATE_NOOP("Luna",
                        "Suele ser memoria mal asignada o un mod añadido a mano. Repara la instalacion y, si sigue, baja un poco la "
                        "RAM en Ajustes."),
      Accion::Reparar },
};

}  // namespace

Diagnostico diagnosticar(int codigo, const QString& registro)
{
    // ⚠ EL REGISTRO MANDA SOBRE EL CODIGO. Un juego puede morir con codigo 1
    //   por cualquier motivo; si el log dice por que, eso es lo que se enseña.
    for (const auto& regla : REGLAS) {
        const QRegularExpression re(QString::fromLatin1(regla.patron), QRegularExpression::CaseInsensitiveOption);
        if (re.match(registro).hasMatch()) {
            return { QObject::tr(regla.titulo, "Luna"), QObject::tr(regla.detalle, "Luna"), regla.accion };
        }
    }

    if (codigo == 0) {
        return {};  // salida normal: no hay nada que contar
    }

    for (const auto& c : CODIGOS) {
        if (c.codigo == codigo) {
            return { QObject::tr(c.titulo, "Luna"), QObject::tr(c.detalle, "Luna"), c.accion };
        }
    }

    // ⚠ NO SE CALLA. Decir "no se lo que ha pasado, aqui esta el registro" es
    //   informacion; cerrar la ventana sin decir nada es lo que hacia antes, y
    //   es lo que convierte un fallo en un jugador que se va.
    return { QObject::tr("El juego se cerro de forma inesperada (codigo %1)", "Luna").arg(codigo),
             QObject::tr("No reconozco la causa. El registro completo esta en la carpeta de registros: abrela, y mandalo por Discord.",
                         "Luna"),
             Accion::Ninguna };
}

QString colaDelRegistro(const QString& gameRoot, int maxLineas)
{
    QFile log(QDir(gameRoot).filePath(QStringLiteral("logs/latest.log")));
    if (!log.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};  // normal: el juego pudo morir antes de escribir uno
    }

    // ⚠ SE LEE DEL FINAL. Un log de Minecraft son miles de lineas y la
    //   excepcion que mato al juego esta SIEMPRE al final. Se guarda una
    //   ventana de las ultimas `maxLineas` y se tira lo demas segun se lee, en
    //   vez de cargar el fichero entero en memoria.
    QStringList ventana;
    ventana.reserve(maxLineas);
    while (!log.atEnd()) {
        ventana.append(QString::fromUtf8(log.readLine()));
        if (ventana.size() > maxLineas) {
            ventana.removeFirst();
        }
    }
    return ventana.join(QString());
}

}  // namespace Luna
