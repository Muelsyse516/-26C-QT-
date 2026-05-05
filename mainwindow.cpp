#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QRandomGenerator>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(windowWidth, windowHeight);
    setWindowTitle("POPUCOM");

    //计算战斗区域(网格)居中的起始坐标
    battleAreaStartX = sidePanelWidth + (windowWidth - 2 * sidePanelWidth - gridCols * cellSize) / 2;
    battleAreaStartY = topBeltHeight + 80;
    //初始化定时器与传送带变量
    spawnTimer = 0;
    gameTimer = new QTimer(this);
    //将定时器的超时信号连接到updateGame函数
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::updateGame);
    //启动定时器，每16毫秒刷新一次
    gameTimer->start(16);

    //初始化网格为空(-1)
    for(int r = 0; r < gridRows; ++r) {
        for(int c = 0; c < gridCols; ++c) {
            battleGrid[r][c] = -1;
        }
    }
    isDragging = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateGame()
{
    //1.让传送带上的方块组整体向右移动 (加号代表向右)
    for(int i=0; i<conveyorBlocks.size(); ++i){
        conveyorBlocks[i].x += 1.5; //移动速度为1.5像素/帧
    }
    //如果玩家正在拖拽方块，让它被抓取时的原始X坐标也跟着传送带的速度一起移动
    if(isDragging){
        originalDragX += 1.5;
    }

    //2.移除已经完全离开屏幕右侧的方块组(没被抓到的方块会被过掉)
    if(!conveyorBlocks.isEmpty() && conveyorBlocks.first().x > width() + 200){
        conveyorBlocks.removeFirst();
    }

    //3.控制生成频率(每隔约2.5秒生成一个，150帧 * 16毫秒 ≈ 2400毫秒)
    spawnTimer++;
    if(spawnTimer >= 150){
        spawnBlockGroup();
        spawnTimer = 0;
    }

    //调用update触发paintEvent重新绘制画面
    update();
}
void MainWindow::spawnBlockGroup()
{
    BlockGroup group;
    //初始位置：从屏幕最左侧外开始生成
    group.x = -200;

    //获取随机数(0-99)来决定形状和颜色策略，处理概率
    int shapeRand = QRandomGenerator::global()->bounded(100);
    int colorPolicyRand = QRandomGenerator::global()->bounded(100);

    //30%概率同色，70%概率随机色
    bool isSameColor = (colorPolicyRand < 30);
    //如果判定为同色，则先随机确定这组方块的主颜色
    BlockColor mainColor = static_cast<BlockColor>(QRandomGenerator::global()->bounded(4));

    //使用Lambda表达式来方便地获取颜色
    auto getColor = [&]()->BlockColor{
        if(isSameColor) return mainColor; //如果是同色策略，直接返回主颜色
        //如果是随机色策略，每个方块独立随机生成一种颜色
        return static_cast<BlockColor>(QRandomGenerator::global()->bounded(4));
    };

    //根据你的要求分配形状的概率
    if(shapeRand < 25){
        //25%概率(0-24): 单独的一个方块
        group.blocks.append({0,0,getColor()});
    }else if(shapeRand < 55){
        //30%概率(25-54): 两个方块相连
        //【修改】细分为50%横向，50%竖向
        if(QRandomGenerator::global()->bounded(100) < 50){
            group.blocks.append({0,0,getColor()});
            group.blocks.append({1,0,getColor()}); // 横向
        } else {
            group.blocks.append({0,0,getColor()});
            group.blocks.append({0,1,getColor()}); // 竖向
        }
    }else if(shapeRand < 90){
        //35%概率(55-89): 三个方块成直角(L型)
        //进一步细分，50%概率正L，50%概率反L(手性异构)
        if(QRandomGenerator::global()->bounded(100) < 50){
            //正L型
            group.blocks.append({0,0,getColor()}); //原点
            group.blocks.append({1,0,getColor()}); //右边一个
            group.blocks.append({0,1,getColor()}); //下边一个
        }else{
            //反L型(镜像)
            group.blocks.append({0,0,getColor()}); //原点
            group.blocks.append({1,0,getColor()}); //右边一个
            group.blocks.append({0,-1,getColor()}); //上边一个
        }
    }else{
        //10%概率(90-99): 三个方块成一条直线
        group.blocks.append({0,0,getColor()}); //原点
        group.blocks.append({1,0,getColor()}); //右边第一个
        group.blocks.append({2,0,getColor()}); //右边第二个
    }

    //【新增】动态计算完美居中的初始Y坐标
    int minRy = 0;
    int maxRy = 0;
    if (!group.blocks.isEmpty()) {
        minRy = group.blocks[0].ry;
        maxRy = group.blocks[0].ry;
        for (const Block &b : group.blocks) {
            if (b.ry < minRy) minRy = b.ry;
            if (b.ry > maxRy) maxRy = b.ry;
        }
    }
    // 居中公式：传送带高度一半(75) - 形状自身占用高度的中心偏移
    group.y = 75 - (minRy + maxRy + 1) * (cellSize / 2);

    //将生成好的方块组加入到传送带列表中
    conveyorBlocks.append(group);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //1.绘制全局背景底色
    painter.fillRect(0, 0, width(), height(), QColor(245, 235, 215));

    //2.绘制上方传送带区域
    painter.fillRect(0, 0, width(), topBeltHeight, QColor(220, 240, 235));
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawLine(0, topBeltHeight, width(), topBeltHeight);

    //3.绘制左侧道具区边框
    painter.setPen(QPen(QColor(80, 80, 80), 2, Qt::DashLine));
    painter.setBrush(QColor(235, 225, 205));
    painter.drawRoundedRect(20, topBeltHeight + 50, sidePanelWidth - 40, height() - topBeltHeight - 100, 10, 10);
    painter.drawText(QRect(20, topBeltHeight + 60, sidePanelWidth - 40, 30), Qt::AlignCenter, "道具区(SKILL)");

    //4.绘制右侧计分板与时间区边框
    painter.drawRoundedRect(width() - sidePanelWidth + 20, topBeltHeight + 50, sidePanelWidth - 40, height() - topBeltHeight - 100, 10, 10);
    painter.drawText(QRect(width() - sidePanelWidth + 20, topBeltHeight + 60, sidePanelWidth - 40, 30), Qt::AlignCenter, "状态(STATUS)");

    //5.绘制中间核心战斗区域(12x6网格)
    painter.setPen(QPen(QColor(50, 50, 50), 3));
    painter.setBrush(QColor(250, 245, 230));
    painter.drawRect(battleAreaStartX, battleAreaStartY, gridCols * cellSize, gridRows * cellSize);
    //绘制内部网格线
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    for (int row = 0; row <= gridRows; ++row) {
        int y = battleAreaStartY + row * cellSize;
        painter.drawLine(battleAreaStartX, y, battleAreaStartX + gridCols * cellSize, y);
    }
    for (int col = 0; col <= gridCols; ++col) {
        int x = battleAreaStartX + col * cellSize;
        painter.drawLine(x, battleAreaStartY, x, battleAreaStartY + gridRows * cellSize);
    }
    //怪物刷新区标识
    painter.setBrush(QColor(255, 200, 200, 50));
    painter.setPen(Qt::NoPen);
    painter.drawRect(battleAreaStartX + (gridCols - 1) * cellSize, battleAreaStartY, cellSize, gridRows * cellSize);

    //6.绘制传送带上的方块组
    for(const BlockGroup &group : conveyorBlocks){
        for(const Block &b : group.blocks){
            //计算每个小方块的绝对坐标
            double drawX = group.x + b.rx * cellSize;
            double drawY = group.y + b.ry * cellSize;

            QColor fillColor;
            switch(b.color){
            case Color_Red: fillColor = QColor(250,100,100); break;
            case Color_Yellow: fillColor = QColor(250,220,100); break;
            case Color_Blue: fillColor = QColor(100,150,250); break;
            case Color_Green: fillColor = QColor(100,200,100); break;
            }

            //绘制纯平面颜色的圆角方块
            painter.setBrush(fillColor);
            painter.setPen(QPen(Qt::black, 2)); //保留黑色边框让方块轮廓清晰
            painter.drawRoundedRect(drawX, drawY, cellSize, cellSize, 8, 8);

        }
    }
    //7.绘制已经放置在战斗网格中的方块
    for (int r = 0; r < gridRows; ++r) {
        for (int c = 0; c < gridCols; ++c) {
            if (battleGrid[r][c] != -1) {
                int drawX = battleAreaStartX + c * cellSize;
                int drawY = battleAreaStartY + r * cellSize;

                QColor fillColor;
                switch(battleGrid[r][c]){
                case Color_Red: fillColor = QColor(250,100,100); break;
                case Color_Yellow: fillColor = QColor(250,220,100); break;
                case Color_Blue: fillColor = QColor(100,150,250); break;
                case Color_Green: fillColor = QColor(100,200,100); break;
                }
                painter.setBrush(fillColor);
                painter.setPen(QPen(Qt::black, 2));
                painter.drawRoundedRect(drawX, drawY, cellSize, cellSize, 8, 8);
            }
        }
    }
    //8.绘制正在被鼠标拖拽的方块组(必须画在最上层所以放在最后)
    if (isDragging) {
        for(const Block &b : draggedGroup.blocks){
            double drawX = draggedGroup.x + b.rx * cellSize;
            double drawY = draggedGroup.y + b.ry * cellSize;

            QColor fillColor;
            switch(b.color){
            case Color_Red: fillColor = QColor(250,100,100); break;
            case Color_Yellow: fillColor = QColor(250,220,100); break;
            case Color_Blue: fillColor = QColor(100,150,250); break;
            case Color_Green: fillColor = QColor(100,200,100); break;
            }

            painter.setBrush(fillColor);
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRoundedRect(drawX, drawY, cellSize, cellSize, 8, 8);
        }
    }
}
//鼠标按下事件：判断是否点中了传送带上的方块，以及处理拖拽时的右键旋转
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    //如果当前正在拖拽方块，且按下了鼠标右键，则进行顺时针旋转90度
    if(isDragging && event->button() == Qt::RightButton) {
        for(int i = 0; i < draggedGroup.blocks.size(); ++i) {
            //顺时针旋转90度的坐标变换公式: 新x = -旧y, 新y = 旧x
            int oldRx = draggedGroup.blocks[i].rx;
            int oldRy = draggedGroup.blocks[i].ry;
            draggedGroup.blocks[i].rx = -oldRy;
            draggedGroup.blocks[i].ry = oldRx;
        }
        update();
        return; //旋转完毕后直接返回，不执行后续的抓取逻辑
    }

    if(event->button() == Qt::LeftButton) {
        //从后往前遍历，确保先抓取视觉上最上层的方块
        for(int i = conveyorBlocks.size() - 1; i >= 0; --i) {
            BlockGroup group = conveyorBlocks[i];
            bool hit = false;
            //检测鼠标坐标是否落在这个方块组的某个小方块内
            for(const Block &b : group.blocks) {
                QRectF blockRect(group.x + b.rx * cellSize, group.y + b.ry * cellSize, cellSize, cellSize);
                if(blockRect.contains(event->pos())) {
                    hit = true;
                    break;
                }
            }
            if(hit) {
                isDragging = true;
                draggedGroup = group;
                //记录鼠标点击位置与方块组左上角的偏移量，保证拖拽平滑
                dragOffset = event->pos() - QPoint(group.x, group.y);

                //记录抓取时的原始X和Y坐标
                originalDragX = group.x;
                originalDragY = group.y;

                conveyorBlocks.removeAt(i); //从传送带上取下
                update();
                return; //每次只能抓起一个方块组
            }
        }
    }
}

//鼠标移动事件：更新被拖拽方块的坐标
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isDragging) {
        draggedGroup.x = event->pos().x() - dragOffset.x();
        draggedGroup.y = event->pos().y() - dragOffset.y();
        update();
    }
}

//鼠标松开事件：计算网格吸附与放置逻辑
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && isDragging) {
        isDragging = false;

        //利用qRound实现四舍五入，计算最靠近哪个网格
        int targetCol = qRound((draggedGroup.x - battleAreaStartX) / (double)cellSize);
        int targetRow = qRound((draggedGroup.y - battleAreaStartY) / (double)cellSize);

        //检查合法性：不能超出网格边界，且目标格子必须为空
        bool canPlace = true;
        for(const Block &b : draggedGroup.blocks) {
            int r = targetRow + b.ry;
            int c = targetCol + b.rx;
            if(r < 0 || r >= gridRows || c < 0 || c >= gridCols || battleGrid[r][c] != -1) {
                canPlace = false;
                break;
            }
        }

        if(canPlace) {
            //如果合法，将方块的颜色写入对应的网格数据中
            for(const Block &b : draggedGroup.blocks) {
                int r = targetRow + b.ry;
                int c = targetCol + b.rx;
                battleGrid[r][c] = b.color;
            }
        } else {
            //【修改】如果放置不合法，恢复抓取时的原始坐标，并放回传送带中
            draggedGroup.x = originalDragX;
            draggedGroup.y = originalDragY;
            conveyorBlocks.append(draggedGroup);
        }

        update();
    }
}