// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QTest>
#include <QUrl>

#include <luna/LunaConfig.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea. Ver LunaManifest_test.cpp.
class LunaConfigTest : public QObject {
    Q_OBJECT
   private slots:

    void test_lasUrlSonValidas()
    {
        for (const auto& u : { Luna::pointerUrl(), Luna::fallbackManifestUrl() }) {
            const QUrl url(u);
            QVERIFY2(url.isValid(), qPrintable(u));
            QCOMPARE(url.scheme(), QStringLiteral("https"));
            QVERIFY(!url.host().isEmpty());
        }
    }

    void test_elPunteroSaleDelCDNyNoDeRaw()
    {
        // ⚠ `raw.githubusercontent` limita por peticiones y cachea unos 3
        //   minutos POR RUTA. Esa cache costo un incidente real: se publico el
        //   pack, un jugador sincronizo UN MINUTO despues y se llevo el
        //   manifiesto anterior sin enterarse. El puntero NO puede volver ahi.
        QVERIFY(!Luna::pointerUrl().contains(QStringLiteral("raw.githubusercontent")));
        QVERIFY(Luna::pointerUrl().contains(QStringLiteral("releases/download")));
    }

    void test_elRespaldoSiEsRaw()
    {
        // Es la direccion que leian los launchers 1.0.x. Se mantiene a
        // proposito: mejor lenta que dejar a alguien sin pack.
        QVERIFY(Luna::fallbackManifestUrl().contains(QStringLiteral("raw.githubusercontent")));
    }

    void test_laRamaEsMasterNoMain()
    {
        // ⚠ Escribir `main` por costumbre publica bien y deja el enlace en
        //   404: todo el mundo sin pack, y el launcher sin forma de saber por
        //   que. Ya paso una vez.
        QVERIFY(Luna::fallbackManifestUrl().contains(QStringLiteral("/master/")));
        QVERIFY(!Luna::fallbackManifestUrl().contains(QStringLiteral("/main/")));
    }

    void test_perfiles()
    {
        QCOMPARE(Luna::defaultProfile(), QStringLiteral("jugador"));
        QVERIFY(Luna::profiles().contains(Luna::defaultProfile()));
        QVERIFY(Luna::profiles().contains(QStringLiteral("constructor")));
        QCOMPARE(Luna::profiles().size(), 2);
    }
};

QTEST_GUILESS_MAIN(LunaConfigTest)

#include "LunaConfig_test.moc"
