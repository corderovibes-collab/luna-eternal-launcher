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

#include <QByteArray>

#include "luna/LunaManifest.h"
#include "tasks/Task.h"

namespace Luna {

/**
 * Trae el manifiesto vigente. Es lo que enchufa el motor a la red.
 *
 * COMO
 *
 *   1. pide el PUNTERO (~250 bytes, CDN de descargas)
 *   2. sigue hasta el manifiesto que señala, probando sus espejos
 *   3. COMPRUEBA SU HUELLA antes de interpretarlo
 *
 * Si lo que llega en el paso 1 resulta ser ya un manifiesto entero --porque
 * alguien apunte el launcher a la direccion vieja-- se acepta tal cual. Se
 * distingue POR EL CONTENIDO, que es lo unico que no depende de que nadie haya
 * migrado su configuracion.
 *
 * Si el puntero no contesta, se cae al respaldo en `raw`.
 */
class FetchManifestTask : public Task {
    Q_OBJECT
   public:
    explicit FetchManifestTask(QObject* parent = nullptr);
    ~FetchManifestTask() override = default;

    /** Valido solo si la tarea termino bien. */
    const Manifest& manifest() const { return m_manifest; }

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void pedir(const QStringList& urls, bool esPuntero);
    void recibido(bool esPuntero);

    Manifest m_manifest;
    Pointer m_pointer;

    Task::Ptr m_sub;
    QByteArray* m_buffer = nullptr;
    bool m_yaCaiAlRespaldo = false;
};

}  // namespace Luna
