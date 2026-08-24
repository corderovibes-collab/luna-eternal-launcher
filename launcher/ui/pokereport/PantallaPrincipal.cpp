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
#include "ui/pokereport/PantallaPrincipal.h"

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "ui/themes/PokeReportTheme.h"

namespace {

/**
 * Una fila que se puede pulsar.
 *
 * ⚠ ES UN QWidget Y NO UN QPushButton, y eso obliga a esto. Un boton no deja
 *   apilar dos lineas de texto con estilos distintos sin pelearse con su
 *   propio dibujado, y la fila necesita titulo + detalle. A cambio hay que
 *   reemitir el clic a mano.
 */
class FilaPulsable : public QWidget {
   public:
    explicit FilaPulsable(QWidget* parent, std::function<void()> alPulsar)
        : QWidget(parent), m_alPulsar(std::move(alPulsar))
    {
        setCursor(Qt::PointingHandCursor);
    }

   protected:
    void mouseReleaseEvent(QMouseEvent* e) override
    {
        // ⚠ `rect().contains()`: si alguien pulsa dentro y suelta fuera, NO
        //   cuenta. Es como se comporta cualquier boton, y sin esto arrastrar
        //   desde la fila cambiaria de perfil sin querer.
        if (e->button() == Qt::LeftButton && rect().contains(e->pos()) && m_alPulsar) {
            m_alPulsar();
        }
        QWidget::mouseReleaseEvent(e);
    }

   private:
    std::function<void()> m_alPulsar;
};

/**
 * El icono de la fila de perfil: una Poke Ball dibujada a linea.
 *
 * ⚠ SE DIBUJA, NO SE USA LA DEL LOGO. A 20 px la ball en llamas se convierte en
 *   una mancha naranja: el detalle que la hace reconocible --el brillo, el aro
 *   del boton, las llamas-- desaparece. Un circulo partido con su boton se lee
 *   a cualquier tamaño.
 *
 * ⚠ Y NO se usa un glifo de texto. Se probo con `◒`: la fuente del sistema lo
 *   dibuja diminuto y descentrado dentro de su caja, y salia como una rayita.
 */
QLabel* iconoBall(QWidget* padre, QColor color)
{
    const int lado = 34;
    const qreal escala = 2.0;  // por las pantallas a 150 %

    QPixmap pm(QSize(lado, lado) * escala);
    pm.setDevicePixelRatio(escala);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QRectF r(4.5, 4.5, lado - 9.0, lado - 9.0);

    QPen pluma(color, 1.8);
    p.setPen(pluma);
    p.drawEllipse(r);
    p.drawLine(QPointF(r.left(), r.center().y()), QPointF(r.right(), r.center().y()));

    // El boton: relleno del color del fondo para que la linea del medio no lo
    // cruce, y el aro encima.
    const qreal rb = r.width() * 0.22;
    QRectF boton(r.center().x() - rb, r.center().y() - rb, rb * 2, rb * 2);
    p.setBrush(QColor(0x12, 0x0F, 0x11));
    p.drawEllipse(boton);
    p.end();

    auto* l = new QLabel(padre);
    l->setFixedSize(lado, lado);
    l->setPixmap(pm);
    return l;
}

}  // namespace

PantallaPrincipal::PantallaPrincipal(QWidget* parent) : QWidget(parent)
{
    iniciarRecursoPokeReport();

    auto* raiz = new QHBoxLayout(this);
    raiz->setContentsMargins(0, 0, 0, 0);
    raiz->setSpacing(0);

    // ------------------------------------------------------------- izquierda
    auto* lateral = new QWidget(this);
    lateral->setObjectName(QStringLiteral("lateralPerfiles"));
    lateral->setFixedWidth(232);
    auto* col = new QVBoxLayout(lateral);
    col->setContentsMargins(12, 14, 12, 12);
    col->setSpacing(6);

    auto* titulo = new QLabel(tr("PERFILES"), lateral);
    titulo->setObjectName(QStringLiteral("tituloLateral"));
    col->addWidget(titulo);
    col->addSpacing(2);

    m_filaJugador = filaPerfil(tr("Jugador"), tr("1.21.1 · Fabric"), false);
    m_filaConstructor = filaPerfil(tr("Constructor"), tr("Con Axiom"), true);
    col->addWidget(m_filaJugador);
    col->addWidget(m_filaConstructor);
    col->addStretch(1);

    // La tarjeta del servidor, abajo del todo.
    auto* tarjeta = new QWidget(lateral);
    tarjeta->setObjectName(QStringLiteral("tarjetaServidor"));
    auto* tc = new QVBoxLayout(tarjeta);
    tc->setContentsMargins(12, 10, 12, 10);
    tc->setSpacing(6);

    auto* etiqueta = new QLabel(tr("SERVIDOR"), tarjeta);
    etiqueta->setObjectName(QStringLiteral("tituloLateral"));
    tc->addWidget(etiqueta);

    auto* fila = new QWidget(tarjeta);
    auto* fh = new QHBoxLayout(fila);
    fh->setContentsMargins(0, 0, 0, 0);
    fh->setSpacing(8);
    m_estadoPunto = new QLabel(QStringLiteral("●"), fila);
    m_estadoTexto = new QLabel(fila);
    m_estadoTexto->setObjectName(QStringLiteral("estadoServidor"));
    fh->addWidget(m_estadoPunto);
    fh->addWidget(m_estadoTexto);
    fh->addStretch(1);
    tc->addWidget(fila);

    m_pack = new QLabel(tarjeta);
    m_pack->setObjectName(QStringLiteral("packServidor"));
    tc->addWidget(m_pack);

    col->addWidget(tarjeta);
    raiz->addWidget(lateral);

    // ---------------------------------------------------------------- heroe
    auto* heroe = new QWidget(this);
    heroe->setObjectName(QStringLiteral("heroe"));
    auto* hv = new QVBoxLayout(heroe);
    hv->setContentsMargins(30, 30, 30, 30);
    hv->setSpacing(0);
    hv->addStretch(3);

    auto* ball = new QLabel(heroe);
    QPixmap pb(QStringLiteral(":/pokereport/ball@2x.png"));
    if (!pb.isNull()) {
        pb.setDevicePixelRatio(2.0);
        ball->setPixmap(pb);
    }
    ball->setAlignment(Qt::AlignCenter);
    hv->addWidget(ball);

    auto* rotulo = new QLabel(heroe);
    QPixmap pr(QStringLiteral(":/pokereport/letras_grande@2x.png"));
    if (!pr.isNull()) {
        pr.setDevicePixelRatio(2.0);
        rotulo->setPixmap(pr);
    }
    rotulo->setAlignment(Qt::AlignCenter);
    // El rotulo sube sobre la ball: en el logo van pegados, no separados.
    hv->addSpacing(-14);
    hv->addWidget(rotulo);

    hv->addSpacing(26);
    m_jugar = new QPushButton(tr("JUGAR"), heroe);
    m_jugar->setObjectName(QStringLiteral("botonJugar"));
    m_jugar->setCursor(Qt::PointingHandCursor);
    // ⚠ `setDefault` NO basta aqui: esto no es un dialogo. El degradado se le
    //   pone por el nombre del objeto en el .qss.
    m_jugar->setFixedHeight(52);
    m_jugar->setMinimumWidth(230);
    auto* wj = new QHBoxLayout();
    wj->addStretch(1);
    wj->addWidget(m_jugar);
    wj->addStretch(1);
    hv->addLayout(wj);

    hv->addStretch(4);
    raiz->addWidget(heroe, 1);

    connect(m_jugar, &QPushButton::clicked, this, &PantallaPrincipal::jugarPulsado);

    ponerEstado(Estado::Comprobando);
    repintarPerfiles();
}

QWidget* PantallaPrincipal::filaPerfil(const QString& titulo, const QString& detalle, bool constructor)
{
    auto* fila = new FilaPulsable(this, [this, constructor] { emit perfilElegido(constructor); });
    fila->setObjectName(QStringLiteral("filaPerfil"));

    auto* h = new QHBoxLayout(fila);
    h->setContentsMargins(10, 9, 10, 9);
    h->setSpacing(11);

    auto* icono = iconoBall(fila, QColor(0xA7, 0x9F, 0xA4));
    icono->setObjectName(QStringLiteral("iconoPerfil"));
    h->addWidget(icono);

    auto* textos = new QVBoxLayout();
    textos->setContentsMargins(0, 0, 0, 0);
    textos->setSpacing(1);
    auto* t = new QLabel(titulo, fila);
    t->setObjectName(QStringLiteral("tituloPerfil"));
    auto* d = new QLabel(detalle, fila);
    d->setObjectName(QStringLiteral("detallePerfil"));
    textos->addWidget(t);
    textos->addWidget(d);
    h->addLayout(textos);
    h->addStretch(1);

    return fila;
}

void PantallaPrincipal::repintarPerfiles()
{
    // ⚠ SE CAMBIA UNA PROPIEDAD Y SE FUERZA EL REPINTADO. Qt no vuelve a
    //   aplicar la hoja de estilos cuando cambia una propiedad dinamica: hay
    //   que decirselo. Sin `unpolish`/`polish` el color se queda como estaba y
    //   parece que el clic no ha hecho nada.
    const std::pair<QWidget*, bool> filas[] = {
        { m_filaJugador, false },
        { m_filaConstructor, true },
    };
    for (const auto& par : filas) {
        if (!par.first) {
            continue;
        }
        par.first->setProperty("elegido", par.second == m_constructor);
        par.first->style()->unpolish(par.first);
        par.first->style()->polish(par.first);
        // Los hijos tambien: el titulo y el detalle cambian de color con el
        // selector `[elegido="true"] QLabel#...`, y a ellos hay que decirselo
        // por separado.
        const auto hijos = par.first->findChildren<QWidget*>();
        for (QWidget* hijo : hijos) {
            hijo->style()->unpolish(hijo);
            hijo->style()->polish(hijo);
        }
    }
}

void PantallaPrincipal::ponerPerfil(bool constructor)
{
    m_constructor = constructor;
    repintarPerfiles();
}

void PantallaPrincipal::ponerPack(const QString& texto)
{
    m_pack->setText(texto);
    m_pack->setVisible(!texto.isEmpty());
}

void PantallaPrincipal::ponerEstado(Estado estado, const QString& detalle)
{
    QString color;
    QString texto;
    switch (estado) {
        case Estado::EnLinea:
            color = QStringLiteral("#4ADE80");
            texto = tr("En linea");
            break;
        case Estado::SinConexion:
            color = QStringLiteral("#FF4D5B");
            texto = tr("Sin conexion");
            break;
        case Estado::Comprobando:
            color = QStringLiteral("#6B6469");
            texto = tr("Comprobando…");
            break;
    }
    m_estadoPunto->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(color));
    m_estadoTexto->setText(detalle.isEmpty() ? texto : texto + QStringLiteral(" · ") + detalle);
}

void PantallaPrincipal::ponerJugarActivo(bool activo, const QString& etiqueta)
{
    m_jugar->setEnabled(activo);
    m_jugar->setText(etiqueta.isEmpty() ? tr("JUGAR") : etiqueta);
}
