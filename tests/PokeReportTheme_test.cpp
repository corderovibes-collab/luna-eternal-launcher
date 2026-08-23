// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PokeReport Network - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport Network
 */

#include <QTest>

#include <ui/themes/PokeReportTheme.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea. Ver LunaManifest_test.cpp.
class PokeReportThemeTest : public QObject {
    Q_OBJECT
   private slots:

    void initTestCase()
    {
        // ⚠⚠ SIN ESTO LA PRUEBA PASARIA SIENDO MENTIRA.
        //
        //    El .qrc se compila DENTRO de Launcher_logic, que es una libreria
        //    ESTATICA. El enlazador tira todo objeto al que nadie referencia, y
        //    el inicializador del recurso no lo referencia nadie: en el binario
        //    de prueba el recurso no existiria y `appStyleSheet()` devolveria
        //    vacio -- exactamente el fallo que esta prueba busca.
        Q_INIT_RESOURCE(pokereport);
    }

    void test_elTemaSeIdentifica()
    {
        PokeReportTheme tema;
        QCOMPARE(tema.id(), QStringLiteral("pokereport"));
        QVERIFY(!tema.name().isEmpty());
        QVERIFY(tema.hasStyleSheet());
    }

    // ⚠⚠ ESTA ES LA PRUEBA QUE IMPORTA, Y CUBRE UN FALLO MUDO.
    //
    //    `appStyleSheet()` lee la hoja del recurso Qt. Si algun dia
    //    `pokereport.qrc` se cae de `qt_add_resources()` en el CMakeLists, todo
    //    sigue COMPILANDO: simplemente el fichero no esta dentro del binario,
    //    la lectura falla y la ventana se abre con el gris de Qt por defecto.
    //
    //    Desde fuera eso se lee como "el tema no se ha aplicado", que manda a
    //    buscar el fallo en los colores en vez de en una linea del CMakeLists.
    void test_laHojaDeEstilosViajaDentroDelBinario()
    {
        PokeReportTheme tema;
        const auto qss = tema.appStyleSheet();

        QVERIFY2(!qss.isEmpty(), "la hoja no esta en el recurso: mira que pokereport.qrc siga en qt_add_resources()");
        QVERIFY(qss.size() > 2000);
    }

    // Los cinco colores de la marca salen del logo, asi que no pueden
    // desaparecer de la hoja sin que alguien lo decida.
    void test_losColoresDeLaMarcaSiguenAhi()
    {
        const auto qss = PokeReportTheme().appStyleSheet();
        for (const auto& color : { QStringLiteral("#FFC420"),    // oro
                                   QStringLiteral("#FFE04F"),    // oro claro
                                   QStringLiteral("#F86800"),    // naranja
                                   QStringLiteral("#E8189B") })  // magenta del neon
        {
            QVERIFY2(qss.contains(color), qPrintable(QStringLiteral("falta el color de marca ") + color));
        }
    }

    // ⚠ El boton por defecto --Jugar, Instalar, Aceptar-- es el UNICO con
    //   degradado, y de eso depende que en cualquier pantalla se vea de un
    //   vistazo cual es la accion que se espera.
    void test_elBotonPrincipalSigueSiendoElDegradado()
    {
        const auto qss = PokeReportTheme().appStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("QPushButton:default")));
        QVERIFY(qss.contains(QStringLiteral("qlineargradient")));
    }

    // ⚠ La seleccion es MAGENTA y el hover es gris, y estan separados a
    //   proposito: con el mismo color, pasar el raton por una lista parece que
    //   va cambiando lo que tienes elegido.
    void test_seleccionYhoverNoSeConfunden()
    {
        const auto qss = PokeReportTheme().appStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("QListView::item:selected")));
        QVERIFY(qss.contains(QStringLiteral("QListView::item:hover")));

        const auto seleccion = qss.mid(qss.indexOf(QStringLiteral("QListView::item:selected")), 220);
        QVERIFY2(seleccion.contains(QStringLiteral("E8189B")), "la seleccion de lista ha dejado de ser el magenta de la ball");
    }
};

QTEST_GUILESS_MAIN(PokeReportThemeTest)

#include "PokeReportTheme_test.moc"
