// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#include <QDebug>
#include <QList>
#include <QString>
#include <QStringView>

// this implements the FlexVer
// https://git.sleeping.town/exa/FlexVer
class Version {
   public:
    Version(QString str) : m_string(std::move(str)) { parse(); }  // NOLINT(hicpp-explicit-conversions)
    Version() = default;

    /// Convierte una ETIQUETA de git (`v0.2.0`) en la version que representa (`0.2.0`).
    ///
    /// ⚠⚠ NO ES COSMETICO: sin quitar la `v`, la comparacion sale AL REVES.
    ///
    ///    `Version` parte la cadena en tramos y los compara uno a uno. El primer
    ///    tramo de `v0.2.0` es la letra `v` --de tipo TEXTO-- y el de `0.2.0` es
    ///    el `0` --de tipo NUMERO--. Con tipos distintos la comparacion cae a
    ///    codigo de caracter: `v` es 0x76 y `0` es 0x30, asi que la etiqueta
    ///    resulta MAYOR mire los numeros que mire.
    ///
    ///    Traducido: el actualizador creia que `v0.2.0` era mas nueva que 0.2.0,
    ///    y que 0.9.9 tambien. El jugador actualizaba, arrancaba, y se le volvia
    ///    a ofrecer la misma actualizacion, sin un solo error en el log.
    ///
    ///    La `v` se quita SOLO si va seguida de un digito, para no destrozar una
    ///    etiqueta que de verdad empiece por esa letra.
    static Version fromTag(const QString& tag)
    {
        if (tag.size() >= 2 && (tag.at(0) == QLatin1Char('v') || tag.at(0) == QLatin1Char('V')) && tag.at(1).isDigit()) {
            return Version(tag.mid(1));
        }
        return Version(tag);
    }

   private:
    struct Section {
        enum class Type : std::uint8_t { Null, Textual, Numeric, PreRelease };
        explicit Section(Type t = Type::Null, QString value = "") : t(t), value(std::move(value)) {}
        Type t;
        QString value;
        bool operator==(const Section& other) const = default;
        std::strong_ordering operator<=>(const Section& other) const;
    };

   private:
    void parse();

   public:
    QString toString() const { return m_string; }
    bool isEmpty() const { return m_string.isEmpty(); }

    friend QDebug operator<<(QDebug debug, const Version& v);

    bool operator==(const Version& other) const { return (*this <=> other) == std::strong_ordering::equal; }
    std::strong_ordering operator<=>(const Version& other) const;

   private:
    QString m_string;
    QList<Section> m_sections;
};