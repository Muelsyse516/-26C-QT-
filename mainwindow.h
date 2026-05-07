#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QList>
#include <QPoint>
#include "enemy.h"
#include "gamedefines.h"

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
    //绘图事件用于绘制场景基础UI和方块
    void paintEvent(QPaintEvent *event) override;
    //鼠标事件
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
    const int sidePanelWidth = 200; //两侧面板的宽度

    //战斗区域的起始坐标
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
    QList<Enemy*> enemies; //场上的敌人
    int enemySpawnTimer; //敌人生成计时器

    //计分与时间
    int score;
    int gameTime; //存活时间
    int frameCount; //用于计时的帧计数器
    bool isGameOver; //游戏是否结束标志
    bool isPaused;   //是否暂停
    bool isMenu;     //是否在主菜单界面

    //道具与系统控制
    bool isDraggingSkill;
    ActiveSkill draggedSkill;
    QPoint skillDragPos;
    int hammerCooldown; //锤子冷却
    int bombCooldown; //炸弹冷却
    int laserCooldown; //激光冷却
    ActiveSkill currentSkill;
    //私有函数
    void updateGame(); //定时器触发的逻辑更新函数
    void spawnBlockGroup(); //生成新的方块组
    void checkMatch(); //检测并处理同色直线三消
    void resetGame();  //重置游戏状态
};
#endif // MAINWINDOW_H