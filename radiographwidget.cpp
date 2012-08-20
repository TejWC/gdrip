/**********************************************************************
 * This file is part of the GNU Dental Radiograph Image Program, also
 * known as "gdrip."
 *
 * gdrip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gdrip.  If not, see <http://www.gnu.org/licenses/>.
 *********************************************************************/

#include "radiographwidget.h"
#include "ui_radiographwidget.h"

#include <QDebug>
#include <QColor>
#include <QPainter>
#include <QProgressDialog>
#include <QGraphicsPixmapItem>
#include <QBrush>
#include <QGraphicsRectItem>
#include <QRectF>

#include "math.h"

RadiographWidget::RadiographWidget(QWidget *parent) :
    QGraphicsView(parent),
    ui(new Ui::RadiographWidget)
{
    ui->setupUi(this);
    m_Rotation = 0;

    QGraphicsScene *scene = new QGraphicsScene(this);
    m_PixItem= new QGraphicsPixmapItem(0,scene);

    this->setScene(scene);
    this->setInteractive(true);
    this->setRenderHints(QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform);
    this->setDragMode(QGraphicsView::ScrollHandDrag);

    m_MJItem = new QGraphicsRectItem(m_PixItem->boundingRect(),m_PixItem);
    m_MJItem->setBrush(QBrush(Qt::green));

    m_MJEffect = new QGraphicsOpacityEffect();
    m_MJEffect->setOpacity(0);
    m_MJItem->setGraphicsEffect(m_MJEffect);
}

RadiographWidget::~RadiographWidget()
{
    delete ui;
}

void RadiographWidget::setZoom(int newZoom) {
    float amount =  (newZoom/50.0);
    m_PixItem->setScale(amount);
}

void RadiographWidget::setRotation(int angle) {
    m_Rotation = angle;
    m_PixItem->setRotation(angle);
}

void RadiographWidget::setBrightness(int amount) {
    if(amount < 50) { //darken
        m_MJItem->setBrush(Qt::black);
        m_MJEffect->setOpacity(((50-amount) * 2) / 100.0);
    } else { //brighten
        m_MJItem->setBrush(Qt::white);
        m_MJEffect->setOpacity(((amount-50) * 2) / 100.0);
    }
}

void RadiographWidget::setImage(QImage img) {
    QPixmap pixmap;
    pixmap.convertFromImage(img,Qt::ColorOnly);
    ui->label->setText("");
    m_PixItem->setPixmap(pixmap);
    QRectF bounds = m_PixItem->boundingRect();
    m_PixItem->setTransformOriginPoint(bounds.width()/2,bounds.height()/2);
    m_MJItem->setRect(m_PixItem->boundingRect());
}
