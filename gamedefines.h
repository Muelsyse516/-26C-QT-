#ifndef GAMEDEFINES_H
#define GAMEDEFINES_H

#include <QList>

//定义方块的四种颜色
enum BlockColor { Color_Red, Color_Yellow, Color_Blue, Color_Green };

//定义单个小方块
struct Block {
    int rx; //在方块组内的相对X网格坐标
    int ry; //在方块组内的相对Y网格坐标
    BlockColor color; //颜色
};

//定义在传送带上移动的方块组
struct BlockGroup {
    double x; //在屏幕上的绝对X坐标
    double y; //在屏幕上的绝对Y坐标
    QList<Block> blocks; //包含的小方块集合
};

//新增子弹结构体
struct Bullet {
    double x;
    double y;
    BlockColor color;
};

//技能状态枚举
enum ActiveSkill { Skill_None, Skill_Bomb, Skill_Laser, Skill_Hammer };

#endif // GAMEDEFINES_H
