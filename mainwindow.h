#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QList>
#include <QPoint>
#include "enemy.h"

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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    //重写绘图事件用于绘制场景基础UI和方块
    void paintEvent(QPaintEvent *event) override;
    //重写鼠标事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::MainWindow *ui;

    //场景相关常量
    const int windowWidth = 1280;
    const int windowHeight = 720;
    const int cellSize = 60; //单个方格的边长
    const int gridRows = 6;  //场上可用网格的行数
    const int gridCols = 12; //场上可用网格的列数
    const int topBeltHeight = 150; //上方传送带高度
    const int sidePanelWidth = 200; //两侧面板(道具区/计分板)的宽度

    //战斗区域(玩家网格)的起始坐标
    int battleAreaStartX;
    int battleAreaStartY;

    //传送带逻辑相关
    QTimer *gameTimer; //游戏主循环定时器
    QList<BlockGroup> conveyorBlocks; //传送带上的所有方块组
    int spawnTimer; //生成计时器

    //战斗网格数据结构
    int battleGrid[6][12];

    //拖拽相关的状态变量
    bool isDragging;
    BlockGroup draggedGroup;
    QPoint dragOffset;

    //记录抓取时的初始位置,用于放置失败时退回
    double originalDragX;
    double originalDragY;

    //战斗相关变量
    QList<Bullet> bullets; //场上的子弹
    QList<Enemy*> enemies; //场上的敌人(使用指针以支持多态)
    int enemySpawnTimer; //敌人生成计时器

    //私有函数
    void updateGame(); //定时器触发的逻辑更新函数
    void spawnBlockGroup(); //生成新的方块组

    //三消与战斗逻辑函数
    void checkMatch(); //检测并处理同色直线三消
};
#endif // MAINWINDOW_H