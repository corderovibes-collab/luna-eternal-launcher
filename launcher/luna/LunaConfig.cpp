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

#include "luna/LunaConfig.h"

namespace Luna {

namespace {
// ⚠ LA RAMA DE ESTE REPOSITORIO ES `master`, NO `main`.
//   Escribir `main` por costumbre publica bien y deja el enlace en 404: todo
//   el mundo sin pack, y el launcher sin forma de saber por que. Ya paso.
const auto kRepo = QStringLiteral("corderovibes-collab/luna-eternal-pack");
const auto kBranch = QStringLiteral("master");
}  // namespace

QString pointerUrl()
{
    return QStringLiteral("https://github.com/%1/releases/download/pack-manifest/latest.json").arg(kRepo);
}

QString fallbackManifestUrl()
{
    return QStringLiteral("https://raw.githubusercontent.com/%1/%2/manifest.json").arg(kRepo, kBranch);
}

QString defaultProfile()
{
    // Quien construye sabe que construye; quien juega no tiene por que
    // enterarse de que existe la otra opcion.
    return QStringLiteral("jugador");
}

QStringList profiles()
{
    return { QStringLiteral("jugador"), QStringLiteral("constructor") };
}

}  // namespace Luna
