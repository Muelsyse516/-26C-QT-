#ifndef ENEMY_H
#define ENEMY_H

#include <QPainter>
#include "gamedefines.h"

//利用C++多态设计敌人基类
class Enemy {
public:
    double x;
    double y;
    int hp;
    int maxHp;
    double speed;
    int scoreValue; // 【新增】击杀得分

    Enemy(double startX,double startY,int h,double s,int score) : x(startX),y(startY),hp(h),maxHp(h),speed(s),scoreValue(score) {}
    virtual ~Enemy() {}

    //虚函数:移动
    virtual void move() {
        x-=speed;
    }

    //纯虚函数:绘制,交由子类去具体实现不同的外观
    virtual void draw(QPainter &painter) = 0;
};

//1.普通敌人:移速中等(0.25),血量为1
class NormalEnemy : public Enemy {
public:
    NormalEnemy(double startX,double startY) : Enemy(startX,startY,1,0.25,1) {}
    void draw(QPainter &painter) override;
};

//2.重装敌人:移速最慢(0.15),血量为3
class HeavyEnemy : public Enemy {
public:
    HeavyEnemy(double startX,double startY) : Enemy(startX,startY,3,0.15,2) {}
    void draw(QPainter &painter) override;
};

//3.快速敌人:移速较快(0.5),血量为1
class FastEnemy : public Enemy {
public:
    FastEnemy(double startX,double startY) : Enemy(startX,startY,1,0.5,2) {}
    void draw(QPainter &painter) override;
};

//4.精英敌人:移速中等(0.25),血量为2
class EliteEnemy : public Enemy {
public:
    EliteEnemy(double startX,double startY) : Enemy(startX,startY,2,0.25,2) {}
    void draw(QPainter &painter) override;
};

#endif // ENEMY_H