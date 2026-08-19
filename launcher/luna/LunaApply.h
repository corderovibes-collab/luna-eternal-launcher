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

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Luna {

/**
 * Escribe un fichero aunque el que ya este ahi sea OCULTO o de solo lectura.
 *
 * ⚠ EN WINDOWS, ABRIR PARA ESCRITURA UN FICHERO OCULTO FALLA.
 *
 * No es un permiso del usuario ni un antivirus: `CreateFile` con
 * `CREATE_ALWAYS` se niega si el fichero existente tiene el atributo `HIDDEN`
 * y la llamada no lo repite. El error sale con el disco sano, la carpeta
 * escribible y el usuario siendo administrador -- y no menciona en ningun
 * momento que el problema sea que el fichero esta oculto.
 *
 * Ocurrio de verdad y llego a un jugador: el pack trae
 * `config/euphoria_patcher/.data.json`, y el mod EuphoriaPatcher lo vuelve a
 * crear OCULTO en el PC. A partir de ahi, toda actualizacion que reextrajera
 * `config/` moria al 99 % con `EPERM: operation not permitted`.
 *
 * Se borra y se vuelve a escribir: borrar SI funciona sobre un fichero oculto.
 */
bool writeFileSafely(const QString& absPath, const QByteArray& data, QString* error = nullptr);

/**
 * Ruta de una entrada del zip tras quitarle `strip` niveles de carpeta.
 *
 * Devuelve VACIO si no queda nada, o si la entrada intenta escaparse. Un zip
 * tambien viene de la red: `../` dentro de un archivo es la via clasica para
 * escribir fuera del destino (el llamado zip-slip).
 */
QString strippedEntryPath(const QString& entryName, int strip);

struct ExtractResult {
    int written = 0;
    int skipped = 0;  ///< ya existian y `keepExisting` mandaba respetarlos
    QString error;

    bool ok() const { return error.isEmpty(); }
};

/**
 * Extrae un zip en `destDir`.
 *
 * @param keepExisting  no pisar lo que ya exista. Es como se sirve la
 *                      configuracion del pack: llega en un zip por carpeta y
 *                      lo que el jugador haya ajustado se respeta. Lo que si
 *                      llega es un fichero NUEVO, porque ese aun no existe.
 */
ExtractResult extractArchive(const QString& zipPath, const QString& destDir, int strip, bool keepExisting);

/**
 * Borra lo que el manifiesto ya no menciona.
 *
 * ⚠ Quien llame tiene que haber filtrado por `isManaged()` ANTES. Aqui no se
 *   vuelve a comprobar a proposito: si esta funcion decidiera tambien que se
 *   puede borrar, habria dos sitios donde equivocarse.
 *
 * Devuelve cuantos se borraron. Los que no existian no cuentan ni fallan: que
 * un fichero ya no este es exactamente el estado que se buscaba.
 */
int removeEntries(const QString& instanceRoot, const QStringList& relPaths);

}  // namespace Luna
