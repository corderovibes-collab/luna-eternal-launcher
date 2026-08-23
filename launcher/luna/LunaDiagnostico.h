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

#include <QString>

namespace Luna {

/**
 * Lo que el launcher puede hacer solo ante un cierre.
 *
 * No es decoracion: decide QUE BOTON se le enseña al jugador, y un boton que no
 * arregla nada es peor que ninguno.
 */
enum class Accion {
    Ninguna,  ///< se explica y punto. No hay boton
    Reparar,  ///< comprobar fichero a fichero y volver a bajar lo que no cuadre
    Memoria,  ///< abrir Ajustes en la memoria asignada
};

/** El resultado de mirar un cierre. */
struct Diagnostico {
    QString titulo;
    QString detalle;
    Accion accion = Accion::Ninguna;

    /** Vacio = el cierre fue normal y no hay nada que contarle a nadie. */
    bool vacio() const { return titulo.isEmpty(); }
};

/**
 * Traduce un cierre del juego a una frase que un jugador pueda entender.
 *
 * ⚠⚠ CADA REGLA ES UN FALLO QUE YA HA PASADO DE VERDAD, en este proyecto o en
 *    el anterior. No se inventan sintomas: si aparece uno nuevo se añade aqui
 *    con su causa y su arreglo, y deja de costar una tarde.
 *
 *    Lo que ve el jugador cuando Minecraft se cae es una ventana que
 *    desaparece. Lo que ve quien da soporte es
 *    `java.util.zip.ZipException: ZipFile invalid LOC header` dentro de una
 *    pila de netty que NO SE PARECE EN NADA a la causa -- ese caso concreto ya
 *    costo una sesion entera aqui.
 *
 * ⚠ EL REGISTRO MANDA SOBRE EL CODIGO DE SALIDA. Un juego puede morir con
 *   codigo 1 por cualquier motivo; si el log dice «invalid LOC header», eso es
 *   lo que paso y eso es lo que hay que enseñar.
 *
 * @param codigo   codigo de salida del proceso
 * @param registro las ultimas lineas del log del juego
 */
Diagnostico diagnosticar(int codigo, const QString& registro);

/**
 * Las ultimas `maxLineas` del `logs/latest.log` de la instancia.
 *
 * ⚠ SE LEE DEL FINAL, no del principio. Un log de Minecraft son miles de lineas
 *   y la excepcion que mato al juego esta SIEMPRE al final; cargarlo entero
 *   para mirar el ultimo trozo es gastar memoria y tiempo por nada.
 *
 * Devuelve cadena vacia si no hay log, que es un caso normal: el juego pudo
 * morir antes de escribir uno.
 */
QString colaDelRegistro(const QString& gameRoot, int maxLineas = 200);

}  // namespace Luna
