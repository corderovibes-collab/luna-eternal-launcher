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

#include "luna/LunaSync.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace Luna {

namespace {

/**
 * Las carpetas que administra el launcher.
 *
 * `mods` tiene que estar: es lo que hace que quitar un mod del manifiesto lo
 * DESINSTALE. `config`, `resourcepacks` y `shaderpacks` tambien, aunque lo que
 * hay dentro suele ir marcado `once` y no se pisa.
 */
const QStringList& managedDirs()
{
    static const QStringList dirs{ QStringLiteral("mods"), QStringLiteral("config"), QStringLiteral("resourcepacks"),
                                   QStringLiteral("shaderpacks"), QStringLiteral("datapacks") };
    return dirs;
}

}  // namespace

bool isManaged(const QString& relPath)
{
    for (const auto& d : managedDirs()) {
        if (relPath == d || relPath.startsWith(d + QLatin1Char('/')))
            return true;
    }
    return false;
}

bool isSafeRelativePath(const QString& relPath)
{
    if (relPath.isEmpty())
        return false;
    // Absoluta, o con unidad (C:), o UNC.
    if (QDir::isAbsolutePath(relPath) || relPath.contains(QLatin1Char(':')))
        return false;
    // La barra invertida se normaliza a `/` antes de mirar, o la variante de
    // Windows (`..` separados por barra invertida) se colaria entera: solo se
    // busca `..` entre barras normales.
    //
    // Se escribe por codigo (0x5C) y no como literal a proposito: un literal de
    // barra invertida se pierde con facilidad al pasar por generadores y
    // plantillas, y aqui equivocarse significa dejar abierta la puerta que este
    // metodo existe para cerrar.
    static const QChar kBackslash(0x5C);
    const auto norm = QString(relPath).replace(kBackslash, QLatin1Char('/'));
    for (const auto& part : norm.split(QLatin1Char('/'))) {
        if (part == QLatin1String(".."))
            return false;
    }
    return true;
}

Plan computePlan(const Manifest& manifest, const State& installed, const QString& profile, Mode mode, const DiskProbe& disk)
{
    Plan plan;

    for (const auto& f : manifest.files) {
        if (!f.forProfile(profile))
            continue;
        // Una ruta insegura no se instala NI se anota. Ver `isSafeRelativePath`.
        if (!isSafeRelativePath(f.path))
            continue;

        plan.nextState.insert(f.path, f.sha1);

        if (f.once) {
            // `once`: se escribe si falta y NUNCA se pisa. En cuanto el jugador
            // toca un ajuste el fichero deja de coincidir con el del
            // manifiesto, y darlo por corrupto significaba restaurarlo --o sea
            // borrarle la configuracion-- en cada arranque.
            if (!disk.exists(f.path))
                plan.toFetch.append({ f, QStringLiteral("falta (once)") });
            continue;
        }

        if (f.archive) {
            // De una carpeta no se puede comprobar el sha1: basta con que el
            // manifiesto siga anunciando el mismo zip que se extrajo.
            if (mode == Mode::Repair || installed.value(f.path) != f.sha1)
                plan.toFetch.append({ f, QStringLiteral("zip distinto") });
            continue;
        }

        if (mode == Mode::Repair) {
            // Reparar no se fia ni del estado guardado ni del tamaño.
            if (disk.sha1(f.path) != f.sha1)
                plan.toFetch.append({ f, QStringLiteral("huella no cuadra") });
            continue;
        }

        if (installed.value(f.path) != f.sha1) {
            plan.toFetch.append({ f, QStringLiteral("version nueva") });
            continue;
        }
        // El estado dice que ya esta: se confirma que sigue ahi y con su
        // tamaño, que es barato. Sin esto, un fichero borrado a mano no
        // volveria nunca.
        if (!disk.exists(f.path) || (f.size > 0 && disk.size(f.path) != f.size))
            plan.toFetch.append({ f, QStringLiteral("falta o tamaño distinto") });
    }

    // Lo que estaba instalado y ya no toca: mod retirado, renombrado, o del
    // otro perfil. SOLO dentro de lo que administramos.
    for (auto it = installed.constBegin(); it != installed.constEnd(); ++it) {
        if (!plan.nextState.contains(it.key()) && isManaged(it.key()) && isSafeRelativePath(it.key()))
            plan.toRemove.append(it.key());
    }

    // ⚠⚠ Y ADEMAS SE BARRE `mods/` DE VERDAD. Ver `DiskProbe::modJars()`.
    //
    // Solo `mods/`: es 100 % del pack. `config/`, `resourcepacks/` y
    // `shaderpacks/` llevan cosas del jugador, y ahi no se toca nada que no
    // estuviera anotado.
    //
    // Consecuencia asumida: un mod que el jugador añada a mano desaparece en la
    // siguiente actualizacion. Es lo correcto aqui -- la regla del proyecto es
    // que el servidor tiene que ser SUBCONJUNTO del cliente, y un mod extra que
    // registre algo sincronizado es justo lo que echa a la gente.
    for (const auto& rel : disk.modJars()) {
        if (!plan.nextState.contains(rel) && !plan.toRemove.contains(rel) && isSafeRelativePath(rel))
            plan.toRemove.append(rel);
    }

    plan.toRemove.sort();

    for (const auto& f : plan.toFetch)
        plan.bytes += f.file.size;

    return plan;
}

QByteArray stateToJson(const State& state, const QString& packVersion)
{
    QJsonObject files;
    for (auto it = state.constBegin(); it != state.constEnd(); ++it)
        files.insert(it.key(), it.value());

    QJsonObject root;
    root.insert(QStringLiteral("version"), packVersion);
    root.insert(QStringLiteral("files"), files);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

State stateFromJson(const QByteArray& raw, QString* packVersion)
{
    State out;
    const auto doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject())
        return out;
    const auto root = doc.object();
    if (packVersion)
        *packVersion = root.value(QStringLiteral("version")).toString();
    const auto files = root.value(QStringLiteral("files")).toObject();
    for (auto it = files.constBegin(); it != files.constEnd(); ++it)
        out.insert(it.key(), it.value().toString());
    return out;
}

}  // namespace Luna
