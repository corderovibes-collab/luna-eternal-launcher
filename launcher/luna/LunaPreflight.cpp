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

#include "luna/LunaPreflight.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>

#include "Application.h"
#include "HardwareInfo.h"
#include "net/Download.h"

namespace Luna {

namespace {

// El pack son 434 MB comprimidos; descomprimido, con Minecraft, Java y los
// assets, la instalacion completa pasa de 2,5 GB.
constexpr double kGbNecesarios = 4.0;
constexpr double kGbComodos = 8.0;

constexpr quint64 kRamMinimaMiB = 6ULL * 1024;
constexpr quint64 kRamComodaMiB = 8ULL * 1024;

// El enlace corto oficial de Microsoft para el redistribuible actual de la
// linea 2015-2022. Se prefiere a una URL con version dentro porque no caduca:
// una version fija dejaria de existir el dia que Microsoft la retire, y eso
// solo se descubriria cuando le fallara a un jugador.
const auto kUrlVcRedist = QStringLiteral("https://aka.ms/vs/17/release/vc_redist.x64.exe");

#ifdef Q_OS_WIN
// Las tres que importan. `vcruntime140_1.dll` es la que suele faltar: llego con
// Visual Studio 2019, asi que un equipo con redistribuibles antiguos tiene las
// otras dos y NO esta -- y el sintoma es identico a no tener ninguna.
const QStringList kDllsVc = { QStringLiteral("vcruntime140.dll"), QStringLiteral("vcruntime140_1.dll"),
                              QStringLiteral("msvcp140.dll") };
#endif

Comprobacion ok(QString id, QString titulo, QString detalle)
{
    return { std::move(id), Nivel::Ok, std::move(titulo), std::move(detalle), false };
}

Comprobacion aviso(QString id, QString titulo, QString detalle)
{
    return { std::move(id), Nivel::Aviso, std::move(titulo), std::move(detalle), false };
}

Comprobacion fallo(QString id, QString titulo, QString detalle, bool arreglable = false)
{
    return { std::move(id), Nivel::Error, std::move(titulo), std::move(detalle), arreglable };
}

#ifdef Q_OS_WIN
QString carpetaSistema()
{
    const auto raiz = qEnvironmentVariable("SystemRoot", QStringLiteral("C:/Windows"));
    return QDir(raiz).absoluteFilePath(QStringLiteral("System32"));
}

// Fuera de Windows no hay runtime de Visual C++ que comprobar, asi que la
// funcion existe pero no encuentra nada: quien llama no tiene que saber en
// que sistema esta.
#endif  // Q_OS_WIN

QStringList dllsQueFaltan()
{
    QStringList faltan;
#ifdef Q_OS_WIN
    const QDir sys(carpetaSistema());
    for (const auto& dll : kDllsVc) {
        if (!QFileInfo::exists(sys.absoluteFilePath(dll)))
            faltan << dll;
    }
#endif
    return faltan;
}

#ifdef Q_OS_WIN
Comprobacion comprobarVisualCpp()
{
    const auto faltan = dllsQueFaltan();
    if (faltan.isEmpty())
        return ok(QStringLiteral("vcredist"), QObject::tr("Visual C++ 2015-2022"), QObject::tr("Instalado"));

    return fallo(QStringLiteral("vcredist"), QObject::tr("Visual C++ 2015-2022"),
                 QObject::tr("Falta el runtime de Microsoft que necesita Minecraft (%1). "
                             "El launcher puede instalarlo ahora: son unos 25 MB y Windows te "
                             "pedira permiso de administrador.")
                     .arg(faltan.join(QStringLiteral(", "))),
                 /*arreglable=*/true);
}
#endif  // Q_OS_WIN

Comprobacion comprobarDisco(const QString& instanceRoot)
{
    // En la primera ejecucion la carpeta aun no existe: se sube hasta el primer
    // antepasado que si, que es donde de verdad se va a escribir.
    QString objetivo = instanceRoot;
    while (!objetivo.isEmpty() && !QFileInfo::exists(objetivo)) {
        const QString padre = QFileInfo(objetivo).absolutePath();
        if (padre == objetivo)
            break;
        objetivo = padre;
    }

    const QStorageInfo info(objetivo);
    if (!info.isValid() || !info.isReady())
        return ok(QStringLiteral("disco"), QObject::tr("Espacio en disco"), QObject::tr("No se ha podido comprobar"));

    const double libresGb = static_cast<double>(info.bytesAvailable()) / (1024.0 * 1024.0 * 1024.0);
    const QString detalle = QObject::tr("%1 GB libres en %2").arg(libresGb, 0, 'f', 1).arg(info.rootPath());

    if (libresGb < kGbNecesarios)
        return fallo(QStringLiteral("disco"), QObject::tr("Espacio en disco"),
                     QObject::tr("%1. Hacen falta al menos %2 GB para Minecraft y el pack.")
                         .arg(detalle)
                         .arg(kGbNecesarios, 0, 'f', 0));
    if (libresGb < kGbComodos)
        return aviso(QStringLiteral("disco"), QObject::tr("Espacio en disco"),
                     QObject::tr("%1. Va justo: con mundos y capturas se llena.").arg(detalle));
    return ok(QStringLiteral("disco"), QObject::tr("Espacio en disco"), detalle);
}

Comprobacion comprobarMemoria()
{
    const quint64 totalMiB = HardwareInfo::totalRamMiB();
    if (totalMiB == 0)
        return ok(QStringLiteral("ram"), QObject::tr("Memoria del equipo"), QObject::tr("No se ha podido comprobar"));

    const QString detalle = QObject::tr("%1 GB").arg(totalMiB / 1024.0, 0, 'f', 1);

    // Aviso, nunca error. Con 4-6 GB el pack va mal pero ARRANCA, y quien tiene
    // un equipo justo prefiere jugar a tirones antes que no jugar. Bloquear
    // aqui seria decidir por el.
    if (totalMiB < kRamMinimaMiB)
        return aviso(QStringLiteral("ram"), QObject::tr("Memoria del equipo"),
                     QObject::tr("%1. Es poco para este pack: va a ir a tirones. Baja la RAM asignada "
                                 "en Ajustes a 3 GB y cierra el navegador antes de jugar.")
                         .arg(detalle));
    if (totalMiB < kRamComodaMiB)
        return aviso(QStringLiteral("ram"), QObject::tr("Memoria del equipo"),
                     QObject::tr("%1. Justo: asigna 4 GB al juego y cierra lo demas.").arg(detalle));
    return ok(QStringLiteral("ram"), QObject::tr("Memoria del equipo"), detalle);
}

Comprobacion comprobarGrafica()
{
    // ⚠ LO QUE DEVUELVE `gpuInfo()` NO ES UN NOMBRE PELADO. En Windows sale de
    //   DXGI y viene con prefijo -- "GPU: AMD Radeon 780M Graphics" --, y si la
    //   consulta falla devuelve UNA LINEA DE TEXTO, no una lista vacia:
    //
    //       "GPU discovery failed: could not create DXGI factory"
    //
    //   Sin filtrar eso, esa frase se enseñaba al jugador como si fuera el
    //   modelo de su tarjeta. Se comprueba leyendo la funcion, no suponiendo.
    QStringList modelos;
    for (const auto& g : HardwareInfo::gpuInfo()) {
        if (g.contains(QStringLiteral("discovery failed"), Qt::CaseInsensitive))
            continue;
        QString m = g;
        if (m.startsWith(QStringLiteral("GPU: ")))
            m.remove(0, 5);
        if (!m.isEmpty())
            modelos << m;
    }
    if (modelos.isEmpty())
        return ok(QStringLiteral("gpu"), QObject::tr("Tarjeta grafica"), QObject::tr("No se ha podido leer"));

    const QString texto = modelos.join(QStringLiteral(", "));

    // El adaptador basico de Windows es lo que queda cuando NO hay drivers de
    // la grafica instalados: es el estado tipico de un equipo recien formateado,
    // y con el Minecraft no abre porque no hay OpenGL 3.2.
    //
    // ⚠⚠ ESTO DETECTA QUE FALTA EL DRIVER, NO QUE SEA EL BUENO. DXGI da el
    //    nombre del adaptador y nada mas: ni version del driver, ni si hay
    //    OpenGL 3.2 -- que es lo que Minecraft exige de verdad, y DXGI habla de
    //    Direct3D, no de OpenGL. Una tarjeta vieja con driver antiguo pasa esta
    //    comprobacion y luego el juego no arranca. La unica forma de salir de
    //    dudas es intentar crear un contexto OpenGL 3.2 y ver si sale; se dejo
    //    fuera a proposito porque hacerlo con un driver roto puede colgar o
    //    tumbar el launcher, y quedarse sin launcher es peor que un aviso.
    static const QRegularExpression sinDriver(
        QStringLiteral("basic (render|display)|microsoft basic|standard vga|llvmpipe|softwar"),
        QRegularExpression::CaseInsensitiveOption);
    if (sinDriver.match(texto).hasMatch())
        return aviso(QStringLiteral("gpu"), QObject::tr("Tarjeta grafica"),
                     QObject::tr("Windows esta usando el adaptador basico (%1), que casi siempre significa "
                                 "que faltan los drivers de la grafica. Instalalos desde la web de NVIDIA, "
                                 "AMD o Intel. Puedes intentar jugar igualmente: si el juego no abre, son "
                                 "los drivers.")
                         .arg(texto));

    return ok(QStringLiteral("gpu"), QObject::tr("Tarjeta grafica"), texto);
}

}  // namespace

bool faltaVisualCpp()
{
    return !dllsQueFaltan().isEmpty();
}

QList<Comprobacion> comprobarRequisitos(const QString& instanceRoot)
{
    QList<Comprobacion> lista;
#ifdef Q_OS_WIN
    lista << comprobarVisualCpp();
#endif
    lista << comprobarDisco(instanceRoot);
    lista << comprobarMemoria();
    lista << comprobarGrafica();
    return lista;
}

bool hayBloqueos(const QList<Comprobacion>& lista)
{
    for (const auto& c : lista) {
        if (c.nivel == Nivel::Error)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------

// ⚠⚠ `Task(parent)` NO HACE LO QUE PARECE, Y COMPILA IGUAL.
//
// `Task` solo tiene `explicit Task(bool show_debug_log = true)`: NO acepta un
// padre. Escribir `: Task(parent)` no da error --un puntero se convierte a
// `bool`-- sino que pasa `parent != nullptr` como "escribe log de depuracion" y
// TIRA EL PADRE POR EL CAMINO. El objeto se queda sin dueño, y el log se
// enciende o se apaga segun quien lo haya construido.
//
// (`LunaUpdate.cpp` tiene hoy esa misma linea. Ahi no hace daño porque
// `MainWindow` lo construye sin padre y dentro de un `unique_qobject_ptr`, pero
// conviene saberlo antes de fiarse de ese parametro.)
InstalarVcRedistTask::InstalarVcRedistTask(QObject* parent) : Task()
{
    setParent(parent);
}

InstalarVcRedistTask::~InstalarVcRedistTask()
{
    limpiar();
}

bool InstalarVcRedistTask::abort()
{
    m_abortada = true;
    if (m_descarga)
        m_descarga->abort();
    if (m_proceso && m_proceso->state() != QProcess::NotRunning)
        m_proceso->kill();
    return Task::abort();
}

void InstalarVcRedistTask::executeTask()
{
    setStatus(tr("Descargando el runtime de Visual C++..."));

    // ⚠ VA A LA CARPETA TEMPORAL DEL USUARIO, NO A LA DE LA INSTANCIA.
    //   `LunaSync` barre lo que no esta en el manifiesto, y un ejecutable de 25
    //   MB apareciendo y desapareciendo dentro de la instancia es justo el tipo
    //   de cosa que confunde a un antivirus.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_instalador = QDir(dir).absoluteFilePath(QStringLiteral("luna-vc_redist.x64.exe"));
    QFile::remove(m_instalador);

    auto dl = Net::Download::makeFile(QUrl(kUrlVcRedist), m_instalador);
    dl->setNetwork(APPLICATION->network());
    dl->enableAutoRetry(true);
    // ⚠ NO SE PUEDE FIJAR UN SHA1 AQUI, Y NO ES UN DESCUIDO.
    //   `aka.ms/vs/17/release/...` sirve SIEMPRE la ultima revision, asi que
    //   cualquier huella que escribieramos hoy dejaria de valer sin avisar y el
    //   fallo aparecerian meses despues, en la maquina de otro. Lo que sostiene
    //   la confianza aqui son dos cosas: la descarga va por HTTPS contra un
    //   dominio de Microsoft con el certificado validado por Qt, y el propio
    //   instalador esta firmado por Microsoft -- Windows comprueba esa firma al
    //   ejecutarlo con elevacion y se niega si no cuadra.
    m_descarga = dl;

    connect(dl.get(), &Task::succeeded, this, &InstalarVcRedistTask::instalar);
    connect(dl.get(), &Task::failed, this, [this](QString motivo) {
        emitFailed(tr("No se pudo descargar el runtime de Visual C++: %1\n\n"
                      "Puedes instalarlo a mano desde:\n%2")
                       .arg(motivo, kUrlVcRedist));
    });
    connect(dl.get(), &Task::progress, this, &Task::setProgress);
    dl->start();
}

void InstalarVcRedistTask::instalar()
{
    if (m_abortada)
        return;

    setStatus(tr("Instalando el runtime de Visual C++ (Windows pedira permiso)..."));
    setProgress(0, 0);

    m_proceso = new QProcess(this);
    connect(m_proceso, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_abortada)
            return;
        limpiar();
        emitFailed(tr("No se pudo ejecutar el instalador de Visual C++.\n\n"
                      "Instalalo a mano desde:\n%1")
                       .arg(kUrlVcRedist));
    });
    connect(m_proceso, &QProcess::finished, this, [this](int codigo, QProcess::ExitStatus) {
        if (m_abortada)
            return;
        limpiar();

        // 0 = instalado. 1638 = ya hay una version igual o mas nueva, que para
        // nosotros es exito. 3010 = instalado pero pide reiniciar; tampoco es
        // un fallo, aunque hay que decirlo.
        const bool bien = codigo == 0 || codigo == 1638 || codigo == 3010;
        if (!bien) {
            // 1602 es "el usuario cancelo", tipicamente el aviso de
            // administrador. Merece un mensaje propio porque no es un error del
            // launcher y el jugador sabe perfectamente que ha pasado.
            if (codigo == 1602) {
                emitFailed(tr("Has cancelado la instalacion del runtime de Visual C++. "
                              "Sin el, Minecraft se cierra nada mas abrir."));
                return;
            }
            emitFailed(tr("El instalador de Visual C++ termino con el codigo %1.\n\n"
                          "Instalalo a mano desde:\n%2")
                           .arg(codigo)
                           .arg(kUrlVcRedist));
            return;
        }

        // ⚠ SE VUELVE A MIRAR EL DISCO. "El instalador dijo que si" no es lo
        //   mismo que "las DLL estan": si el codigo fue 3010, Windows todavia
        //   no las ha puesto en su sitio y arrancar el juego ahora falla igual,
        //   con el jugador convencido de que ya lo habia arreglado.
        if (faltaVisualCpp()) {
            emitFailed(tr("Visual C++ se instalo, pero Windows aun no lo ve. "
                          "Reinicia el equipo y vuelve a darle a Jugar."));
            return;
        }

        emitSucceeded();
    });

    // `/passive`: barra de progreso, sin preguntas. `/norestart`: no reiniciar
    // el equipo por su cuenta y sin avisar, que con el juego a medio instalar
    // seria una sorpresa desagradable.
    m_proceso->start(m_instalador, { QStringLiteral("/install"), QStringLiteral("/passive"), QStringLiteral("/norestart") });
}

void InstalarVcRedistTask::limpiar()
{
    if (!m_instalador.isEmpty()) {
        QFile::remove(m_instalador);
        m_instalador.clear();
    }
}

}  // namespace Luna
