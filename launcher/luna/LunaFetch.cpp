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

#include "luna/LunaFetch.h"

#include <QUrl>

#include "luna/LunaConfig.h"
#include "net/Download.h"
#include "tasks/MultipleOptionsTask.h"

namespace Luna {

FetchManifestTask::FetchManifestTask(QObject* parent) : Task(parent) {}

bool FetchManifestTask::abort()
{
    if (m_sub)
        m_sub->abort();
    return Task::abort();
}

void FetchManifestTask::executeTask()
{
    setStatus(tr("Comprobando el pack…"));
    pedir({ pointerUrl() }, true);
}

void FetchManifestTask::pedir(const QStringList& urls, bool esPuntero)
{
    if (urls.isEmpty()) {
        emitFailed(tr("No hay ningun origen del que traer el pack."));
        return;
    }

    // Un solo origen tambien pasa por aqui: asi el camino es el mismo con y sin
    // espejos, y no hay dos formas distintas de fallar.
    auto grupo = makeShared<MultipleOptionsTask>(tr("Manifiesto"));
    QByteArray* ultimo = nullptr;
    for (const auto& u : urls) {
        auto [dl, buf] = Net::Download::makeByteArray(QUrl(u));
        grupo->addTask(dl);
        ultimo = buf;
    }
    // ⚠ El buffer pertenece al Download, que vive dentro del grupo. Solo se
    //   puede leer mientras `m_sub` siga vivo -- por eso se copia en cuanto
    //   llega y no se guarda el puntero mas alla.
    m_buffer = ultimo;
    m_sub = grupo;

    connect(grupo.get(), &Task::succeeded, this, [this, esPuntero] { recibido(esPuntero); });
    connect(grupo.get(), &Task::failed, this, [this, esPuntero](QString motivo) {
        // Si el PUNTERO no contesta, queda el manifiesto entero en `raw`. Es el
        // camino de los launchers viejos: mas lento y con cache, pero mejor que
        // dejar al jugador sin pack.
        if (esPuntero && !m_yaCaiAlRespaldo) {
            m_yaCaiAlRespaldo = true;
            setStatus(tr("El origen principal no contesta; probando el de respaldo…"));
            pedir({ fallbackManifestUrl() }, true);
            return;
        }
        emitFailed(motivo);
    });

    grupo->start();
}

void FetchManifestTask::recibido(bool esPuntero)
{
    if (!m_buffer) {
        emitFailed(tr("La descarga no dejo contenido."));
        return;
    }
    const QByteArray crudo = *m_buffer;  // copia: ver el aviso en `pedir`
    QString error;

    if (esPuntero && looksLikePointer(crudo)) {
        m_pointer = parsePointer(crudo, &error);
        if (!m_pointer.isValid()) {
            emitFailed(error.isEmpty() ? tr("El puntero del pack no se entiende.") : error);
            return;
        }
        setStatus(tr("Descargando la lista del pack…"));
        QStringList origenes{ m_pointer.manifestUrl };
        origenes << m_pointer.mirrors;
        pedir(origenes, false);
        return;
    }

    // Manifiesto directo: o bien el puntero llevaba aqui, o bien alguien apunta
    // el launcher a la direccion vieja.
    m_manifest = parseManifest(crudo, m_pointer.sha1, &error);
    if (!m_manifest.isValid()) {
        emitFailed(error.isEmpty() ? tr("La lista del pack no se entiende.") : error);
        return;
    }

    setStatus(tr("Pack %1 · %2 ficheros").arg(m_manifest.packVersion).arg(m_manifest.files.size()));
    emitSucceeded();
}

}  // namespace Luna
