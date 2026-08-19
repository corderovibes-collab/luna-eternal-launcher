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

#include "luna/LunaUpdate.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "luna/LunaApply.h"
#include "luna/LunaDownload.h"
#include "luna/LunaFetch.h"
#include "tasks/ConcurrentTask.h"

namespace Luna {

namespace {

/** Mira el disco de verdad. La version de mentira vive en las pruebas. */
class RealDisk final : public DiskProbe {
   public:
    explicit RealDisk(QString root) : m_root(std::move(root)) {}

    bool exists(const QString& rel) const override { return QFileInfo::exists(abs(rel)); }
    qint64 size(const QString& rel) const override { return QFileInfo(abs(rel)).size(); }

    QString sha1(const QString& rel) const override
    {
        QFile f(abs(rel));
        if (!f.open(QIODevice::ReadOnly))
            return {};
        QCryptographicHash h(QCryptographicHash::Sha1);
        // Por trozos: hay ficheros de 129 MB en este pack y leerlos enteros en
        // memoria para resumirlos es gratuito solo hasta que no lo es.
        if (!h.addData(&f))
            return {};
        return QString::fromLatin1(h.result().toHex());
    }

    QStringList modJars() const override
    {
        QStringList out;
        QDir mods(QDir(m_root).absoluteFilePath(QStringLiteral("mods")));
        if (!mods.exists())
            return out;  // instalacion desde cero
        for (const auto& n : mods.entryList({ QStringLiteral("*.jar") }, QDir::Files))
            out << QStringLiteral("mods/%1").arg(n);
        return out;
    }

   private:
    QString abs(const QString& rel) const { return resolveInInstance(m_root, rel); }
    QString m_root;
};

/** Cuantas descargas a la vez. */
constexpr int kConcurrencia = 10;

}  // namespace

UpdateTask::UpdateTask(QString instanceRoot, QString profile, Mode mode, QObject* parent)
    : Task(parent), m_instanceRoot(std::move(instanceRoot)), m_profile(std::move(profile)), m_mode(mode)
{}

QString UpdateTask::statePath(const QString& instanceRoot)
{
    return QDir(instanceRoot).absoluteFilePath(QStringLiteral("luna-installed.json"));
}

bool UpdateTask::abort()
{
    if (m_sub)
        m_sub->abort();
    return Task::abort();
}

void UpdateTask::executeTask()
{
    auto fetch = makeShared<FetchManifestTask>();
    m_sub = fetch;
    connect(fetch.get(), &Task::succeeded, this, [this, fetch] {
        m_manifest = fetch->manifest();
        // ⚠⚠ SIN ESTO, UN MANIFIESTO VACIO BORRA EL PACK ENTERO.
        //
        // `computePlan` con un manifiesto sin ficheros produce un plan sin nada
        // que bajar y CON TODOS LOS MODS COMO HUERFANOS, porque ninguno aparece
        // ya en lo esperado. El barrido los retiraria y la tarea diria que todo
        // fue bien.
        //
        // Estuvo a punto de pasar: la cadena del puntero se rompio, el
        // manifiesto llego vacio y `UpdateTask` reporto exito habiendo hecho
        // nada. Se salvo de milagro porque la instancia estaba recien creada y
        // no habia nada que borrar.
        if (!m_manifest.isValid()) {
            emitFailed(tr("No se pudo leer la lista del pack. No se toca nada."));
            return;
        }
        conElManifiesto();
    });
    connect(fetch.get(), &Task::failed, this, [this](QString m) { emitFailed(m); });
    connect(fetch.get(), &Task::progress, this, &UpdateTask::setProgress);
    connect(fetch.get(), &Task::status, this, &UpdateTask::setStatus);
    fetch->start();
}

void UpdateTask::conElManifiesto()
{
    setStatus(tr("Comprobando lo que ya tienes…"));

    // ⚠ `Luna::` NO SOBRA. `Task`, la clase base, tiene su propio `State` (el
    //   ciclo de vida de la tarea), y dentro del cuerpo de la clase ese tapa al
    //   nuestro. Sin cualificar, el compilador cree que se le pide convertir el
    //   estado de una tarea en un mapa de ficheros.
    Luna::State instalado;
    QFile f(statePath(m_instanceRoot));
    if (f.open(QIODevice::ReadOnly))
        instalado = stateFromJson(f.readAll());

    const RealDisk disco(m_instanceRoot);
    m_plan = computePlan(m_manifest, instalado, m_profile, m_mode, disco);

    // ⚠ GUARDA: si el plan quiere retirar TODO y no bajar nada, algo esta mal.
    //
    // Un manifiesto correcto nunca deja la instancia vacia. Si el plan dice eso
    // es que llego a medias, y borrar es la unica accion de esta tarea que NO
    // se puede deshacer: lo bajado se vuelve a bajar, lo borrado hay que
    // volver a bajarlo tambien, pero mientras tanto el jugador no puede jugar.
    if (m_plan.toFetch.isEmpty() && m_plan.toRemove.size() > 10) {
        emitFailed(tr("El pack pide retirar %1 ficheros y no instalar ninguno. "
                      "Eso no puede estar bien, asi que no se toca nada.")
                       .arg(m_plan.toRemove.size()));
        return;
    }

    // Paso 4: BORRAR ANTES DE DESCARGAR. Ver el porque en la cabecera.
    m_removed = removeEntries(m_instanceRoot, m_plan.toRemove);

    if (m_plan.toFetch.isEmpty()) {
        terminar();
        return;
    }
    descargar();
}

void UpdateTask::descargar()
{
    setStatus(tr("Descargando %1 ficheros…").arg(m_plan.toFetch.size()));

    auto grupo = makeShared<ConcurrentTask>(tr("Pack"), kConcurrencia);
    for (const auto& f : m_plan.toFetch) {
        // Un zip se baja a un temporal y se extrae despues; un fichero suelto
        // va directo a su sitio.
        const QString destino = f.file.archive
                                    ? QDir(m_instanceRoot).absoluteFilePath(
                                          QStringLiteral(".luna-cache/%1.zip").arg(f.file.sha1))
                                    : resolveInInstance(m_instanceRoot, f.file.path);
        // ⚠⚠ AQUI NO SE SALTA NADA EN SILENCIO, Y ES IMPORTANTE.
        //
        // Si un fichero del plan no se puede programar --ruta insegura, o
        // ningun origen utilizable-- y se ignorara, al terminar se guardaria
        // `nextState` ANOTANDOLO COMO INSTALADO. El arranque siguiente se
        // fiaria del atajo y no lo volveria a intentar NUNCA: el jugador se
        // queda sin ese mod y el launcher cree que lo tiene.
        //
        // Es exactamente el fallo contra el que avisa la cabecera de este
        // fichero, colandose por la puerta de al lado. Mejor parar con un
        // motivo claro que dejar una instalacion que miente.
        auto t = destino.isEmpty() ? nullptr : makeFileTask(f.file, destino);
        if (!t) {
            emitFailed(tr("No se puede descargar %1: el manifiesto no trae un origen valido.").arg(f.file.path));
            return;
        }
        grupo->addTask(t);
    }

    m_sub = grupo;
    connect(grupo.get(), &Task::succeeded, this, [this] { terminar(); });
    connect(grupo.get(), &Task::failed, this, [this](QString m) { emitFailed(m); });
    connect(grupo.get(), &Task::progress, this, &UpdateTask::setProgress);
    grupo->start();
}

void UpdateTask::terminar()
{
    // Extraer los zips que se acaban de bajar.
    for (const auto& f : m_plan.toFetch) {
        if (!f.file.archive)
            continue;
        const QString zip = QDir(m_instanceRoot).absoluteFilePath(QStringLiteral(".luna-cache/%1.zip").arg(f.file.sha1));
        const QString destino = resolveInInstance(m_instanceRoot, f.file.path);
        if (destino.isEmpty() || !QFile::exists(zip))
            continue;

        setStatus(tr("Extrayendo %1…").arg(f.file.path));
        const auto r = extractArchive(zip, destino, f.file.strip, f.file.keepExisting);
        if (!r.ok()) {
            emitFailed(tr("No se pudo extraer %1: %2").arg(f.file.path, r.error));
            return;
        }
        QFile::remove(zip);
    }

    m_downloaded = m_plan.toFetch.size();

    // ⚠ PASO 7, Y VA AQUI POR ALGO. Guardar el estado antes de terminar
    //   dejaria anotado como instalado algo que no esta, y el arranque
    //   siguiente se fiaria del atajo y no lo volveria a bajar nunca.
    QString err;
    if (!writeFileSafely(statePath(m_instanceRoot), stateToJson(m_plan.nextState, m_manifest.packVersion), &err)) {
        emitFailed(tr("No se pudo guardar el estado del pack: %1").arg(err));
        return;
    }

    setStatus(tr("Listo · %1 descargados, %2 retirados").arg(m_downloaded).arg(m_removed));
    emitSucceeded();
}

}  // namespace Luna
