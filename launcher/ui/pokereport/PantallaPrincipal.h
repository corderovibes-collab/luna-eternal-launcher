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

#include <QWidget>

class QLabel;
class QPushButton;

/**
 * La pantalla que ve el jugador: perfiles a la izquierda, logo y JUGAR.
 *
 * ⚠⚠ POR QUE EXISTE, Y QUE SUSTITUYE.
 *
 *    Prism es un GESTOR MULTI-INSTANCIA: su ventana es una rejilla de
 *    instancias con una barra de acciones al lado --Lanzar, Forzar cierre,
 *    Editar, Carpeta-- y un panel de noticias. Eso sirve para alguien que
 *    administra varios modpacks.
 *
 *    Aqui hay UN servidor y UNA instancia. La rejilla enseña siempre lo mismo,
 *    la barra de acciones ofrece cuatro cosas de las que solo una se usa, y
 *    «Lanzar» es un enlace de texto de doce pixeles.
 *
 *    Esta pantalla ocupa el sitio de esa rejilla. La de Prism SIGUE EXISTIENDO
 *    --no se borra nada-- y se puede volver a ella; simplemente esta oculta.
 *
 * ⚠ NO DECIDE NADA. No sabe lanzar el juego ni cambiar de perfil: emite dos
 *   señales y `MainWindow` hace el trabajo, que es donde ya vivia. Si esta
 *   pantalla se tirara mañana, no se perderia ni una regla.
 */
class PantallaPrincipal : public QWidget {
    Q_OBJECT

   public:
    explicit PantallaPrincipal(QWidget* parent = nullptr);

    /** Marca cual de los dos perfiles esta elegido. */
    void ponerPerfil(bool constructor);

    /**
     * El pie de la tarjeta del servidor: version del pack y ficheros.
     *
     * Cadena vacia = todavia no se sabe, y entonces no se enseña nada. ⚠ Un
     * hueco vacio es mejor que un numero inventado: si pusiera «0 ficheros»
     * mientras carga, alguien lo leeria como que su instalacion esta vacia.
     */
    void ponerPack(const QString& texto);

    /**
     * El estado del servidor.
     *
     * ⚠ TRES ESTADOS Y NO DOS. «Comprobando» existe a proposito: mientras no se
     *   sepa, decir «En linea» es inventarselo y decir «Sin conexion» es una
     *   mentira que ademas asusta.
     */
    enum class Estado { Comprobando, EnLinea, SinConexion };
    void ponerEstado(Estado estado, const QString& detalle = {});

    /** Apaga JUGAR mientras el juego esta arrancando o ya corriendo. */
    void ponerJugarActivo(bool activo, const QString& etiqueta = {});

   signals:
    void jugarPulsado();
    void perfilElegido(bool constructor);

   private:
    QWidget* filaPerfil(const QString& titulo, const QString& detalle, bool constructor);
    void repintarPerfiles();

    QPushButton* m_jugar = nullptr;
    QWidget* m_filaJugador = nullptr;
    QWidget* m_filaConstructor = nullptr;
    QLabel* m_pack = nullptr;
    QLabel* m_estadoPunto = nullptr;
    QLabel* m_estadoTexto = nullptr;
    bool m_constructor = false;
};
