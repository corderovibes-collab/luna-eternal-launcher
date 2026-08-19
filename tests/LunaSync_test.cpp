// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QTest>

#include <luna/LunaManifest.h>
#include <luna/LunaSync.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea, deja de ver la clase con
//   Q_OBJECT y el enlazado muere sin decir por que. Ver LunaManifest_test.cpp.
namespace {

QByteArray manifiesto()
{
    return QByteArray(
        "{"
        "  \"packVersion\": \"0.2.0\","
        "  \"minecraft\": \"1.21.1\","
        "  \"fabricLoader\": \"0.19.3\","
        "  \"files\": ["
        "    { \"path\": \"mods/cobblemon.jar\", \"sha1\": \"aaa\", \"size\": 100,"
        "      \"url\": \"https://u/cobblemon.jar\" },"
        "    { \"path\": \"mods/axiom.jar\", \"sha1\": \"bbb\", \"size\": 200,"
        "      \"url\": \"https://u/axiom.jar\", \"profiles\": [\"constructor\"] },"
        "    { \"path\": \"config\", \"sha1\": \"ccc\", \"size\": 300,"
        "      \"url\": \"https://u/config.zip\", \"archive\": true, \"keepExisting\": true },"
        "    { \"path\": \"servers.dat\", \"sha1\": \"ddd\", \"size\": 40,"
        "      \"url\": \"https://u/servers.dat\", \"once\": true },"
        "    { \"path\": \"../fuera.txt\", \"sha1\": \"eee\", \"size\": 5,"
        "      \"url\": \"https://u/malo.txt\" }"
        "  ]"
        "}");
}

/** Disco de mentira: lo que digamos que hay, hay. */
class FakeDisk final : public Luna::DiskProbe {
   public:
    QHash<QString, qint64> sizes;
    QHash<QString, QString> hashes;

    void put(const QString& p, qint64 s, const QString& h)
    {
        sizes[p] = s;
        hashes[p] = h;
    }

    QStringList jars;  ///< lo que hay DE VERDAD en mods/

    bool exists(const QString& p) const override { return sizes.contains(p); }
    qint64 size(const QString& p) const override { return sizes.value(p, -1); }
    QString sha1(const QString& p) const override { return hashes.value(p); }
    QStringList modJars() const override { return jars; }
};

QStringList rutas(const Luna::Plan& p)
{
    QStringList out;
    for (const auto& f : p.toFetch)
        out << f.file.path;
    out.sort();
    return out;
}

}  // namespace

class LunaSyncTest : public QObject {
    Q_OBJECT
   private slots:

    void test_instalacionDesdeCero()
    {
        FakeDisk disco;  // vacio
        auto plan =
            Luna::computePlan(Luna::parseManifest(manifiesto()), {}, QStringLiteral("jugador"), Luna::Mode::Normal, disco);
        // Axiom es del constructor; `../fuera.txt` es una ruta insegura.
        QCOMPARE(rutas(plan),
                 QStringList({ QStringLiteral("config"), QStringLiteral("mods/cobblemon.jar"), QStringLiteral("servers.dat") }));
        QCOMPARE(plan.bytes, 440);
    }

    void test_siNadaCambioNoSeBajaNada()
    {
        FakeDisk disco;
        disco.put(QStringLiteral("mods/cobblemon.jar"), 100, QStringLiteral("aaa"));
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QVERIFY(plan.isEmpty());
    }

    void test_unFicheroBorradoAManoVuelve()
    {
        // El estado dice que esta, pero el disco dice que no. Sin esta
        // comprobacion no volveria NUNCA.
        FakeDisk disco;
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QCOMPARE(rutas(plan), QStringList({ QStringLiteral("mods/cobblemon.jar") }));
    }

    void test_tamanoDistintoLoVuelveABajar()
    {
        FakeDisk disco;
        disco.put(QStringLiteral("mods/cobblemon.jar"), 99, QStringLiteral("aaa"));  // deberia pesar 100
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QCOMPARE(rutas(plan), QStringList({ QStringLiteral("mods/cobblemon.jar") }));
    }

    void test_onceNoSePisaAunqueNoCuadre()
    {
        // ⚠ Es LA razon de que `once` exista. En cuanto el jugador toca un
        //   ajuste, el fichero deja de coincidir con el del manifiesto; darlo
        //   por corrupto seria borrarle la configuracion en cada arranque.
        FakeDisk disco;
        disco.put(QStringLiteral("mods/cobblemon.jar"), 100, QStringLiteral("aaa"));
        disco.put(QStringLiteral("servers.dat"), 12345, QStringLiteral("otra-cosa"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QVERIFY(!rutas(plan).contains(QStringLiteral("servers.dat")));
    }

    void test_repararNoSeFiaDelEstado()
    {
        // El atajo da por bueno lo que el launcher CREE que instalo, y por eso
        // un fichero corrompido DESPUES sobrevive a cualquier actualizacion.
        // Ese fallo costo una sesion entera, con un `ZipFile invalid LOC
        // header` que no se parecia en nada a su causa.
        FakeDisk disco;
        disco.put(QStringLiteral("mods/cobblemon.jar"), 100, QStringLiteral("CORRUPTO"));
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };

        auto normal = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"),
                                        Luna::Mode::Normal, disco);
        QVERIFY(normal.isEmpty());  // el atajo no lo ve

        auto reparar = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"),
                                         Luna::Mode::Repair, disco);
        QVERIFY(rutas(reparar).contains(QStringLiteral("mods/cobblemon.jar")));
    }

    void test_elConstructorSiSeLlevaSusHerramientas()
    {
        FakeDisk disco;
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), {}, QStringLiteral("constructor"),
                                      Luna::Mode::Normal, disco);
        QVERIFY(rutas(plan).contains(QStringLiteral("mods/axiom.jar")));
    }

    void test_cambiarDePerfilDESINSTALA()
    {
        // Sin esto, quien probara el perfil de constructor se quedaria a Axiom
        // cargado para siempre.
        FakeDisk disco;
        disco.put(QStringLiteral("mods/cobblemon.jar"), 100, QStringLiteral("aaa"));
        disco.put(QStringLiteral("mods/axiom.jar"), 200, QStringLiteral("bbb"));
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("mods/axiom.jar"), QStringLiteral("bbb") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QCOMPARE(plan.toRemove, QStringList({ QStringLiteral("mods/axiom.jar") }));
    }

    void test_unJarHUERFANOenModsSeRetira()
    {
        // ⚠ ESTA PRUEBA FIJA EL FALLO QUE DEJO A JUGADORES FUERA EL 2026-08-19.
        //
        // Arrastraban `trinkets` y `accessories-compat-layer` de un pack
        // anterior. El servidor ya no los tenia, y ese puente le mandaba unas
        // ranuras que no sabia leer:
        //
        //     Failed to decode packet 'clientbound/minecraft:custom_payload'
        //     Caused by: StructFieldException: [Field: exported_slots]
        //
        // La limpieza solo miraba el estado guardado, asi que un jar que llego
        // por otra via sobrevivia a TODAS las actualizaciones. Al dueño no le
        // pasaba --su instalacion estaba bien anotada-- y por eso parecia cosa
        // de maquinas concretas en vez de un hueco del launcher.
        FakeDisk disco;
        disco.jars = { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("mods/trinkets-3.10.0.jar") };
        disco.put(QStringLiteral("mods/cobblemon.jar"), 100, QStringLiteral("aaa"));
        disco.put(QStringLiteral("servers.dat"), 40, QStringLiteral("ddd"));

        // El estado NO menciona al huerfano: es justo el caso.
        Luna::State estado{ { QStringLiteral("mods/cobblemon.jar"), QStringLiteral("aaa") },
                            { QStringLiteral("config"), QStringLiteral("ccc") },
                            { QStringLiteral("servers.dat"), QStringLiteral("ddd") } };

        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"),
                                      Luna::Mode::Normal, disco);
        QVERIFY(plan.toRemove.contains(QStringLiteral("mods/trinkets-3.10.0.jar")));
        QVERIFY(!plan.toRemove.contains(QStringLiteral("mods/cobblemon.jar")));
    }

    void test_soloSeBorraDentroDeLoQueAdministramos()
    {
        // Mundos, capturas y registros son del jugador. Que el manifiesto deje
        // de mencionarlos no da derecho a borrarlos.
        FakeDisk disco;
        Luna::State estado{ { QStringLiteral("saves/mi-mundo/level.dat"), QStringLiteral("x") },
                            { QStringLiteral("options.txt"), QStringLiteral("y") },
                            { QStringLiteral("mods/viejo.jar"), QStringLiteral("z") } };
        auto plan = Luna::computePlan(Luna::parseManifest(manifiesto()), estado, QStringLiteral("jugador"), Luna::Mode::Normal,
                                      disco);
        QCOMPARE(plan.toRemove, QStringList({ QStringLiteral("mods/viejo.jar") }));
    }

    void test_rutasQueSeEscapanSeIGNORAN()
    {
        // ⚠ El manifiesto viene de la red. Sin esto, una entrada con `../../`
        //   escribe donde quiera en la maquina del jugador.
        const QChar barra(0x5C);  // la invertida, sin literal para no pelearse con el escapado
        const QString windows = QStringLiteral("mods") + barra + QStringLiteral("..") + barra + QStringLiteral("..") + barra +
                                QStringLiteral("x");

        QVERIFY(!Luna::isSafeRelativePath(QStringLiteral("../fuera.txt")));
        QVERIFY(!Luna::isSafeRelativePath(QStringLiteral("mods/../../fuera.txt")));
        QVERIFY(!Luna::isSafeRelativePath(QStringLiteral("/etc/passwd")));
        QVERIFY(!Luna::isSafeRelativePath(QStringLiteral("C:/Windows/system32/x.dll")));
        QVERIFY(!Luna::isSafeRelativePath(windows));
        QVERIFY(Luna::isSafeRelativePath(QStringLiteral("mods/cobblemon.jar")));
        QVERIFY(Luna::isSafeRelativePath(QStringLiteral("config/iris.properties")));

        FakeDisk disco;
        auto plan =
            Luna::computePlan(Luna::parseManifest(manifiesto()), {}, QStringLiteral("jugador"), Luna::Mode::Normal, disco);
        QVERIFY(!rutas(plan).contains(QStringLiteral("../fuera.txt")));
        QVERIFY(!plan.nextState.contains(QStringLiteral("../fuera.txt")));
    }

    void test_carpetasAdministradas()
    {
        QVERIFY(Luna::isManaged(QStringLiteral("mods/x.jar")));
        QVERIFY(Luna::isManaged(QStringLiteral("config/a/b.toml")));
        QVERIFY(Luna::isManaged(QStringLiteral("shaderpacks")));
        QVERIFY(!Luna::isManaged(QStringLiteral("saves/mundo/level.dat")));
        QVERIFY(!Luna::isManaged(QStringLiteral("options.txt")));
        QVERIFY(!Luna::isManaged(QStringLiteral("modsdeotro/x.jar")));  // no vale el prefijo a medias
    }

    void test_elEstadoVaYVuelve()
    {
        Luna::State s{ { QStringLiteral("mods/a.jar"), QStringLiteral("111") },
                       { QStringLiteral("config"), QStringLiteral("222") } };
        QString version;
        auto vuelta = Luna::stateFromJson(Luna::stateToJson(s, QStringLiteral("0.2.0")), &version);
        QCOMPARE(version, QStringLiteral("0.2.0"));
        QCOMPARE(vuelta, s);
        // Basura no revienta ni inventa nada.
        QVERIFY(Luna::stateFromJson(QByteArray("no soy json")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(LunaSyncTest)

#include "LunaSync_test.moc"
