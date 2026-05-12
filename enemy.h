#ifndef ENEMY_H
#define ENEMY_H

#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QString>
#include "gamedefines.h"

//敌人基类
class Enemy {
public:
    double x;
    double y;
    int hp;
    int maxHp;
    double speed;
    int scoreValue;
    QPixmap pixmap;

    Enemy(double startX,double startY,int h,double s,int score, const QString& imgPath) 
        : x(startX),y(startY),hp(h),maxHp(h),speed(s),scoreValue(score) 
    {
        if (pixmap.load(imgPath)) {
            pixmap.setMask(pixmap.createMaskFromColor(Qt::white));
        }
    }
    virtual ~Enemy() {}

    //移动
    virtual void move() {
        x-=speed;
    }

    //绘制贴图
    virtual void draw(QPainter &painter) {
        if(!pixmap.isNull()) {

            painter.drawPixmap(x, y, 40, 40, pixmap);
        } else {
            painter.setBrush(Qt::gray);
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRect(x, y, 40, 40);
            painter.setPen(Qt::white);
            painter.drawText(x + 5, y + 25, "IMG?");
        }
        // 画血条
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawRect(x, y + 45, 40 * hp / maxHp, 5);
    }
};

//1.普通敌人:移速中等偏慢(0.20),血量为1,得分1
class NormalEnemy : public Enemy {
public:
    NormalEnemy(double startX,double startY) : Enemy(startX,startY,1,0.20,1,"D:/Qtproject/popucom/images/normal.jpg") {}
};

//2.精英敌人:移速中等偏快(0.35),血量为2,得分2
class EliteEnemy : public Enemy {
public:
    EliteEnemy(double startX,double startY) : Enemy(startX,startY,2,0.35,2,"D:/Qtproject/popucom/images/elite.jpg") {}
};

//3.重装敌人:移速缓慢(0.10),血量为3,得分2
class HeavyEnemy : public Enemy {
public:
    HeavyEnemy(double startX,double startY) : Enemy(startX,startY,3,0.10,2,"D:/Qtproject/popucom/images/heavy.jpg") {}
};

//4.先锋敌人:移速快(0.50),血量为1,得分2
class VanguardEnemy : public Enemy {
public:
    VanguardEnemy(double startX,double startY) : Enemy(startX,startY,1,0.50,2,"D:/Qtproject/popucom/images/vanguard.jpg") {}
};

#endif // ENEMY_H