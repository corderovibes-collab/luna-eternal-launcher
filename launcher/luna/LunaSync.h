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

#include <QHash>
#include <QList>
#include <QString>

#include "luna/LunaManifest.h"

namespace Luna {

/**
 * Lo que el launcher cree que dejo instalado la ultima vez: ruta -> sha1.
 *
 * Es un ATAJO, y por eso existe: sin el habria que leer y resumir los ~450 MB
 * del pack EN CADA ARRANQUE, y solo Cobblemon son 129. Con el basta comprobar
 * que el fichero sigue ahi y con su tamaño, que es una llamada al sistema.
 *
 * Y es tambien su propia limitacion: da por bueno lo que el launcher CREE que
 * instalo, asi que un fichero que se corrompio DESPUES sobrevive a cualquier
 * numero de actualizaciones. Para eso esta `Mode::Repair`.
 */
using State = QHash<QString, QString>;

enum class Mode {
    Normal,  ///< se fia del estado guardado. Es el arranque de todos los dias
    Repair,  ///< no se fia de nada: comprueba el sha1 de todo lo que hay en disco
};

/**
 * Lo que hace falta saber del disco para decidir.
 *
 * Es una interfaz y no llamadas directas al sistema de ficheros para que la
 * decision --que es donde estan todos los fallos interesantes-- se pueda probar
 * sin tocar el disco.
 */
class DiskProbe {
   public:
    virtual ~DiskProbe() = default;
    virtual bool exists(const QString& relPath) const = 0;
    virtual qint64 size(const QString& relPath) const = 0;
    /** Caro: solo se llama en `Mode::Repair`. */
    virtual QString sha1(const QString& relPath) const = 0;

    /**
     * Los `.jar` que hay DE VERDAD en `mods/`, como rutas relativas.
     *
     * ⚠ HACE FALTA PORQUE EL ESTADO GUARDADO NO BASTA.
     *
     * La limpieza que solo mira `installed.json` alcanza lo que el launcher
     * recuerda haber puesto. Un jar que llego por otra via --una instalacion
     * anterior, o una version que aun no lo apuntaba-- sobrevive a TODAS las
     * actualizaciones, para siempre.
     *
     * Eso dejo a jugadores fuera del servidor el 2026-08-19: arrastraban
     * `trinkets` y `accessories-compat-layer` de un pack anterior, y ese puente
     * mandaba unas ranuras que el servidor no sabia leer. Al dueño no le pasaba
     * --su instalacion estaba bien anotada-- asi que parecia cosa de maquinas
     * concretas.
     */
    virtual QStringList modJars() const = 0;
};

/** Un fichero que hay que traer. */
struct Fetch {
    File file;
    QString reason;  ///< para el registro: por que se baja
};

struct Plan {
    QList<Fetch> toFetch;
    QStringList toRemove;  ///< instalado, ya no esta en el manifiesto
    State nextState;       ///< lo que quedara instalado al terminar
    qint64 bytes = 0;

    bool isEmpty() const { return toFetch.isEmpty() && toRemove.isEmpty(); }
};

/**
 * ¿Esta ruta cae en una carpeta que administramos?
 *
 * Solo dentro de ellas se puede BORRAR. Fuera --mundos, capturas, registros--
 * es del jugador y no se toca ni aunque el manifiesto deje de mencionarlo.
 */
bool isManaged(const QString& relPath);

/**
 * ¿Es una ruta relativa segura?
 *
 * ⚠ Bloquea `../`, rutas absolutas y unidades. El manifiesto viene de la red:
 *   sin esto, una entrada con `../../` escribe donde quiera en la maquina del
 *   jugador. Es la razon por la que el manifiesto se verifica por huella, y
 *   esta es la segunda cerradura por si esa fallara.
 */
bool isSafeRelativePath(const QString& relPath);

/**
 * Decide que bajar y que borrar. No toca nada: solo decide.
 */
Plan computePlan(const Manifest& manifest,
                 const State& installed,
                 const QString& profile,
                 Mode mode,
                 const DiskProbe& disk);

/** Serializa y lee el estado. */
QByteArray stateToJson(const State& state, const QString& packVersion);
State stateFromJson(const QByteArray& raw, QString* packVersion = nullptr);

}  // namespace Luna
