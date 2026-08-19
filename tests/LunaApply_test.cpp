// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Luna Eternal - Minecraft Launcher
 *  Copyright (C) 2026 PokeReport: Luna Eternal
 */

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include <luna/LunaApply.h>

// ⚠ NADA DE LITERALES EN BRUTO: `moc` no los parsea. Ver LunaManifest_test.cpp.
namespace {

bool escribir(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(data);
    f.close();
    return true;
}

QByteArray leer(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

/** Marca un fichero como oculto. Solo hace algo en Windows. */
void ocultar(const QString& path)
{
#ifdef Q_OS_WIN
    QProcess p;
    p.start(QStringLiteral("attrib"), { QStringLiteral("+h"), QDir::toNativeSeparators(path) });
    p.waitForFinished(5000);
#else
    Q_UNUSED(path)
#endif
}

}  // namespace

class LunaApplyTest : public QObject {
    Q_OBJECT
   private slots:

    void test_escribeUnFicheroNuevo()
    {
        QTemporaryDir tmp;
        const auto p = tmp.filePath(QStringLiteral("sub/carpeta/x.txt"));
        QString err;
        QVERIFY2(Luna::writeFileSafely(p, QByteArray("uno"), &err), qPrintable(err));
        QCOMPARE(leer(p), QByteArray("uno"));
    }

    void test_pisaUnoQueYaEstaba()
    {
        QTemporaryDir tmp;
        const auto p = tmp.filePath(QStringLiteral("x.txt"));
        QVERIFY(escribir(p, QByteArray("viejo")));
        QVERIFY(Luna::writeFileSafely(p, QByteArray("nuevo")));
        QCOMPARE(leer(p), QByteArray("nuevo"));
    }

    void test_pisaUnFicheroOCULTO()
    {
        // ⚠ ESTA ES LA PRUEBA QUE IMPORTA DE TODAS LAS DE ESTE FICHERO.
        //
        // En Windows, abrir para escritura un fichero OCULTO falla, y el error
        // no menciona en ningun momento que la causa sea que esta oculto.
        // Llego a un jugador: el pack trae config/euphoria_patcher/.data.json,
        // el mod lo recrea oculto, y toda actualizacion que reextrajera
        // config/ moria al 99 %.
        QTemporaryDir tmp;
        const auto p = tmp.filePath(QStringLiteral(".data.json"));
        QVERIFY(escribir(p, QByteArray("viejo")));
        ocultar(p);

        QString err;
        QVERIFY2(Luna::writeFileSafely(p, QByteArray("nuevo"), &err), qPrintable(err));
        QCOMPARE(leer(p), QByteArray("nuevo"));
    }

    void test_pisaUnoDeSoloLectura()
    {
        QTemporaryDir tmp;
        const auto p = tmp.filePath(QStringLiteral("ro.txt"));
        QVERIFY(escribir(p, QByteArray("viejo")));
        QFile::setPermissions(p, QFile::ReadOwner);

        QString err;
        QVERIFY2(Luna::writeFileSafely(p, QByteArray("nuevo"), &err), qPrintable(err));
        QCOMPARE(leer(p), QByteArray("nuevo"));
    }

    void test_stripQuitaNivelesDeCarpeta()
    {
        QCOMPARE(Luna::strippedEntryPath(QStringLiteral("a/b/c.txt"), 0), QStringLiteral("a/b/c.txt"));
        QCOMPARE(Luna::strippedEntryPath(QStringLiteral("a/b/c.txt"), 1), QStringLiteral("b/c.txt"));
        QCOMPARE(Luna::strippedEntryPath(QStringLiteral("a/b/c.txt"), 2), QStringLiteral("c.txt"));
    }

    void test_stripQueSeLoComeTodoDevuelveVacio()
    {
        // La entrada ERA una de las carpetas que se quitan.
        QVERIFY(Luna::strippedEntryPath(QStringLiteral("a/b.txt"), 2).isEmpty());
        QVERIFY(Luna::strippedEntryPath(QStringLiteral("a"), 1).isEmpty());
    }

    void test_entradasDeZipQueSeEscapanSeIGNORAN()
    {
        // ⚠ Un zip tambien viene de la red. `../` dentro de un archivo es la
        //   via clasica para escribir fuera del destino.
        QVERIFY(Luna::strippedEntryPath(QStringLiteral("../fuera.txt"), 0).isEmpty());
        QVERIFY(Luna::strippedEntryPath(QStringLiteral("a/../../fuera.txt"), 1).isEmpty());
        QVERIFY(Luna::strippedEntryPath(QStringLiteral("/etc/passwd"), 0).isEmpty());
    }

    void test_barrasDeWindowsSeNormalizan()
    {
        // Sin normalizar, una entrada de un zip hecho en Windows se cuela
        // entera como un solo nombre y `strip` no hace nada.
        const QChar barra(0x5C);
        const QString entrada = QStringLiteral("a") + barra + QStringLiteral("b") + barra + QStringLiteral("c.txt");
        QCOMPARE(Luna::strippedEntryPath(entrada, 1), QStringLiteral("b/c.txt"));
    }

    void test_borraFicherosYCarpetas()
    {
        QTemporaryDir tmp;
        QVERIFY(escribir(tmp.filePath(QStringLiteral("mods/viejo.jar")), QByteArray("x")));
        QVERIFY(escribir(tmp.filePath(QStringLiteral("config/sub/a.toml")), QByteArray("y")));

        const int n = Luna::removeEntries(tmp.path(), { QStringLiteral("mods/viejo.jar"), QStringLiteral("config") });
        QCOMPARE(n, 2);
        QVERIFY(!QFile::exists(tmp.filePath(QStringLiteral("mods/viejo.jar"))));
        QVERIFY(!QDir(tmp.filePath(QStringLiteral("config"))).exists());
    }

    void test_borrarLoQueNoEstaNoEsUnFallo()
    {
        // Que un fichero ya no este es exactamente el estado que se buscaba.
        QTemporaryDir tmp;
        QCOMPARE(Luna::removeEntries(tmp.path(), { QStringLiteral("mods/no-existe.jar") }), 0);
    }

    void test_noBorraFueraDeLaInstancia()
    {
        QTemporaryDir fuera;
        const auto victima = fuera.filePath(QStringLiteral("importante.txt"));
        QVERIFY(escribir(victima, QByteArray("no me borres")));

        QTemporaryDir instancia;
        Luna::removeEntries(instancia.path(), { QStringLiteral("../../importante.txt"),
                                                QStringLiteral("/etc/passwd") });
        QVERIFY(QFile::exists(victima));
    }
};

QTEST_GUILESS_MAIN(LunaApplyTest)

#include "LunaApply_test.moc"
