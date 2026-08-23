// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PokeReport Network - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport Network
 */

#include <QTemporaryDir>
#include <QTest>

#include <luna/LunaDiagnostico.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea. Ver LunaManifest_test.cpp.
class LunaDiagnosticoTest : public QObject {
    Q_OBJECT
   private slots:

    void test_unaSalidaLimpiaNoInventaUnProblema()
    {
        QVERIFY(Luna::diagnosticar(0, QStringLiteral("todo bien")).vacio());
        QVERIFY(Luna::diagnosticar(0, QString()).vacio());
    }

    // El caso que costo una sesion entera: la excepcion sale dentro de una pila
    // de netty que no se parece en nada a la causa.
    void test_reconoceUnJarCorruptoYOfreceReparar()
    {
        const auto d = Luna::diagnosticar(1, QStringLiteral("java.util.zip.ZipException: ZipFile invalid LOC header"));
        QVERIFY(!d.vacio());
        QVERIFY(d.titulo.contains(QStringLiteral("corrupto")));
        QCOMPARE(d.accion, Luna::Accion::Reparar);
    }

    // ⚠⚠ ESTA ES LA PRUEBA QUE MAS VALE DE TODAS, Y CUBRE UN FALSO POSITIVO.
    //
    //    El patron de la grafica llevaba `OpenGL 3.2` a secas, y Minecraft
    //    escribe esa cadena en una linea de EXITO. O sea: saltaba en CADA
    //    arranque correcto.
    //
    //    Un diagnostico que se equivoca siempre es PEOR que no tenerlo: enseña
    //    a la gente a ignorar los avisos, y el dia que haya uno de verdad
    //    tampoco lo van a leer.
    void test_laLineaNormalDeLaGpuNoDisparaNingunaAlarma()
    {
        const auto log = QStringLiteral(
            "[19:36:10] [Render thread/INFO]: GPU: Intel(R) UHD Graphics 620 (Supports OpenGL 3.2.0 - Build 31.0.101.2135)");
        QVERIFY2(Luna::diagnosticar(0, log).vacio(), "la linea de exito de la GPU no puede dar un aviso");
    }

    void test_siReconoceUnaGraficaQueDeVerdadNoLlega()
    {
        for (const auto& log : { QStringLiteral("GLFW error 65543: WGL: Driver does not support OpenGL version 3.2"),
                                 QStringLiteral("Failed to create window"),
                                 QStringLiteral("Pixel format not accelerated") }) {
            const auto d = Luna::diagnosticar(1, log);
            QVERIFY2(!d.vacio(), qPrintable(log));
            QCOMPARE(d.accion, Luna::Accion::Ninguna);  // esto no lo arregla reparar
        }
    }

    void test_reconoceQuedarseSinMemoriaYMandaAAjustes()
    {
        const auto d = Luna::diagnosticar(1, QStringLiteral("java.lang.OutOfMemoryError: Java heap space"));
        QCOMPARE(d.accion, Luna::Accion::Memoria);
    }

    void test_reconoceModsQueNoEncajan()
    {
        const auto d = Luna::diagnosticar(1, QStringLiteral("Incompatible mods found! net.fabricmc.loader.impl.FormattedException"));
        QCOMPARE(d.accion, Luna::Accion::Reparar);
    }

    // ⚠ El log manda sobre el codigo: un juego puede morir con cualquier codigo,
    //   pero si el log dice la causa, esa es la que se enseña.
    void test_elRegistroMandaSobreElCodigoDeSalida()
    {
        const auto d = Luna::diagnosticar(-1073741819, QStringLiteral("java.lang.OutOfMemoryError: Java heap space"));
        QCOMPARE(d.accion, Luna::Accion::Memoria);
        QVERIFY(!d.titulo.contains(QStringLiteral("violacion")));
    }

    // ⚠ Estos dos NO dejan ni una linea en el log: el proceso muere de golpe.
    //   Si no se miraran los codigos, el jugador se quedaria sin explicacion.
    void test_losCierresDeGolpeDeWindowsSeReconocenSinLog()
    {
        QVERIFY(!Luna::diagnosticar(-1073741819, QString()).vacio());
        QCOMPARE(Luna::diagnosticar(-1073740791, QString()).accion, Luna::Accion::Reparar);
    }

    void test_unCierreDesconocidoExplicaQueNoSeSabeEnVezDeCallar()
    {
        const auto d = Luna::diagnosticar(42, QStringLiteral("nada reconocible por aqui"));
        QVERIFY(!d.vacio());
        QVERIFY2(d.titulo.contains(QStringLiteral("42")), "el codigo tiene que salir, es lo unico que se sabe");
        QCOMPARE(d.accion, Luna::Accion::Ninguna);
    }

    void test_sinLogNoRevienta()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QCOMPARE(Luna::colaDelRegistro(dir.path()), QString());
        QCOMPARE(Luna::colaDelRegistro(QStringLiteral("Z:/no/existe/en/ningun/sitio")), QString());
    }

    // ⚠ Se lee del FINAL. Un log de Minecraft son miles de lineas y la
    //   excepcion esta siempre al final; leer del principio traeria el arranque
    //   y se perderia justo lo que importa.
    void test_elRegistroSeLeeDelFinal()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("logs")));

        QFile log(QDir(dir.path()).filePath(QStringLiteral("logs/latest.log")));
        QVERIFY(log.open(QIODevice::WriteOnly | QIODevice::Text));
        for (int i = 0; i < 500; ++i) {
            log.write(QStringLiteral("linea de arranque %1\n").arg(i).toUtf8());
        }
        log.write("java.util.zip.ZipException: ZipFile invalid LOC header\n");
        log.close();

        const auto cola = Luna::colaDelRegistro(dir.path(), 10);
        QVERIFY2(cola.contains(QStringLiteral("invalid LOC header")), "la excepcion del final tiene que estar");
        QVERIFY2(!cola.contains(QStringLiteral("linea de arranque 0\n")), "el principio NO tiene que estar");
        QCOMPARE(Luna::diagnosticar(1, cola).accion, Luna::Accion::Reparar);
    }
};

QTEST_GUILESS_MAIN(LunaDiagnosticoTest)

#include "LunaDiagnostico_test.moc"
