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
#include "PokeReportTheme.h"

#include <QDebug>
#include <QFile>
#include <QObject>

namespace {
// Los cinco de la marca, muestreados del logo. Se repiten en el .qss porque
// una hoja de estilos de Qt no tiene variables; si aqui cambia uno, alli
// tambien.
const QColor ORO(0xFF, 0xC4, 0x20);
const QColor MAGENTA(0xE8, 0x18, 0x9B);

// Y las superficies. Grises CALIDOS: sobre un gris azulado el oro vira a
// mostaza y el logo deja de pegar con su propia interfaz.
const QColor FONDO(0x0D, 0x0B, 0x0C);
const QColor SUPERFICIE(0x17, 0x14, 0x16);
const QColor ELEVADA(0x21, 0x1D, 0x20);
const QColor TEXTO(0xF3, 0xEE, 0xEA);
const QColor TEXTO_SUAVE(0xA7, 0x9F, 0xA4);
const QColor TEXTO_APAGADO(0x6B, 0x64, 0x69);
}  // namespace

void iniciarRecursoPokeReport()
{
    // ⚠ `Launcher_logic` es una libreria ESTATICA. El enlazador tira todo
    //   objeto al que nadie referencia, y al inicializador que genera `rcc` no
    //   lo referencia nadie: sin esta llamada el recurso sencillamente no esta
    //   en el binario, y no hay ningun error de compilacion ni de enlazado.
    Q_INIT_RESOURCE(pokereport);
}

QString PokeReportTheme::id()
{
    return "pokereport";
}

QString PokeReportTheme::name()
{
    return QObject::tr("PokeReport Network");
}

QString PokeReportTheme::tooltip()
{
    return QObject::tr("El tema de la casa: oro, naranja y el neon de la Poke Ball.");
}

QPalette PokeReportTheme::colorScheme()
{
    QPalette p;
    p.setColor(QPalette::Window, FONDO);
    p.setColor(QPalette::WindowText, TEXTO);
    p.setColor(QPalette::Base, SUPERFICIE);
    p.setColor(QPalette::AlternateBase, ELEVADA);
    p.setColor(QPalette::ToolTipBase, ELEVADA);
    p.setColor(QPalette::ToolTipText, TEXTO);
    p.setColor(QPalette::Text, TEXTO);
    p.setColor(QPalette::Button, ELEVADA);
    p.setColor(QPalette::ButtonText, TEXTO);
    p.setColor(QPalette::BrightText, ORO);
    p.setColor(QPalette::Link, ORO);
    p.setColor(QPalette::LinkVisited, ORO.darker(120));

    // ⚠ EL RESALTE ES EL MAGENTA, NO EL ORO, y no es indiferente: el oro es el
    //   color de "esto se pulsa" y el magenta el de "esto es lo que tienes
    //   elegido". Con el mismo color para las dos cosas, pasar el raton por una
    //   lista parece que va cambiando la seleccion.
    p.setColor(QPalette::Highlight, MAGENTA);
    p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));

    p.setColor(QPalette::PlaceholderText, TEXTO_APAGADO);
    return fadeInactive(p, fadeAmount(), fadeColor());
}

double PokeReportTheme::fadeAmount()
{
    return 0.5;
}

QColor PokeReportTheme::fadeColor()
{
    return TEXTO_SUAVE;
}

bool PokeReportTheme::hasStyleSheet()
{
    return true;
}

QString PokeReportTheme::appStyleSheet()
{
    iniciarRecursoPokeReport();

    QFile hoja(QStringLiteral(":/pokereport/pokereport.qss"));

    // ⚠ SI EL RECURSO NO ESTA, SE DICE. Devolver una cadena vacia en silencio
    //   deja la ventana con el estilo de Qt por defecto --gris de sistema-- y
    //   eso se lee como "el tema no se ha aplicado", que manda a buscar el
    //   fallo en los colores en vez de en el .qrc que falta en el CMakeLists.
    if (!hoja.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "PokeReportTheme: no se pudo abrir :/pokereport/pokereport.qss --" << hoja.errorString()
                   << "-- la ventana se quedara con el estilo por defecto. Comprueba que pokereport.qrc esta en qt_add_resources().";
        return {};
    }

    return QString::fromUtf8(hoja.readAll());
}
