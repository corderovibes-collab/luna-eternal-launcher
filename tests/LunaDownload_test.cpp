// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QDir>
#include <QTest>

#include <luna/LunaDownload.h>
#include <luna/LunaManifest.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea. Ver LunaManifest_test.cpp.
namespace {

Luna::File conOrigenes(const QStringList& urls, const QString& sha1 = QStringLiteral("aa"))
{
    Luna::File f;
    f.path = QStringLiteral("mods/x.jar");
    f.sha1 = sha1;
    f.size = 10;
    f.urls = urls;
    return f;
}

}  // namespace

class LunaDownloadTest : public QObject {
    Q_OBJECT
   private slots:

    void test_rutaSeResuelveDentroDeLaInstancia()
    {
        const QString raiz = QDir::toNativeSeparators(QStringLiteral("/tmp/instancia"));
        const auto r = Luna::resolveInInstance(raiz, QStringLiteral("mods/x.jar"));
        QVERIFY(!r.isEmpty());
        QVERIFY(r.endsWith(QStringLiteral("mods/x.jar")));
    }

    void test_rutasQueSeEscapanDevuelvenVACIO()
    {
        // ⚠ TERCERA CERRADURA contra `../`. Las otras dos son la huella del
        //   manifiesto y el filtrado del planificador; esta es la ultima antes
        //   de escribir en el disco de verdad.
        const QString raiz = QStringLiteral("/tmp/instancia");
        QVERIFY(Luna::resolveInInstance(raiz, QStringLiteral("../fuera.txt")).isEmpty());
        QVERIFY(Luna::resolveInInstance(raiz, QStringLiteral("mods/../../fuera.txt")).isEmpty());
        QVERIFY(Luna::resolveInInstance(raiz, QStringLiteral("/etc/passwd")).isEmpty());
        QVERIFY(Luna::resolveInInstance(raiz, QString()).isEmpty());
    }

    void test_origenesInutilesSeDescartan()
    {
        // Distinguir "sin origenes utiles" de "todos fallaron" importa: el
        // primero es un manifiesto mal generado y el segundo es la red.
        auto f = conOrigenes({ QStringLiteral("https://uno/x.jar"), QStringLiteral("no-es-una-url"),
                               QStringLiteral("ftp://"), QString(), QStringLiteral("https://espejo/x.jar") });
        QCOMPARE(Luna::usableOrigins(f),
                 QStringList({ QStringLiteral("https://uno/x.jar"), QStringLiteral("https://espejo/x.jar") }));
    }

    void test_seRespetaElOrdenDeLosOrigenes()
    {
        // El primero es el primario. Si se reordenara, un espejo lento pasaria
        // a ser el camino normal de todo el mundo.
        auto f = conOrigenes({ QStringLiteral("https://primario/x.jar"), QStringLiteral("https://espejo/x.jar") });
        const auto o = Luna::usableOrigins(f);
        QCOMPARE(o.first(), QStringLiteral("https://primario/x.jar"));
    }

    void test_soloUn4xxDescartaElOrigen()
    {
        // ⚠⚠ ESTA TABLA ES LA POLITICA DE REINTENTOS, Y CADA FILA RECONSTRUYE EL
        //    FALLO ORIGINAL SI SE EQUIVOCA.
        //
        // El pack no reintentaba NADA, y eso hacia fallar la instalacion
        // completa el 96 % de las veces. La correccion depende entera de acertar
        // aqui: dar un 503 por definitivo es volver a rendirse a la primera, y
        // reintentar un 404 cuatro veces es hacer esperar al jugador de balde
        // para darle al final la misma mala noticia.

        // Sin respuesta HTTP: DNS, TLS, conexion cortada. Es EL caso a
        // reintentar, porque es lo que le pasa a un wifi domestico.
        QVERIFY(!Luna::origenDescartado(0));

        // Salio bien o redirige: no descarta nada.
        QVERIFY(!Luna::origenDescartado(200));
        QVERIFY(!Luna::origenDescartado(302));

        // 4xx: el fichero no esta AHI, y preguntarlo otra vez da lo mismo.
        QVERIFY(Luna::origenDescartado(400));
        QVERIFY(Luna::origenDescartado(403));
        QVERIFY(Luna::origenDescartado(404));
        QVERIFY(Luna::origenDescartado(410));

        // Las dos excepciones del 4xx, que hablan de TIEMPO y no de contenido.
        // El 429 es exactamente lo que devolvia `raw.githubusercontent` cuando
        // servia el pack (D-036): darlo por definitivo dejaria sin pack a quien
        // instala de cero, que es como empezo todo esto.
        QVERIFY(!Luna::origenDescartado(408));
        QVERIFY(!Luna::origenDescartado(429));

        // 5xx: el servidor esta teniendo un mal rato y se le vuelve a preguntar.
        QVERIFY(!Luna::origenDescartado(500));
        QVERIFY(!Luna::origenDescartado(502));
        QVERIFY(!Luna::origenDescartado(503));
    }

    void test_unaTareaPorFicheroConSusOpciones()
    {
        auto f = conOrigenes({ QStringLiteral("https://uno/x.jar"), QStringLiteral("https://espejo/x.jar") });
        auto t = Luna::makeFileTask(f, QStringLiteral("/tmp/instancia/mods/x.jar"));
        QVERIFY(t != nullptr);
    }

    void test_sinOrigenUtilNoHayTarea()
    {
        QCOMPARE(Luna::makeFileTask(conOrigenes({}), QStringLiteral("/tmp/x")), nullptr);
        QCOMPARE(Luna::makeFileTask(conOrigenes({ QStringLiteral("no-es-una-url") }), QStringLiteral("/tmp/x")), nullptr);
    }

    void test_sinDestinoNoHayTarea()
    {
        // `resolveInInstance` devuelve vacio para rutas inseguras, y eso tiene
        // que impedir la descarga en vez de escribir en cualquier sitio.
        auto f = conOrigenes({ QStringLiteral("https://uno/x.jar") });
        QCOMPARE(Luna::makeFileTask(f, QString()), nullptr);
    }

    void test_sinHuellaSigueHabiendoTarea()
    {
        // Los ficheros `once` se descargan sin validar huella: el jugador los
        // toca y dejan de coincidir a proposito.
        auto f = conOrigenes({ QStringLiteral("https://uno/x.jar") }, QString());
        QVERIFY(Luna::makeFileTask(f, QStringLiteral("/tmp/instancia/mods/x.jar")) != nullptr);
    }
};

QTEST_GUILESS_MAIN(LunaDownloadTest)

#include "LunaDownload_test.moc"
