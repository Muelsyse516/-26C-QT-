#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QList>

//定义方块四种颜色
enum BlockColor { Color_Red, Color_Yellow, Color_Blue, Color_Green };

//定义小方块
struct Block {
    int rx;
    int ry;
    BlockColor color;
};

//定义传送带上的方块组
struct BlockGroup {
    double x;
    double y;
    QList<Block> blocks; //小方块集合
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
    void paintEvent(QPaintEvent *event) override;
    //鼠标
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::MainWindow *ui;

    //场景
    const int windowWidth = 1280;
    const int windowHeight = 720;
    const int cellSize = 60; //方格边长
    const int gridRows = 6;  //网格行数
    const int gridCols = 12; //网格列数
    const int topBeltHeight = 150; //上方传送带高度
    const int sidePanelWidth = 200; //两侧面板(道具区/计分板)的宽度

    //战斗区域起始坐标
    int battleAreaStartX;
    int battleAreaStartY;

    //传送带相关逻辑
    QTimer *gameTimer; //游戏主循环定时器
    QList<BlockGroup> conveyorBlocks; //传送带所有方块组
    int spawnTimer; //生成计时器

    void updateGame(); //定时器触发的逻辑更新函数
    void spawnBlockGroup(); //生成新的方块组

    //战斗网格数据结构(-1表示空，其他值对应BlockColor)
    int battleGrid[6][12];

    //拖拽相关的状态变量
    bool isDragging;
    BlockGroup draggedGroup;
    QPoint dragOffset;

    //记录抓取时的初始位置，用于放置失败时退回
    double originalDragX;
    double originalDragY;
};
#endif // MAINWINDOW_H