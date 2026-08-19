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

/**
 * DONDE VIVE EL PACK. Un solo sitio.
 *
 * Todo lo que apunta al servidor de Luna Eternal esta aqui y en ningun otro
 * lado. Si algun dia cambia el repositorio, se toca este fichero y ya.
 */
namespace Luna {

/**
 * El PUNTERO: ~250 bytes que dicen cual es el manifiesto vigente.
 *
 * Sale de la release de GitHub --el CDN de descargas-- y NO de
 * `raw.githubusercontent`, que limita por peticiones y cachea unos 3 minutos
 * por ruta. Esa cache costo un incidente: el pack se publico, un jugador
 * sincronizo UN MINUTO despues y se llevo el manifiesto anterior sin
 * enterarse.
 */
QString pointerUrl();

/**
 * El manifiesto completo en `raw`, como RESPALDO.
 *
 * Es la direccion que leian los launchers 1.0.x. Se mantiene por si el puntero
 * no contesta, pero no es el camino normal: tiene la cache de 3 minutos y el
 * limite por peticiones.
 */
QString fallbackManifestUrl();

/** Perfiles posibles. El de constructor añade Axiom y WorldEdit CUI. */
QString defaultProfile();
QStringList profiles();

/**
 * El perfil elegido, guardado en los ajustes del launcher.
 *
 * ⚠ CAMBIAR DE PERFIL INSTALA O DESINSTALA, no solo marca una casilla. El
 *   planificador retira lo que ya no toca, asi que pasar de constructor a
 *   jugador quita Axiom de verdad. Sin eso, quien probara el perfil de
 *   constructor una vez se quedaba sus 46 MB para siempre.
 */
QString currentProfile();
void setCurrentProfile(const QString& profile);

/** ¿Este perfil da acceso a las herramientas de construccion? */
bool isBuilder(const QString& profile);

}  // namespace Luna
