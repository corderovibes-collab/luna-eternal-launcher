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

#include <QList>
#include <QString>

#include "tasks/Task.h"

class QProcess;

namespace Luna {

/**
 * Requisitos del EQUIPO, no del pack.
 *
 * El launcher se trae Java, Minecraft, Fabric y 159 ficheros de mods. Lo que
 * NO puede traerse es lo que vive en el sistema operativo, y ahi estan las
 * causas habituales de "a mi no me abre" en un Windows recien formateado:
 *
 *   - el runtime de Visual C++, que necesitan los `natives` de LWJGL
 *   - drivers de grafica de verdad (Minecraft quiere OpenGL 3.2)
 *   - sitio en disco y memoria
 *
 * ⚠ ESTO YA EXISTIA Y SE PERDIO AL CAMBIAR DE LAUNCHER. El de Electron lo
 *   tenia en `core/preflight.js`, con la instalacion del redistribuible
 *   incluida; el fork de Qt nacio sin ello, y con el fork es con lo que juega
 *   la gente hoy. No es una idea nueva: es una funcionalidad que hay que
 *   devolver.
 *
 * ⚠ AVISAR NO ES BLOQUEAR. Solo `Error` impide jugar. Un equipo con 6 GB de
 *   RAM va a tirones, y esa es una decision del jugador, no nuestra: quien
 *   tiene un equipo justo prefiere jugar mal antes que no jugar.
 */
enum class Nivel { Ok, Aviso, Error };

struct Comprobacion {
    QString id;
    Nivel nivel = Nivel::Ok;
    QString titulo;
    QString detalle;

    /**
     * `true` si esta comprobacion se arregla sola con
     * `InstalarVcRedistTask`. Es lo que separa "te falta algo, apantate" de
     * "te falta algo, dale a Aceptar" -- que es el objetivo entero de esto.
     */
    bool arreglable = false;
};

/**
 * Mira el equipo. NO toca nada, NO pide red y NO tarda: se puede llamar en el
 * hilo de la interfaz antes de cada partida.
 *
 * `instanceRoot` solo se usa para saber en que unidad medir el espacio libre.
 */
QList<Comprobacion> comprobarRequisitos(const QString& instanceRoot);

/** Hay algo de nivel `Error` en la lista? */
bool hayBloqueos(const QList<Comprobacion>& lista);

/**
 * Falta en el SISTEMA el runtime de Visual C++ 2015-2022 (x64).
 *
 * ⚠ NO CONFUNDIR CON LAS DLL QUE VIAJAN JUNTO AL .exe. Esas hacen que arranque
 *   el LAUNCHER (ver el bloque `InstallRequiredSystemLibraries` en
 *   `launcher/CMakeLists.txt`). Esta comprobacion es para el JUEGO: los
 *   `natives` de LWJGL los carga la JVM desde la carpeta de la instancia, y a
 *   esos no les sirve una copia que este junto a nuestro ejecutable. Si falta,
 *   Minecraft se cierra nada mas abrir y el log no lo explica.
 */
bool faltaVisualCpp();

/**
 * Baja el redistribuible OFICIAL de Microsoft y lo instala sin preguntar nada
 * mas.
 *
 * ⚠ NO ES SILENCIOSO DEL TODO, Y ES CORRECTO QUE NO LO SEA: instalar en el
 *   sistema requiere permisos de administrador, asi que Windows va a enseñar
 *   su aviso. Se usa `/passive` --barra de progreso, sin preguntas-- en vez de
 *   `/quiet`, para que el jugador vea que algo esta pasando en lugar de mirar
 *   una ventana congelada.
 */
class InstalarVcRedistTask final : public Task {
    Q_OBJECT
   public:
    explicit InstalarVcRedistTask(QObject* parent = nullptr);
    ~InstalarVcRedistTask() override;

    bool canAbort() const override { return true; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void instalar();
    void limpiar();

    QString m_instalador;
    Task::Ptr m_descarga;
    QProcess* m_proceso = nullptr;
    bool m_abortada = false;
};

}  // namespace Luna
