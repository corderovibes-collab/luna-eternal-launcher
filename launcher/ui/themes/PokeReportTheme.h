// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PokeReport Network - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport Network
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
 */
#pragma once

#include "FusionTheme.h"

/**
 * El tema de la casa.
 *
 * ⚠⚠ SUS COLORES SALEN DEL LOGO, no de un gusto. Se muestrearon los dos PNG de
 *    la marca --la Poke Ball en llamas y el rotulo POKEREPORT NETWORK-- y de
 *    ahi vienen los cinco que manda todo: oro #FFC420, oro claro #FFE04F,
 *    naranja #F86800, granate #5C1210 y el magenta #E8189B del neon del boton
 *    de la ball. El porque de cada uno esta en la cabecera del .qss.
 *
 * ⚠ LA HOJA DE ESTILOS ES UN FICHERO, no una cadena dentro de este .cpp.
 *   `FreesmTheme::appStyleSheet()` devuelve un literal concatenado en ~40
 *   trozos: no se puede leer, y en un commit el cambio de un color aparece como
 *   una linea de 4.000 caracteres. Aqui vive en `:/pokereport/pokereport.qss`,
 *   dentro del recurso Qt, y se lee al aplicar el tema.
 */
/**
 * Mete `pokereport.qrc` en el sistema de recursos de Qt.
 *
 * ⚠⚠ HAY QUE LLAMARLA A MANO, Y NO SIRVE HACERLO EN `main.cpp`.
 *
 *    Los demas recursos del launcher se inicializan en `main.cpp`, pero eso
 *    ocurre DESPUES de construir `Application` -- y el tema se aplica DENTRO de
 *    ese constructor. Puesto alli, la hoja de estilos llegaria tarde: el
 *    launcher arranca, aplica el tema, no encuentra el fichero, y se queda con
 *    el gris de Qt. Pasó exactamente eso la primera vez.
 *
 *    Es idempotente, asi que llamarla de mas no cuesta nada. La llaman quien
 *    lee la hoja y quien carga el rotulo de la barra.
 */
void iniciarRecursoPokeReport();

class PokeReportTheme : public FusionTheme {
   public:
    virtual ~PokeReportTheme() {}

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
};
