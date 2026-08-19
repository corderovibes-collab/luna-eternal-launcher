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

#include "luna/LunaApply.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "archive/ArchiveReader.h"
#include "luna/LunaDownload.h"
#include "luna/LunaSync.h"

namespace Luna {

bool writeFileSafely(const QString& absPath, const QByteArray& data, QString* error)
{
    const QFileInfo info(absPath);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error)
            *error = QObject::tr("No se pudo crear la carpeta %1").arg(info.absolutePath());
        return false;
    }

    auto intentar = [&](QString* err) {
        QFile f(absPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err)
                *err = f.errorString();
            return false;
        }
        const bool escrito = f.write(data) == data.size();
        f.close();
        if (!escrito && err)
            *err = QObject::tr("escritura incompleta");
        return escrito;
    };

    QString primerFallo;
    if (intentar(&primerFallo))
        return true;

    // Ver la cabecera: en Windows esto es lo que pasa con un fichero OCULTO, y
    // el mensaje no lo dice. Borrar si funciona sobre el, asi que se borra y se
    // reescribe. Si el fichero no existia, `remove` no hace nada y el segundo
    // intento falla igual -- entonces el problema era otro y se reporta.
    if (QFile::exists(absPath)) {
        QFile::setPermissions(absPath, QFile::WriteOwner | QFile::ReadOwner);
        QFile::remove(absPath);
        if (intentar(error))
            return true;
    }

    if (error && error->isEmpty())
        *error = primerFallo;
    return false;
}

QString strippedEntryPath(const QString& entryName, int strip)
{
    // Las barras invertidas se normalizan antes de partir, o una entrada de un
    // zip hecho en Windows se cuela entera como un solo nombre.
    static const QChar kBackslash(0x5C);
    QString norm = QString(entryName).replace(kBackslash, QLatin1Char('/'));

    // ⚠ SE COMPRUEBA ANTES DE PARTIR, Y ES NECESARIO.
    //
    // `split(SkipEmptyParts)` se come la barra inicial, asi que `/etc/passwd`
    // se convertiria en `etc/passwd` -- una ruta relativa perfectamente
    // valida. No llegaria a escribir fuera del destino, pero una entrada que
    // se declara absoluta esta malformada y darla por buena en silencio es
    // justo el tipo de indulgencia que acaba costando cara.
    //
    // Lo encontro la prueba, no una revision a ojo.
    if (!isSafeRelativePath(norm))
        return {};

    auto partes = norm.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (strip > 0) {
        if (partes.size() <= strip)
            return {};  // la entrada ERA una de las carpetas que se quitan
        partes = partes.mid(strip);
    }
    if (partes.isEmpty())
        return {};

    const QString rel = partes.join(QLatin1Char('/'));
    // ⚠ Un zip tambien viene de la red. `../` dentro de un archivo es la via
    //   clasica para escribir fuera del destino.
    if (!isSafeRelativePath(rel))
        return {};
    return rel;
}

ExtractResult extractArchive(const QString& zipPath, const QString& destDir, int strip, bool keepExisting)
{
    ExtractResult out;

    if (!QFile::exists(zipPath)) {
        out.error = QObject::tr("No existe el archivo %1").arg(zipPath);
        return out;
    }
    if (!QDir().mkpath(destDir)) {
        out.error = QObject::tr("No se pudo crear %1").arg(destDir);
        return out;
    }

    MMCZip::ArchiveReader lector(zipPath);
    QString fallo;

    const bool recorrido = lector.parse([&](MMCZip::ArchiveReader::File* entrada) {
        if (!entrada->isFile())
            return true;  // las carpetas se crean solas al escribir dentro

        const auto rel = strippedEntryPath(entrada->filename(), strip);
        if (rel.isEmpty()) {
            entrada->skip();
            return true;
        }

        const auto destino = resolveInInstance(destDir, rel);
        if (destino.isEmpty()) {
            entrada->skip();
            return true;
        }

        if (keepExisting && QFile::exists(destino)) {
            entrada->skip();
            out.skipped++;
            return true;
        }

        int estado = 0;
        const auto datos = entrada->readAll(&estado);
        QString err;
        if (!writeFileSafely(destino, datos, &err)) {
            fallo = QObject::tr("%1: %2").arg(rel, err);
            return false;  // corta el recorrido
        }
        out.written++;
        return true;
    });

    if (!fallo.isEmpty())
        out.error = fallo;
    else if (!recorrido)
        out.error = QObject::tr("No se pudo leer %1").arg(zipPath);

    return out;
}

int removeEntries(const QString& instanceRoot, const QStringList& relPaths)
{
    int borrados = 0;
    for (const auto& rel : relPaths) {
        const auto abs = resolveInInstance(instanceRoot, rel);
        if (abs.isEmpty())
            continue;  // ruta insegura: ni se toca

        const QFileInfo info(abs);
        if (!info.exists())
            continue;  // ya no estaba, que es el estado que se buscaba

        // Una entrada retirada puede ser un fichero suelto o una carpeta
        // entera que se extrajo de un zip.
        const bool ok = info.isDir() ? QDir(abs).removeRecursively() : QFile::remove(abs);
        if (ok)
            borrados++;
    }
    return borrados;
}

}  // namespace Luna
