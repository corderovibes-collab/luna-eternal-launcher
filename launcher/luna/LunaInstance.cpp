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

#include "luna/LunaInstance.h"

#include <QDebug>
#include <memory>

#include "InstanceList.h"
#include "minecraft/VanillaInstanceCreationTask.h"

namespace Luna {

QString instanceName()
{
    return QStringLiteral("PokeReport Network");
}

QStringList instanceNamesAntiguos()
{
    // Se añade AL FINAL segun vaya cambiando el nombre, y no se quita nunca
    // ninguno: quien lleve meses sin abrir el launcher tiene el nombre de
    // entonces en el disco.
    return { QStringLiteral("Luna Eternal") };
}

QString fabricUid()
{
    // El identificador del componente, no el nombre del mod. Lo fija el
    // formato de Prism y no depende de nuestro manifiesto.
    return QStringLiteral("net.fabricmc.fabric-loader");
}

Versions versionsFor(const Manifest& manifest)
{
    Versions v;
    v.minecraft = manifest.minecraft;
    if (!manifest.fabricLoader.isEmpty()) {
        v.loaderUid = fabricUid();
        v.loaderVersion = manifest.fabricLoader;
    }
    return v;
}

InstanceTask* makeCreationTask(const Manifest& manifest)
{
    const auto v = versionsFor(manifest);
    if (!v.isValid())
        return nullptr;

    auto mc = std::make_shared<PlainVersion>(v.minecraft);

    VanillaCreationTask* task = nullptr;
    if (v.hasLoader()) {
        auto loader = std::make_shared<PlainVersion>(v.loaderVersion);
        task = new VanillaCreationTask(mc, v.loaderUid, loader);
    } else {
        task = new VanillaCreationTask(mc);
    }

    task->setName(instanceName());
    // Grupo vacio: no hay carpetas de instancias que organizar cuando solo hay
    // una. Ponerle grupo solo añadiria un desplegable que no lleva a ningun
    // sitio.
    task->setGroup(QString());
    return task;
}

BaseInstance* findInstance(InstanceList* list)
{
    if (!list)
        return nullptr;

    // Primero por el nombre de ahora, que es el caso normal.
    for (int i = 0; i < list->count(); ++i) {
        auto* inst = list->at(i);
        if (inst && inst->name() == instanceName())
            return inst;
    }

    // ⚠⚠ Y SOLO SI NO APARECE, por los nombres viejos -- y se RENOMBRA.
    //
    //    Sin esto, renombrar el servidor le crearia una SEGUNDA instancia a
    //    todo el que ya lo tenga instalado: otros 450 MB, y su instancia de
    //    siempre --con sus partidas y sus ajustes-- ahi al lado, con pinta de
    //    haberse perdido.
    //
    //    Renombrar en vez de solo devolverla hace que la migracion pase UNA
    //    vez: al siguiente arranque ya la encuentra por el bucle de arriba.
    const auto antiguos = instanceNamesAntiguos();
    for (int i = 0; i < list->count(); ++i) {
        auto* inst = list->at(i);
        if (inst && antiguos.contains(inst->name())) {
            qInfo() << "Instancia" << inst->name() << "renombrada a" << instanceName();
            inst->setName(instanceName());
            return inst;
        }
    }

    return nullptr;
}

}  // namespace Luna
