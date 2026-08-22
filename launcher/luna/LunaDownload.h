// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
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
 *
 */

#pragma once

#include <QString>

#include "luna/LunaManifest.h"
#include "tasks/Task.h"

namespace Luna {

/**
 * Ruta absoluta de un fichero del pack dentro de la instancia.
 *
 * ⚠ Devuelve VACIO si la ruta no es segura. No lanza ni corrige: quien llame
 *   tiene que comprobarlo. Es la tercera cerradura contra `../` --las otras dos
 *   son la huella del manifiesto y el filtrado del planificador-- y esta es la
 *   ultima antes de tocar el disco de verdad.
 */
QString resolveInInstance(const QString& instanceRoot, const QString& relPath);

/** Intentos por origen antes de darse por vencido con un fichero. */
constexpr int kIntentosPorOrigen = 4;

/**
 * Ese codigo HTTP descarta el origen para siempre, o merece otro intento?
 *
 * Vive en la cabecera para poder PROBARLA. Es una tabla de cuatro casos, ninguno
 * obvio, y equivocarse en cualquiera reconstruye el fallo que `makeFileTask`
 * existe para arreglar:
 *
 *   - dar un 503 por definitivo  = volver a rendirse a la primera
 *   - reintentar un 404 cuatro veces = hacer esperar al jugador de balde para
 *     darle al final la misma mala noticia
 *
 * `0` significa que no hubo respuesta HTTP siquiera --DNS, TLS, conexion
 * cortada-- y eso es exactamente lo que hay que reintentar.
 */
bool origenDescartado(int http);

/**
 * Tarea que trae UN fichero: prueba sus origenes EN ORDEN y REINTENTA cada uno
 * con espera creciente. `nullptr` si el fichero no trae origen o su ruta no es
 * segura.
 *
 * POR QUE VARIOS ORIGENES
 *
 * Antes cada entrada tenia una sola URL: si ese origen caia, caia el pack
 * entero y no habia nada que hacer salvo esperar. Un 4xx tiene que ser
 * definitivo para ESE ORIGEN, no para el fichero.
 *
 * ⚠⚠ POR QUE REINTENTA, Y POR QUE ESTO NO ES `MultipleOptionsTask`
 *
 * Esto se escribio con `MultipleOptionsTask`, que prueba las opciones en
 * secuencia hasta que una funciona. Parecia suficiente y NO LO ERA, por una
 * razon que solo se ve con el manifiesto delante: **157 de los 159 ficheros
 * tienen UN SOLO origen**. Para casi todo el pack, "varias opciones" era una
 * sola, o sea CERO REINTENTOS.
 *
 * Y `Net::Download` tampoco reintenta por su cuenta: su `AutoRetry` hay que
 * encenderlo a mano (`makeFile` no lo hace) y ademas solo cubre el HTTP 429.
 * Un corte de TLS, un DNS que tarda, un 503 del CDN o una descarga que se
 * queda parada mas de `RequestTimeout` mataban el fichero al primer tropiezo.
 *
 * La cuenta que lo convierte en un problema de producto: con 159 ficheros,
 * un 2 % de fallo por fichero --normal en un wifi domestico-- hace que la
 * instalacion COMPLETA falle el 96 % de las veces. El jugador ve
 * "Multiples subtareas fallidas" y se va. Es exactamente la leccion que el
 * launcher de Electron ya habia aprendido (4 reintentos -> 8, tope 8 s -> 30 s)
 * y que este fork no habia heredado.
 *
 * ⚠ CADA ORIGEN LLEVA SU PROPIO VALIDADOR DE HUELLA. Si un espejo sirve un
 *   fichero corrupto, ese intento falla y se pasa al siguiente en vez de dar
 *   la descarga por buena -- que es justo lo que se quiere de un espejo en el
 *   que no confiamos tanto como en el primario.
 *
 * ⚠ EL MOTIVO DEL FALLO LLEGA CON NOMBRE Y APELLIDOS. `MultipleOptionsTask`
 *   decia "All attempts have failed!" y tiraba el error de debajo, asi que el
 *   dialogo no nombraba ni el fichero ni la causa: hubo que reconstruirla
 *   leyendo el codigo. Ahora el mensaje dice que fichero, que servidor y que
 *   contesto.
 */
Task::Ptr makeFileTask(const File& file, const QString& destPath, int intentosPorOrigen = kIntentosPorOrigen);

/**
 * Cuantos origenes utiles tiene una entrada, ya filtrados.
 *
 * Existe aparte para poder comprobarlo sin construir tareas ni tocar la red.
 */
QStringList usableOrigins(const File& file);

}  // namespace Luna
