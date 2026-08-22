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
#include <QTimer>
#include <QUrl>

#include "luna/LunaSync.h"
#include "Application.h"
#include "net/ChecksumValidator.h"
#include "net/Download.h"

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

bool origenDescartado(int http)
{
    // Las dos excepciones hablan de TIEMPO y no de contenido: 408 (se agoto la
    // espera) y 429 (vas demasiado rapido). Esas cambian solas; un 404 no.
    return http >= 400 && http < 500 && http != 408 && http != 429;
}

// ⚠ NO ES UN NAMESPACE ANONIMO, Y ES A PROPOSITO: moc y los namespaces
//   anonimos se llevan mal segun la version, y el sintoma es un error de
//   enlazado que no menciona ni moc ni el namespace.
namespace detalle {

constexpr int kEsperaBaseMs = 1000;
constexpr int kEsperaTopeMs = 15000;

/** Trae un fichero insistiendo: varios origenes, varios intentos, espera creciente. */
class FileTask final : public Task {
    Q_OBJECT
   public:
    FileTask(File file, QString dest, QStringList origenes, int intentosPorOrigen)
        : Task()
        , m_file(std::move(file))
        , m_dest(std::move(dest))
        , m_origenes(std::move(origenes))
        , m_intentosPorOrigen(intentosPorOrigen)
        , m_maxIntentos(static_cast<int>(m_origenes.size()) * intentosPorOrigen)
    {
        m_espera.setSingleShot(true);
        // El reloj grueso basta para una espera de segundos y no despierta la
        // CPU con precision de milisegundo: hay hasta 10 de estas a la vez.
        m_espera.setTimerType(Qt::VeryCoarseTimer);
        connect(&m_espera, &QTimer::timeout, this, &FileTask::intentar);
    }

    bool canAbort() const override { return true; }

    bool abort() override
    {
        m_abortada = true;
        m_espera.stop();
        if (m_actual)
            m_actual->abort();
        return Task::abort();
    }

   protected:
    void executeTask() override
    {
        setStatus(tr("Descargando %1").arg(m_file.path));
        intentar();
    }

   private slots:
    void intentar()
    {
        if (m_abortada)
            return;

        if (m_intento >= m_maxIntentos) {
            rendirse();
            return;
        }

        // Se alterna entre origenes en vez de agotar el primero: si el CDN
        // principal esta teniendo un mal minuto, el espejo contesta ya en el
        // segundo intento en lugar de dentro de cuatro.
        const QString url = m_origenes.at(m_intento % m_origenes.size());

        auto dl = Net::Download::makeFile(QUrl(url), m_dest);
        // Mismo motivo que en LunaFetch: solo `NetJob` asigna el gestor de red,
        // y esto no es un `NetJob`. Sin esta linea, la descarga usa un puntero
        // nulo en cuanto arranca.
        dl->setNetwork(APPLICATION->network());
        // Cubre el 429 con la espera que pida el propio servidor (`Retry-After`),
        // que siempre sera mejor que la nuestra a ciegas. El resto lo lleva el
        // bucle de aqui, porque `AutoRetry` NO cubre nada mas.
        dl->enableAutoRetry(true);

        // EL VALIDADOR VA EN CADA INTENTO, NO UNA VEZ AL FINAL.
        // Si un espejo sirve un fichero corrupto, ese intento falla y se pasa al
        // siguiente. Validando solo al final, un espejo malo tumbaria la
        // descarga entera sin llegar a probar en otro sitio.
        if (!m_file.sha1.isEmpty())
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, m_file.sha1));

        m_actual = dl;
        connect(dl.get(), &Task::succeeded, this, [this] {
            if (!m_abortada)
                emitSucceeded();
        });
        connect(dl.get(), &Task::failed, this, [this, dl, url](QString motivo) { falloIntento(dl, url, motivo); });
        connect(dl.get(), &Task::progress, this, &Task::setProgress);
        dl->start();
    }

   private:
    void falloIntento(Net::Download::Ptr dl, const QString& url, const QString& motivo)
    {
        if (m_abortada)
            return;

        const int http = dl->replyStatusCode();
        m_ultimoServidor = QUrl(url).host();
        m_ultimoMotivo = http > 0 ? tr("HTTP %1").arg(http) : motivo;
        m_intento++;

        if (origenDescartado(http)) {
            // Este origen esta descartado para siempre; los intentos que le
            // quedaban se le regalan a los demas. Si era el unico, se acabo.
            m_origenes.removeAll(url);
            if (m_origenes.isEmpty()) {
                rendirse();
                return;
            }
            m_intento = 0;
            m_maxIntentos = static_cast<int>(m_origenes.size()) * m_intentosPorOrigen;
            intentar();
            return;
        }

        if (m_intento >= m_maxIntentos) {
            rendirse();
            return;
        }

        // 1 s, 2 s, 4 s, 8 s... con tope. Un corte de red domestico dura
        // segundos, no milisegundos: reintentar al instante es reintentar dentro
        // del mismo bache.
        // El desplazamiento se acota antes de hacerlo: hoy `m_intento` no pasa
        // de 8, pero `1000 << 31` es comportamiento indefinido, y lo que cambia
        // ese numero es una constante de otro fichero.
        const int pasos = qBound(0, m_intento - 1, 20);
        const int espera = qMin(kEsperaBaseMs << pasos, kEsperaTopeMs);
        setStatus(tr("%1: reintentando en %2 s (%3)").arg(m_file.path).arg(espera / 1000).arg(m_ultimoMotivo));
        m_espera.start(espera);
    }

    void rendirse()
    {
        // EL MENSAJE NOMBRA EL FICHERO, EL SERVIDOR Y LA RESPUESTA.
        // El anterior era "All attempts have failed!" repetido tres veces: ni
        // que fichero, ni de donde, ni por que. Diagnosticar aquello exigio leer
        // el codigo fuente; esto se lee de la propia ventana.
        emitFailed(tr("%1 - %2 respondio %3 (%4 intentos)").arg(m_file.path, m_ultimoServidor, m_ultimoMotivo).arg(m_intento));
    }

    File m_file;
    QString m_dest;
    QStringList m_origenes;
    int m_intentosPorOrigen;
    int m_maxIntentos;
    int m_intento = 0;
    bool m_abortada = false;
    QString m_ultimoServidor;
    QString m_ultimoMotivo = tr("sin respuesta");
    Net::Download::Ptr m_actual;
    QTimer m_espera;
};

}  // namespace detalle

Task::Ptr makeFileTask(const File& file, const QString& destPath, int intentosPorOrigen)
{
    if (destPath.isEmpty())
        return nullptr;

    const auto origenes = usableOrigins(file);
    if (origenes.isEmpty())
        return nullptr;

    return makeShared<detalle::FileTask>(file, destPath, origenes, qMax(1, intentosPorOrigen));
}

}  // namespace Luna

#include "LunaDownload.moc"
