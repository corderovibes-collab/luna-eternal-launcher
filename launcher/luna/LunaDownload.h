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

/**
 * Tarea que trae UN fichero, probando sus origenes EN ORDEN hasta que uno
 * conteste. `nullptr` si el fichero no trae origen o su ruta no es segura.
 *
 * POR QUE VARIOS ORIGENES
 *
 * Antes cada entrada tenia una sola URL: si ese origen caia, caia el pack
 * entero y no habia nada que hacer salvo esperar. Un 4xx tiene que ser
 * definitivo para ESE ORIGEN, no para el fichero.
 *
 * Se apoya en `MultipleOptionsTask`, que ejecuta subtareas en secuencia hasta
 * que una funciona. No hace falta escribir el failover a mano.
 *
 * ⚠ CADA ORIGEN LLEVA SU PROPIO VALIDADOR DE HUELLA. Si un espejo sirve un
 *   fichero corrupto, esa opcion falla y se pasa a la siguiente en vez de dar
 *   la descarga por buena -- que es justo lo que se quiere de un espejo en el
 *   que no confiamos tanto como en el primario.
 */
Task::Ptr makeFileTask(const File& file, const QString& destPath);

/**
 * Cuantos origenes utiles tiene una entrada, ya filtrados.
 *
 * Existe aparte para poder comprobarlo sin construir tareas ni tocar la red.
 */
QStringList usableOrigins(const File& file);

}  // namespace Luna
