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

#include "BaseVersion.h"
#include "luna/LunaManifest.h"

class InstanceTask;

namespace Luna {

/** El nombre que ve el jugador. Hay UNA instancia y es esta. */
QString instanceName();

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
 * No comprueba si ya existe: de eso se encarga quien la llama.
 */
InstanceTask* makeCreationTask(const Manifest& manifest);

}  // namespace Luna
