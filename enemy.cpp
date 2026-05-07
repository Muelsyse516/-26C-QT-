#include "enemy.h"
#include <QPolygonF>
#include <cmath>

//1.普通敌人(灰色圆形)
void NormalEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(150,150,150));
    painter.setPen(QPen(Qt::black,2));
    painter.drawEllipse(x, y, 40, 40);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 45, 40*hp/maxHp, 5);
}

//2.精英敌人(紫色正五边形)
void EliteEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(150,50,200));
    painter.setPen(QPen(Qt::black,2));
    QPolygonF pentagon;
    for (int i = 0; i < 5; ++i) {
        double angle = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
        pentagon << QPointF(x + 20 + 20 * cos(angle), y + 20 + 20 * sin(angle));
    }
    painter.drawPolygon(pentagon); 
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 45, 40*hp/maxHp, 5);
}

//3.重装敌人(黑色正方形)
void HeavyEnemy::draw(QPainter &painter) {
    painter.setBrush(Qt::black);
    painter.setPen(QPen(QColor(100,100,100),2));
    painter.drawRect(x, y, 40, 40); 
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 45, 40*hp/maxHp, 5);
}

//4.先锋敌人(橙色正三角形)
void VanguardEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(250,150,50));
    painter.setPen(QPen(Qt::black,2));
    QPolygonF triangle;
    triangle << QPointF(x, y + 20) << QPointF(x + 40, y) << QPointF(x + 40, y + 40);
    painter.drawPolygon(triangle); 
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 45, 40*hp/maxHp, 5);
}