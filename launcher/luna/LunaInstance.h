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
#include <QStringList>

#include "BaseVersion.h"
#include "luna/LunaManifest.h"

class InstanceTask;
class InstanceList;
class BaseInstance;

namespace Luna {

/** El nombre que ve el jugador. Hay UNA instancia y es esta. */
QString instanceName();

/**
 * Los nombres que tuvo antes.
 *
 * ⚠⚠ ESTO NO ES NOSTALGIA, ES LO QUE EVITA UNA SEGUNDA DESCARGA DE 450 MB.
 *
 *    `findInstance()` busca la instancia POR NOMBRE. Al renombrar el servidor
 *    a «PokeReport Network» (MARCA-001), la instancia que la gente ya tiene en
 *    el disco se sigue llamando «Luna Eternal»: sin esta lista no se
 *    encontraria, el launcher creeria que no hay ninguna y **crearia una
 *    segunda al lado** -- otro pack entero, y las partidas y la configuracion
 *    del jugador aparentemente perdidas en la primera.
 *
 *    Por eso `findInstance()` reconoce los nombres viejos y RENOMBRA la
 *    instancia que encuentra, en vez de limitarse a devolverla: asi la
 *    migracion ocurre una sola vez y sola.
 */
QStringList instanceNamesAntiguos();

/** Identificador del cargador dentro del sistema de componentes. */
QString fabricUid();

/**
 * Una version que es solo su cadena.
 *
 * El sistema de componentes solo pide `descriptor()`, y resuelve de verdad al
 * arrancar el juego. Envolver la cadena que ya trae el manifiesto evita tener
 * que cargar las listas de versiones POR RED antes de poder crear la
 * instancia -- que ademas seria pedirle a un servidor de metadatos algo que
 * nuestro propio manifiesto ya nos ha dicho.
 */
class PlainVersion final : public BaseVersion {
   public:
    explicit PlainVersion(QString descriptor) : m_descriptor(std::move(descriptor)) {}
    QString descriptor() const override { return m_descriptor; }
    QString name() const override { return m_descriptor; }
    QString typeString() const override { return {}; }

   private:
    QString m_descriptor;
};

/** Lo que hay que instalar, sacado del manifiesto y no escrito a mano. */
struct Versions {
    QString minecraft;
    QString loaderUid;
    QString loaderVersion;

    /**
     * Minecraft es obligatorio; el cargador no.
     *
     * Un pack sin Fabric es raro pero valido. Uno sin version de Minecraft no
     * se puede crear, y es mejor decirlo aqui que dejar una instancia a medias.
     */
    bool isValid() const { return !minecraft.isEmpty(); }
    bool hasLoader() const { return !loaderVersion.isEmpty(); }
};

/**
 * Las versiones que pide este manifiesto.
 *
 * ⚠ NO SE ESCRIBEN A MANO EN NINGUN SITIO. Ya costo una vez: la version del
 *   cargador estaba puesta a mano, caduco en silencio y el pack generado no
 *   arrancaba. El manifiesto es la unica fuente.
 */
Versions versionsFor(const Manifest& manifest);

/**
 * Tarea que crea la instancia del servidor. `nullptr` si el manifiesto no
 * trae version de Minecraft.
 *
 * No comprueba si ya existe: de eso se encarga `findInstance()`.
 */
InstanceTask* makeCreationTask(const Manifest& manifest);

/**
 * LA instancia del servidor, o `nullptr` si todavia no existe.
 *
 * Se busca POR NOMBRE y no se guarda su identificador en los ajustes. Un
 * identificador guardado se queda apuntando a la nada en cuanto alguien borra
 * la instancia desde el gestor, y entonces el launcher no la encuentra Y
 * tampoco la crea, porque cree que ya existe. Buscar por nombre siempre dice la
 * verdad de lo que hay.
 *
 * Cuando exista el modo quiosco no habra gestor y esto sera una formalidad;
 * mientras tanto, el jugador puede tener otras instancias al lado.
 */
BaseInstance* findInstance(InstanceList* list);

}  // namespace Luna
