#ifndef ENEMY_H
#define ENEMY_H

#include <QPainter>
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

    Enemy(double startX,double startY,int h,double s,int score) : x(startX),y(startY),hp(h),maxHp(h),speed(s),scoreValue(score) {}
    virtual ~Enemy() {}

    //虚函数:移动
    virtual void move() {
        x-=speed;
    }

    //纯虚函数:绘制,交由子类去具体实现不同的外观
    virtual void draw(QPainter &painter) = 0;
};

//1.普通敌人:移速中等偏慢(0.20),血量为1,得分1
class NormalEnemy : public Enemy {
public:
    NormalEnemy(double startX,double startY) : Enemy(startX,startY,1,0.20,1) {}
    void draw(QPainter &painter) override;
};

//2.精英敌人:移速中等偏快(0.35),血量为2,得分2
class EliteEnemy : public Enemy {
public:
    EliteEnemy(double startX,double startY) : Enemy(startX,startY,2,0.35,2) {}
    void draw(QPainter &painter) override;
};

//3.重装敌人:移速缓慢(0.10),血量为3,得分2
class HeavyEnemy : public Enemy {
public:
    HeavyEnemy(double startX,double startY) : Enemy(startX,startY,3,0.10,2) {}
    void draw(QPainter &painter) override;
};

//4.先锋敌人(原快速敌人):移速快(0.50),血量为1,得分2
class VanguardEnemy : public Enemy {
public:
    VanguardEnemy(double startX,double startY) : Enemy(startX,startY,1,0.50,2) {}
    void draw(QPainter &painter) override;
};

#endif // ENEMY_H