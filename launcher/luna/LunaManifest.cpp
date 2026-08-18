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

#include "LunaManifest.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace Luna {

namespace {

/** Deja `error` puesto si se puede, y devuelve siempre false. */
bool fail(QString* error, const QString& msg)
{
    if (error)
        *error = msg;
    return false;
}

QJsonObject parseObject(const QByteArray& raw, QString* error, bool* ok)
{
    QJsonParseError perr{};
    auto doc = QJsonDocument::fromJson(raw, &perr);
    if (perr.error != QJsonParseError::NoError) {
        fail(error, QObject::tr("El fichero no es JSON valido: %1").arg(perr.errorString()));
        *ok = false;
        return {};
    }
    if (!doc.isObject()) {
        fail(error, QObject::tr("El fichero no es un objeto JSON"));
        *ok = false;
        return {};
    }
    *ok = true;
    return doc.object();
}

/**
 * De donde se puede bajar una entrada, en orden.
 *
 * Se acepta `urls` (lista) y `url` (cadena suelta). Nunca las dos a medias: si
 * hay lista, manda la lista, y `url` solo entra si la lista no trae nada -- que
 * es el caso de los manifiestos anteriores al pack 0.3.0.
 */
QStringList readOrigins(const QJsonObject& o)
{
    QStringList out;
    const auto urls = o.value(QStringLiteral("urls")).toArray();
    for (const auto& u : urls) {
        const auto s = u.toString();
        if (!s.isEmpty())
            out << s;
    }
    if (out.isEmpty()) {
        const auto single = o.value(QStringLiteral("url")).toString();
        if (!single.isEmpty())
            out << single;
    }
    return out;
}

}  // namespace

bool looksLikePointer(const QByteArray& raw)
{
    bool ok = false;
    QString ignored;
    const auto o = parseObject(raw, &ignored, &ok);
    if (!ok)
        return false;
    // El manifiesto trae `files`; el puntero trae `manifest`. Mirar el
    // CONTENIDO y no la URL es lo unico que no depende de que nadie haya
    // migrado nada: un launcher con la direccion vieja guardada sigue valiendo.
    return !o.contains(QStringLiteral("files")) && o.contains(QStringLiteral("manifest"));
}

Pointer parsePointer(const QByteArray& raw, QString* error)
{
    Pointer p;
    bool ok = false;
    const auto o = parseObject(raw, error, &ok);
    if (!ok)
        return p;

    p.manifestUrl = o.value(QStringLiteral("manifest")).toString();
    if (p.manifestUrl.isEmpty()) {
        fail(error, QObject::tr("El puntero no dice a que manifiesto apunta"));
        return {};
    }
    p.packVersion = o.value(QStringLiteral("packVersion")).toString();
    p.sha1 = o.value(QStringLiteral("sha1")).toString().toLower();
    p.size = static_cast<qint64>(o.value(QStringLiteral("size")).toDouble());
    for (const auto& m : o.value(QStringLiteral("espejos")).toArray()) {
        const auto s = m.toString();
        if (!s.isEmpty())
            p.mirrors << s;
    }
    return p;
}

Manifest parseManifest(const QByteArray& raw, const QString& expectedSha1, QString* error)
{
    // ⚠ LA HUELLA SE COMPRUEBA ANTES DE INTERPRETAR NADA.
    //   Ver el porque en la cabecera. Si no cuadra, no se mira ni una linea del
    //   contenido: el fichero ya no es de fiar.
    if (!expectedSha1.isEmpty()) {
        const auto actual = QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
        if (actual.compare(expectedSha1, Qt::CaseInsensitive) != 0) {
            fail(error, QObject::tr("El manifiesto no coincide con su huella. "
                                    "Vuelve a darle a Jugar; si sigue pasando, avisa en Discord."));
            return {};
        }
    }

    Manifest m;
    bool ok = false;
    const auto o = parseObject(raw, error, &ok);
    if (!ok)
        return m;

    if (!o.contains(QStringLiteral("files"))) {
        fail(error, QObject::tr("El manifiesto no trae lista de ficheros"));
        return {};
    }

    m.packVersion = o.value(QStringLiteral("packVersion")).toString();
    m.minecraft = o.value(QStringLiteral("minecraft")).toString();
    m.fabricLoader = o.value(QStringLiteral("fabricLoader")).toString();

    const auto srv = o.value(QStringLiteral("server")).toObject();
    m.serverName = srv.value(QStringLiteral("name")).toString();
    m.serverHost = srv.value(QStringLiteral("host")).toString();
    if (srv.contains(QStringLiteral("port")))
        m.serverPort = srv.value(QStringLiteral("port")).toInt(25565);

    for (const auto& v : o.value(QStringLiteral("files")).toArray()) {
        const auto f = v.toObject();
        File file;
        file.path = f.value(QStringLiteral("path")).toString();
        file.sha1 = f.value(QStringLiteral("sha1")).toString().toLower();
        file.size = static_cast<qint64>(f.value(QStringLiteral("size")).toDouble());
        file.urls = readOrigins(f);
        file.once = f.value(QStringLiteral("once")).toBool(false);
        file.archive = f.value(QStringLiteral("archive")).toBool(false);
        file.keepExisting = f.value(QStringLiteral("keepExisting")).toBool(false);
        file.strip = f.value(QStringLiteral("strip")).toInt(0);
        for (const auto& p : f.value(QStringLiteral("profiles")).toArray()) {
            const auto s = p.toString();
            if (!s.isEmpty())
                file.profiles << s;
        }

        // Una entrada sin ruta o sin origen no se puede instalar. Se rechaza el
        // manifiesto ENTERO en vez de saltarsela: un pack al que le falta un mod
        // no arranca, y "Incompatible mods found!" en la pantalla del jugador es
        // mucho peor que un error claro aqui.
        if (file.path.isEmpty() || file.urls.isEmpty()) {
            fail(error, QObject::tr("El manifiesto trae una entrada sin ruta o sin origen (%1)")
                            .arg(file.path.isEmpty() ? QStringLiteral("sin nombre") : file.path));
            return {};
        }
        m.files << file;
    }

    if (m.files.isEmpty()) {
        fail(error, QObject::tr("El manifiesto no trae ni un fichero"));
        return {};
    }
    return m;
}

}  // namespace Luna
