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
#include "Application.h"
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
    m_buffers.clear();
    for (const auto& u : urls) {
        auto [dl, buf] = Net::Download::makeByteArray(QUrl(u));
        // ⚠⚠ SIN ESTO LA DESCARGA REVIENTA AL ARRANCAR, NO AL RECIBIR.
        //
        // `Net::Download` guarda un puntero al gestor de red y lo usa en cuanto
        // empieza. Quien se lo pasa es `NetJob`, y SOLO `NetJob`. Metiendo las
        // descargas en un `MultipleOptionsTask` --que es un `ConcurrentTask`,
        // no un `NetJob`-- nadie se lo asigna y se llama sobre un nulo.
        //
        // Paso de verdad: el launcher se cerraba entero al pulsar el boton, y
        // el registro se cortaba justo en "Running <url>", que parecia un
        // problema de red cuando era un puntero sin asignar.
        dl->setNetwork(APPLICATION->network());
        grupo->addTask(dl);
        m_buffers << buf;  // uno por origen: ver la cabecera
    }
    // El buffer pertenece al Download, que vive dentro del grupo. Solo se puede
    // leer mientras el grupo siga vivo -- por eso se copia en cuanto llega.
    m_sub = grupo;

    // ⚠⚠ EL SIGUIENTE PASO SE APLAZA AL BUCLE DE EVENTOS, Y NO ES UN ADORNO.
    //
    // `recibido()` vuelve a llamar a `pedir()`, que reasigna `m_sub`. Si eso
    // ocurriera aqui dentro, se estaria DESTRUYENDO LA TAREA MIENTRAS SE
    // EJECUTA SU PROPIO MANEJADOR: uso despues de liberar, y el launcher se
    // cierra entero sin dialogo ni mensaje.
    //
    // Paso de verdad: el registro se cortaba justo al lanzar la descarga y la
    // ventana desaparecia. `Qt::QueuedConnection` deja que la señal termine de
    // emitirse antes de tocar nada.
    connect(grupo.get(), &Task::succeeded, this, [this, esPuntero] { recibido(esPuntero); },
            Qt::QueuedConnection);
    connect(grupo.get(), &Task::failed, this, [this, esPuntero](QString motivo) {  // NOLINT
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
    }, Qt::QueuedConnection);

    grupo->start();
}

void FetchManifestTask::recibido(bool esPuntero)
{
    // El primero que traiga algo es el del origen que funciono.
    QByteArray crudo;
    for (auto* b : m_buffers) {
        if (b && !b->isEmpty()) {
            crudo = *b;  // copia: el original muere con el grupo
            break;
        }
    }
    if (crudo.isEmpty()) {
        emitFailed(tr("La descarga no dejo contenido."));
        return;
    }
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
