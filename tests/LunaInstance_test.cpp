// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QTest>

#include <luna/LunaInstance.h>
#include <luna/LunaManifest.h>

// ⚠ NADA DE LITERALES EN BRUTO. `moc` no los parsea y deja de ver la clase con
//   Q_OBJECT; el enlazado muere con simbolos sin resolver que no dicen por que.
namespace {

QByteArray conFabric()
{
    return QByteArray(
        "{"
        "  \"minecraft\": \"1.21.1\","
        "  \"fabricLoader\": \"0.19.3\","
        "  \"files\": ["
        "    {"
        "      \"path\": \"mods/x.jar\","
        "      \"sha1\": \"aa\","
        "      \"url\": \"https://u/x.jar\""
        "    }"
        "  ]"
        "}");
}

QByteArray sinFabric()
{
    return QByteArray(
        "{"
        "  \"minecraft\": \"1.21.1\","
        "  \"files\": ["
        "    {"
        "      \"path\": \"mods/x.jar\","
        "      \"sha1\": \"aa\","
        "      \"url\": \"https://u/x.jar\""
        "    }"
        "  ]"
        "}");
}

QByteArray sinMinecraft()
{
    return QByteArray(
        "{"
        "  \"fabricLoader\": \"0.19.3\","
        "  \"files\": ["
        "    {"
        "      \"path\": \"mods/x.jar\","
        "      \"sha1\": \"aa\","
        "      \"url\": \"https://u/x.jar\""
        "    }"
        "  ]"
        "}");
}

}  // namespace

class LunaInstanceTest : public QObject {
    Q_OBJECT
   private slots:

    void test_lasVersionesSalenDelManifiesto()
    {
        // ⚠ NO SE ESCRIBEN A MANO EN NINGUN SITIO. Ya costo una vez: la version
        //   del cargador estaba puesta a mano, caduco en silencio y el pack
        //   generado no arrancaba.
        auto m = Luna::parseManifest(conFabric());
        QVERIFY(m.isValid());

        auto v = Luna::versionsFor(m);
        QVERIFY(v.isValid());
        QVERIFY(v.hasLoader());
        QCOMPARE(v.minecraft, QStringLiteral("1.21.1"));
        QCOMPARE(v.loaderVersion, QStringLiteral("0.19.3"));
        QCOMPARE(v.loaderUid, QStringLiteral("net.fabricmc.fabric-loader"));
    }

    void test_sinCargadorSigueSiendoValido()
    {
        // Un pack sin Fabric es raro, pero no es un error.
        auto v = Luna::versionsFor(Luna::parseManifest(sinFabric()));
        QVERIFY(v.isValid());
        QVERIFY(!v.hasLoader());
        QVERIFY(v.loaderUid.isEmpty());
    }

    void test_sinMinecraftNoSePuedeCrear()
    {
        // Mejor decirlo aqui que dejar una instancia a medias que no arranca.
        auto v = Luna::versionsFor(Luna::parseManifest(sinMinecraft()));
        QVERIFY(!v.isValid());
        QCOMPARE(Luna::makeCreationTask(Luna::parseManifest(sinMinecraft())), nullptr);
    }

    void test_manifiestoInvalidoNoCreaNada()
    {
        QCOMPARE(Luna::makeCreationTask(Luna::Manifest{}), nullptr);
    }

    void test_versionEnvueltaDevuelveSuCadena()
    {
        // El sistema de componentes solo pide descriptor(); resolver de verdad
        // ocurre al arrancar. Por eso no hace falta cargar las listas por red.
        Luna::PlainVersion v(QStringLiteral("1.21.1"));
        QCOMPARE(v.descriptor(), QStringLiteral("1.21.1"));
        QCOMPARE(v.name(), QStringLiteral("1.21.1"));
        QVERIFY(v.typeString().isEmpty());
    }

    void test_hayUnSoloNombreDeInstancia()
    {
        QVERIFY(!Luna::instanceName().isEmpty());
        QCOMPARE(Luna::instanceName(), QStringLiteral("Luna Eternal"));
    }
};

QTEST_GUILESS_MAIN(LunaInstanceTest)

#include "LunaInstance_test.moc"
