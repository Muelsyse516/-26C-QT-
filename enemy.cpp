#include "enemy.h"

//1.普通敌人(灰色方形)
void NormalEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(150,150,150));
    painter.setPen(QPen(Qt::black,2));
    painter.drawRect(x,y,40,40);
    //血条
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 45, 40*hp/maxHp, 5);
}

//2.重装敌人(深蓝色大方形)
void HeavyEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(50,50,150));
    painter.setPen(QPen(Qt::black,2));
    painter.drawRect(x,y-5,50,50); 
    //血条
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 50, 50*hp/maxHp, 5);
}

//3.快速敌人(橙色小圆形)
void FastEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(250,150,50));
    painter.setPen(QPen(Qt::black,2));
    painter.drawEllipse(x+5,y+5,30,30); 
    //血条
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 40, 40*hp/maxHp, 5);
}

//4.精英敌人(紫色圆)
void EliteEnemy::draw(QPainter &painter) {
    painter.setBrush(QColor(150,50,200));
    painter.setPen(QPen(Qt::black,2));
    painter.drawEllipse(x,y,45,45); 
    //血条
    painter.setBrush(Qt::red);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y + 50, 45*hp/maxHp, 5);
}