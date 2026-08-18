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
#include <QList>
#include <QString>
#include <QStringList>

/**
 * Lectura del manifiesto del pack de Luna Eternal.
 *
 * Aqui NO hay red: solo se interpreta y se valida lo que llegue. Es
 * deliberado, porque asi lo que decide que se instala en la maquina de un
 * jugador se puede probar entero sin depender de que GitHub tenga un buen dia.
 */
namespace Luna {

/** Un fichero del pack. */
struct File {
    QString path;   ///< donde va, relativo a la instancia
    QString sha1;   ///< huella esperada
    qint64 size = 0;

    /**
     * De donde se puede bajar, en orden de preferencia.
     *
     * El manifiesto trae `urls` desde el pack 0.3.0 y `url` a secas desde
     * antes. Se admiten los dos porque el launcher y el pack no se actualizan
     * a la vez, y durante un rato conviven.
     */
    QStringList urls;

    /**
     * `once`: se escribe si falta y NUNCA se pisa.
     *
     * Son los ajustes del jugador (servers.dat, config/, shaderpacks/). Sin
     * esto, a quien hubiera tocado un shader se lo revertiriamos en cada
     * arranque.
     */
    bool once = false;

    /** El fichero es un zip que se extrae en `path` en vez de copiarse. */
    bool archive = false;
    /** Al extraer, no pisar lo que ya exista. */
    bool keepExisting = false;
    /** Niveles de carpeta que se quitan del principio al extraer. */
    int strip = 0;

    /** Vacio = para todos los perfiles. Si no, `jugador` o `constructor`. */
    QStringList profiles;

    bool forProfile(const QString& profile) const { return profiles.isEmpty() || profiles.contains(profile); }
};

/** El manifiesto entero. */
struct Manifest {
    QString packVersion;
    QString minecraft;     ///< p.ej. "1.21.1"
    QString fabricLoader;  ///< p.ej. "0.19.3"

    QString serverName;
    QString serverHost;
    int serverPort = 25565;

    QList<File> files;

    bool isValid() const { return !files.isEmpty() && !minecraft.isEmpty(); }
};

/**
 * El puntero: 250 bytes que dicen cual es el manifiesto vigente.
 *
 * Existe para que publicar mal deje de ser irreversible. El manifiesto lleva su
 * huella en el nombre y no se toca jamas; volver atras es reescribir ESTO.
 */
struct Pointer {
    QString packVersion;
    QString manifestUrl;
    QString sha1;  ///< huella del manifiesto al que apunta
    qint64 size = 0;
    QStringList mirrors;

    bool isValid() const { return !manifestUrl.isEmpty(); }
};

/**
 * Interpreta el puntero. Devuelve invalido y llena `error` si no cuadra.
 */
Pointer parsePointer(const QByteArray& raw, QString* error = nullptr);

/**
 * Interpreta el manifiesto.
 *
 * ⚠ Si `expectedSha1` no esta vacio, se COMPRUEBA antes de interpretar nada, y
 *   se rechaza si no coincide. No es paranoia de manual: este fichero elige de
 *   que URL salen los ~450 MB que se instalan, y con ellas lo que acaba
 *   ejecutandose en la maquina del jugador. Sin la comprobacion, cualquiera que
 *   pueda colocarle un JSON --un DNS envenenado, el proxy de un wifi publico--
 *   le elige los mods.
 */
Manifest parseManifest(const QByteArray& raw, const QString& expectedSha1 = QString(), QString* error = nullptr);

/** Distingue puntero de manifiesto POR EL CONTENIDO, no por la URL. */
bool looksLikePointer(const QByteArray& raw);

}  // namespace Luna
