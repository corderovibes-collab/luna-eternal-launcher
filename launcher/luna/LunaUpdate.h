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

#include <QString>

#include "luna/LunaManifest.h"
#include "luna/LunaSync.h"
#include "tasks/Task.h"

namespace Luna {

/**
 * Deja la instancia igual que el manifiesto. Es LA tarea del launcher.
 *
 * Junta las seis piezas del motor en el orden correcto:
 *
 *   1. traer el manifiesto        (LunaFetch: puntero, espejos, huella)
 *   2. leer el estado guardado    (que se instalo la ultima vez)
 *   3. decidir                    (LunaSync: que bajar, que borrar)
 *   4. borrar lo retirado         (LunaApply)
 *   5. descargar                  (LunaDownload: espejos, huella por origen)
 *   6. extraer los zips           (LunaApply: `once`, `keepExisting`, ocultos)
 *   7. guardar el estado          <- SOLO SI TODO LO ANTERIOR SALIO BIEN
 *
 * ⚠ EL ORDEN DEL PASO 7 NO ES NEGOCIABLE.
 *
 * El estado dice "esto es lo que hay instalado". Si se guardara antes de
 * terminar, una descarga cortada a la mitad dejaria anotado como instalado
 * algo que no esta -- y el arranque siguiente se fiaria del atajo y no lo
 * volveria a bajar NUNCA. El jugador se quedaria con un pack incompleto que el
 * launcher cree completo, que es el peor estado posible: no da error y no se
 * arregla solo.
 *
 * ⚠ Y LOS BORRADOS VAN ANTES QUE LAS DESCARGAS (paso 4 antes que 5).
 *
 * Al reves, quitar un mod y añadir su sustituto en la misma publicacion
 * dejaria los dos conviviendo un rato. Con mods de bloques eso es
 * "Registry remapping failed" en la cara del jugador.
 */
class UpdateTask : public Task {
    Q_OBJECT
   public:
    UpdateTask(QString instanceRoot, QString profile, Mode mode, QObject* parent = nullptr);
    ~UpdateTask() override = default;

    bool abort() override;

    /** Valido cuando la tarea termina bien. */
    const Manifest& manifest() const { return m_manifest; }
    int downloaded() const { return m_downloaded; }
    int removed() const { return m_removed; }

    /** Donde se guarda el estado de esta instancia. */
    static QString statePath(const QString& instanceRoot);

   protected:
    void executeTask() override;

   private:
    void conElManifiesto();
    void descargar();
    void terminar();

    QString m_instanceRoot;
    QString m_profile;
    Mode m_mode;

    Manifest m_manifest;
    Plan m_plan;
    Task::Ptr m_sub;

    int m_downloaded = 0;
    int m_removed = 0;
};

}  // namespace Luna
