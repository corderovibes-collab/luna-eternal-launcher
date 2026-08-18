// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QCryptographicHash>
#include <QTest>

#include <luna/LunaManifest.h>

namespace {

QByteArray manifiestoMinimo()
{
    // ⚠ NADA DE LITERALES EN BRUTO EN ESTE FICHERO.
    //   `moc` no los parsea: deja de ver la clase con Q_OBJECT, no genera el
    //   metaobjeto, y el enlazado muere con tres simbolos sin resolver que no
    //   mencionan la causa por ningun lado. Aviso: "No relevant classes found".
    return QByteArray(
        "{"
        "  \"packVersion\": \"0.2.0\","
        "  \"minecraft\": \"1.21.1\","
        "  \"fabricLoader\": \"0.19.3\","
        "  \"server\": {"
        "    \"name\": \"PokeReport\","
        "    \"host\": \"s12.mia.us.tarohosting.lat\","
        "    \"port\": 33043"
        "  },"
        "  \"files\": ["
        "    {"
        "      \"path\": \"mods/cobblemon.jar\","
        "      \"sha1\": \"aa\","
        "      \"size\": 10,"
        "      \"url\": \"https://uno/cobblemon.jar\","
        "      \"urls\": ["
        "        \"https://uno/cobblemon.jar\","
        "        \"https://espejo/cobblemon.jar\""
        "      ]"
        "    },"
        "    {"
        "      \"path\": \"mods/axiom.jar\","
        "      \"sha1\": \"bb\","
        "      \"size\": 20,"
        "      \"url\": \"https://uno/axiom.jar\","
        "      \"profiles\": ["
        "        \"constructor\""
        "      ]"
        "    },"
        "    {"
        "      \"path\": \"config\","
        "      \"sha1\": \"cc\","
        "      \"size\": 30,"
        "      \"url\": \"https://uno/config.zip\","
        "      \"archive\": true,"
        "      \"keepExisting\": true,"
        "      \"strip\": 1"
        "    },"
        "    {"
        "      \"path\": \"servers.dat\","
        "      \"sha1\": \"dd\","
        "      \"size\": 40,"
        "      \"url\": \"https://uno/servers.dat\","
        "      \"once\": true"
        "    }"
        "  ]"
        "}");
}

QString sha1De(const QByteArray& b)
{
    return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha1).toHex());
}

}  // namespace

class LunaManifestTest : public QObject {
    Q_OBJECT
   private slots:

    void test_leeLoBasico()
    {
        QString err;
        auto m = Luna::parseManifest(manifiestoMinimo(), QString(), &err);
        QVERIFY2(m.isValid(), qPrintable(err));
        QCOMPARE(m.minecraft, QStringLiteral("1.21.1"));
        QCOMPARE(m.fabricLoader, QStringLiteral("0.19.3"));
        QCOMPARE(m.serverHost, QStringLiteral("s12.mia.us.tarohosting.lat"));
        QCOMPARE(m.serverPort, 33043);
        QCOMPARE(m.files.size(), 4);
    }

    void test_urlsGanaAUrlSuelta()
    {
        auto m = Luna::parseManifest(manifiestoMinimo());
        QCOMPARE(m.files[0].urls.size(), 2);
        QCOMPARE(m.files[0].urls[1], QStringLiteral("https://espejo/cobblemon.jar"));
    }

    void test_urlSueltaSigueValiendo()
    {
        // Compatibilidad hacia atras: los manifiestos anteriores al pack 0.3.0
        // solo traen `url`. Quien tenga uno cacheado tiene que seguir jugando.
        auto m = Luna::parseManifest(manifiestoMinimo());
        QCOMPARE(m.files[1].urls.size(), 1);
        QCOMPARE(m.files[1].urls[0], QStringLiteral("https://uno/axiom.jar"));
    }

    void test_banderasDeCadaEntrada()
    {
        auto m = Luna::parseManifest(manifiestoMinimo());
        QVERIFY(m.files[2].archive);
        QVERIFY(m.files[2].keepExisting);
        QCOMPARE(m.files[2].strip, 1);
        QVERIFY(m.files[3].once);
        QVERIFY(!m.files[0].once);
    }

    void test_perfiles()
    {
        auto m = Luna::parseManifest(manifiestoMinimo());
        // Sin `profiles` es de todos.
        QVERIFY(m.files[0].forProfile(QStringLiteral("jugador")));
        QVERIFY(m.files[0].forProfile(QStringLiteral("constructor")));
        // Axiom es solo del constructor.
        QVERIFY(!m.files[1].forProfile(QStringLiteral("jugador")));
        QVERIFY(m.files[1].forProfile(QStringLiteral("constructor")));
    }

    void test_huellaCorrectaPasa()
    {
        const auto raw = manifiestoMinimo();
        QString err;
        auto m = Luna::parseManifest(raw, sha1De(raw), &err);
        QVERIFY2(m.isValid(), qPrintable(err));
    }

    void test_huellaQueNoCuadraSeRECHAZA()
    {
        // ⚠ ESTA ES LA PRUEBA QUE IMPORTA DE TODAS.
        //
        // El manifiesto elige de que URL salen los ~450 MB que se instalan, o
        // sea lo que acaba EJECUTANDOSE en la maquina del jugador. Sin esta
        // comprobacion, cualquiera que pueda colocarle un JSON --un DNS
        // envenenado, el proxy de un wifi publico-- le elige los mods.
        auto manipulado = manifiestoMinimo();
        manipulado.replace("https://uno/cobblemon.jar", "https://malo/cobblemon.jar");

        QString err;
        auto m = Luna::parseManifest(manipulado, sha1De(manifiestoMinimo()), &err);
        QVERIFY(!m.isValid());
        QVERIFY(err.contains(QStringLiteral("huella")));
    }

    void test_punteroSeDistingueDelManifiesto()
    {
        // Por CONTENIDO, no por la URL: es lo unico que no depende de que nadie
        // haya migrado su configuracion.
        const QByteArray puntero = QByteArray("{\"packVersion\": \"0.2.0\", \"manifest\": \"https://x/manifest-abc.json\", \"sha1\": \"ff\", \"size\": 250}");
        QVERIFY(Luna::looksLikePointer(puntero));
        QVERIFY(!Luna::looksLikePointer(manifiestoMinimo()));

        QString err;
        auto p = Luna::parsePointer(puntero, &err);
        QVERIFY2(p.isValid(), qPrintable(err));
        QCOMPARE(p.manifestUrl, QStringLiteral("https://x/manifest-abc.json"));
        QCOMPARE(p.sha1, QStringLiteral("ff"));
    }

    void test_entradaSinOrigenTumbaElManifiestoEntero()
    {
        // No se salta la entrada: un pack al que le falta un mod no arranca, y
        // "Incompatible mods found!" en la pantalla del jugador es mucho peor
        // que un error claro aqui.
        const QByteArray roto = QByteArray("{\"minecraft\": \"1.21.1\", \"files\": [{\"path\": \"mods/x.jar\", \"sha1\": \"aa\"}]}");
        QString err;
        auto m = Luna::parseManifest(roto, QString(), &err);
        QVERIFY(!m.isValid());
        QVERIFY(!err.isEmpty());
    }

    void test_basuraNoRevienta()
    {
        QString err;
        QVERIFY(!Luna::parseManifest(QByteArray("no soy json"), QString(), &err).isValid());
        QVERIFY(!err.isEmpty());
        QVERIFY(!Luna::parseManifest(QByteArray("[]"), QString(), &err).isValid());
        QVERIFY(!Luna::parseManifest(QByteArray("{}"), QString(), &err).isValid());
        QVERIFY(!Luna::looksLikePointer(QByteArray("no soy json")));
    }
};

QTEST_GUILESS_MAIN(LunaManifestTest)

#include "LunaManifest_test.moc"
