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

#include "luna/LunaDownload.h"

#include <QDir>
#include <QUrl>

#include "luna/LunaSync.h"
#include "Application.h"
#include "net/ChecksumValidator.h"
#include "net/Download.h"
#include "tasks/MultipleOptionsTask.h"

namespace Luna {

QString resolveInInstance(const QString& instanceRoot, const QString& relPath)
{
    if (!isSafeRelativePath(relPath))
        return {};

    const QDir root(instanceRoot);
    const QString absolute = QDir::cleanPath(root.absoluteFilePath(relPath));

    // Cinturon y tirantes: aunque `isSafeRelativePath` ya haya dicho que si, se
    // confirma que el resultado sigue cayendo DENTRO. Un enlace simbolico o una
    // normalizacion inesperada podrian sacarlo, y aqui ya se va a escribir de
    // verdad.
    const QString rootAbs = QDir::cleanPath(root.absolutePath());
    if (absolute != rootAbs && !absolute.startsWith(rootAbs + QLatin1Char('/')))
        return {};

    return absolute;
}

QStringList usableOrigins(const File& file)
{
    QStringList out;
    for (const auto& u : file.urls) {
        const QUrl parsed(u);
        // Una URL sin esquema o sin servidor no lleva a ningun sitio. Se
        // descarta aqui en vez de dejar que falle al ejecutar, para que
        // "sin origenes utiles" se pueda distinguir de "todos fallaron".
        if (parsed.isValid() && !parsed.scheme().isEmpty() && !parsed.host().isEmpty())
            out << u;
    }
    return out;
}

Task::Ptr makeFileTask(const File& file, const QString& destPath)
{
    if (destPath.isEmpty())
        return nullptr;

    const auto origins = usableOrigins(file);
    if (origins.isEmpty())
        return nullptr;

    auto group = makeShared<MultipleOptionsTask>(QObject::tr("Descargando %1").arg(file.path));

    for (const auto& url : origins) {
        auto dl = Net::Download::makeFile(QUrl(url), destPath);
        // ⚠ Mismo motivo que en LunaFetch: solo `NetJob` asigna el gestor de
        //   red, y esto no es un `NetJob`. Sin esta linea, la descarga usa un
        //   puntero nulo en cuanto arranca.
        dl->setNetwork(APPLICATION->network());

        // ⚠ EL VALIDADOR VA EN CADA ORIGEN, NO UNA VEZ AL FINAL.
        //   Si un espejo sirve un fichero corrupto, esa opcion falla y se pasa
        //   a la siguiente. Validando solo al final, un espejo malo tumbaria la
        //   descarga entera sin llegar a reintentar en otro sitio.
        if (!file.sha1.isEmpty())
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.sha1));

        group->addTask(dl);
    }

    return group;
}

}  // namespace Luna
