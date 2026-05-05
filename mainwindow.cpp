#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QRandomGenerator>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //设置窗口固定大小为1280x720(16:9)
    setFixedSize(windowWidth,windowHeight);
    setWindowTitle("消消乐防线");

    //计算战斗区域(网格)居中的起始坐标
    battleAreaStartX=sidePanelWidth+(windowWidth-2*sidePanelWidth-gridCols*cellSize)/2;
    battleAreaStartY=topBeltHeight+80;

    //初始化网格为空(-1)
    for(int r=0;r<gridRows;++r) {
        for(int c=0;c<gridCols;++c) {
            battleGrid[r][c]=-1;
        }
    }
    isDragging=false;

    //初始化定时器与变量
    spawnTimer=0;
    enemySpawnTimer=0;
    gameTimer=new QTimer(this);
    //将定时器的超时信号连接到updateGame函数
    connect(gameTimer,&QTimer::timeout,this,&MainWindow::updateGame);
    //启动定时器,每16毫秒刷新一次(约60帧每秒)
    gameTimer->start(16);
}

MainWindow::~MainWindow()
{
    //释放敌人指针占用的内存,防止内存泄漏
    for(Enemy* e:enemies){
        delete e;
    }
    delete ui;
}

void MainWindow::updateGame()
{
    //1.让传送带上的方块组整体向右移动(【修改】速度再次提高到4.0)
    for(int i=0;i<conveyorBlocks.size();++i){
        conveyorBlocks[i].x+=4.0; 
    }

    if(isDragging){
        originalDragX+=4.0; //同步加速
    }

    //2.移除已经完全离开屏幕右侧的方块组
    if(!conveyorBlocks.isEmpty()&&conveyorBlocks.first().x>width()+200){
        conveyorBlocks.removeFirst();
    }

    //3.控制生成频率(传送带变快了,生成间隔缩短到60帧,约1秒)
    spawnTimer++;
    if(spawnTimer>=60){
        spawnBlockGroup();
        spawnTimer=0;
    }

    //4.处理子弹移动与出界移除
    for(int i=0;i<bullets.size();++i){
        bullets[i].x+=8.0; 
    }
    for(int i=bullets.size()-1;i>=0;--i){
        if(bullets[i].x>width()){
            bullets.removeAt(i);
        }
    }

    //5.处理敌人移动与生成
    for(int i=0;i<enemies.size();++i){
        enemies[i]->move(); //触发多态
    }
    //移除走到最左侧屏幕外的敌人
    for(int i=enemies.size()-1;i>=0;--i){
        if(enemies[i]->x<-100){
            delete enemies[i];
            enemies.removeAt(i);
        }
    }
    
    //【修改】怪物生成逻辑: 修改概率与修复Y坐标生成位置
    enemySpawnTimer++;
    if(enemySpawnTimer>150){ //每隔约2.5秒生成一个敌人
        enemySpawnTimer=0;
        
        //随机在0到5行之间生成
        int spawnRow=QRandomGenerator::global()->bounded(gridRows);
        //X坐标从屏幕最右侧稍微靠里一点的地方出现(之前是+50,会导致看不见出生瞬间)
        double startX=width()-50;
        
        //【修复】精确计算Y坐标：网格起始Y + 当前行数*格子高度 + 居中偏移(格子高度60 - 怪物高度40)/2
        double startY=battleAreaStartY + spawnRow*cellSize + 10.0; 
        
        //【修改】精确控制生成概率
        int randType = QRandomGenerator::global()->bounded(100);
        if(randType < 70) {
            enemies.append(new NormalEnemy(startX,startY)); //70%普通
        } else if(randType < 85) {
            enemies.append(new HeavyEnemy(startX,startY));  //15%重装
        } else if(randType < 95) {
            enemies.append(new EliteEnemy(startX,startY));  //10%精英
        } else {
            enemies.append(new FastEnemy(startX,startY));   //5%快速
        }
    }

    //6.处理子弹与敌人的碰撞检测
    for(int i=bullets.size()-1;i>=0;--i){
        bool hit=false;
        for(int j=enemies.size()-1;j>=0;--j){
            QRectF bulletRect(bullets[i].x,bullets[i].y,15,15);
            //取一个大概的碰撞范围(40x40)
            QRectF enemyRect(enemies[j]->x,enemies[j]->y,40,40); 
            if(bulletRect.intersects(enemyRect)){
                enemies[j]->hp -= 1; //每发子弹只扣1点血
                hit=true;
                
                if(enemies[j]->hp<=0){
                    delete enemies[j]; 
                    enemies.removeAt(j); 
                }
                break; 
            }
        }
        if(hit){
            bullets.removeAt(i); 
        }
    }

    update();
}

//【修改】检测并处理同色直线三消(已删除了旧的dfs算法)
void MainWindow::checkMatch()
{
    //使用一个二维布尔数组记录哪些方块将被消除
    bool toRemove[6][12]={false};
    bool hasMatch=false;

    //1.横向扫描:寻找每一行中连续3个或以上同色的方块
    for(int r=0;r<gridRows;++r) {
        int c=0;
        while(c<gridCols) {
            int currentColor=battleGrid[r][c];
            if(currentColor==-1) {
                c++;
                continue;
            }

            //向右看有多少个连续同色的
            int matchLength=1;
            while(c+matchLength<gridCols&&battleGrid[r][c+matchLength]==currentColor) {
                matchLength++;
            }

            //如果大于等于3个,标记它们为待消除
            if(matchLength>=3) {
                for(int i=0;i<matchLength;++i) {
                    toRemove[r][c+i]=true;
                }
                hasMatch=true;
            }
            c+=matchLength; //跳过已经检测过的块
        }
    }

    //2.竖向扫描:寻找每一列中连续3个或以上同色的方块
    for(int c=0;c<gridCols;++c) {
        int r=0;
        while(r<gridRows) {
            int currentColor=battleGrid[r][c];
            if(currentColor==-1) {
                r++;
                continue;
            }

            //向下看有多少个连续同色的
            int matchLength=1;
            while(r+matchLength<gridRows&&battleGrid[r+matchLength][c]==currentColor) {
                matchLength++;
            }

            //如果大于等于3个,标记它们为待消除
            if(matchLength>=3) {
                for(int i=0;i<matchLength;++i) {
                    toRemove[r+i][c]=true;
                }
                hasMatch=true;
            }
            r+=matchLength; //跳过已经检测过的块
        }
    }

    //3.处理消除和子弹生成
    if(hasMatch) {
        QList<Bullet> newBullets;
        for(int r=0;r<gridRows;++r) {
            for(int c=0;c<gridCols;++c) {
                if(toRemove[r][c]) {
                    //【修改】每个被消除的方块都生成一颗子弹
                    Bullet bullet;
                    //子弹起始坐标就在这个方块的中心
                    bullet.x=battleAreaStartX+c*cellSize+cellSize/2.0;
                    bullet.y=battleAreaStartY+r*cellSize+cellSize/2.0-7.5;
                    bullet.color=static_cast<BlockColor>(battleGrid[r][c]);
                    newBullets.append(bullet);

                    //清空网格
                    battleGrid[r][c]=-1;
                }
            }
        }
        //将新生成的子弹加入场上
        bullets.append(newBullets);
    }
}

void MainWindow::spawnBlockGroup()
{
    BlockGroup group;
    //初始位置:从屏幕最左侧外开始生成
    group.x=-200;

    //获取随机数(0-99)来决定形状和颜色策略,处理概率
    int shapeRand=QRandomGenerator::global()->bounded(100);
    int colorPolicyRand=QRandomGenerator::global()->bounded(100);

    //30%概率同色,70%概率随机色
    bool isSameColor=(colorPolicyRand<30);
    //如果判定为同色,则先随机确定这组方块的主颜色
    BlockColor mainColor=static_cast<BlockColor>(QRandomGenerator::global()->bounded(4));

    //使用Lambda表达式来方便地获取颜色
    auto getColor=[&]()->BlockColor{
        if(isSameColor) return mainColor; //如果是同色策略,直接返回主颜色
        //如果是随机色策略,每个方块独立随机生成一种颜色
        return static_cast<BlockColor>(QRandomGenerator::global()->bounded(4));
    };

    //根据你的要求分配形状的概率
    if(shapeRand<25){
        //25%概率(0-24):单独的一个方块
        group.blocks.append({0,0,getColor()});
    }else if(shapeRand<55){
        //30%概率(25-54):两个方块相连
        //细分为50%横向,50%竖向
        if(QRandomGenerator::global()->bounded(100)<50){
            group.blocks.append({0,0,getColor()});
            group.blocks.append({1,0,getColor()}); //横向
        } else {
            group.blocks.append({0,0,getColor()});
            group.blocks.append({0,1,getColor()}); //竖向
        }
    }else if(shapeRand<90){
        //35%概率(55-89):三个方块成直角(L型)
        //进一步细分,50%概率正L,50%概率反L(手性异构)
        if(QRandomGenerator::global()->bounded(100)<50){
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
        //10%概率(90-99):三个方块成一条直线
        group.blocks.append({0,0,getColor()}); //原点
        group.blocks.append({1,0,getColor()}); //右边第一个
        group.blocks.append({2,0,getColor()}); //右边第二个
    }

    //动态计算完美居中的初始Y坐标
    int minRy=0;
    int maxRy=0;
    if(!group.blocks.isEmpty()) {
        minRy=group.blocks[0].ry;
        maxRy=group.blocks[0].ry;
        for(const Block &b:group.blocks) {
            if(b.ry<minRy) minRy=b.ry;
            if(b.ry>maxRy) maxRy=b.ry;
        }
    }
    //居中公式:传送带高度一半(75)-形状自身占用高度的中心偏移
    group.y=75-(minRy+maxRy+1)*(cellSize/2);

    //将生成好的方块组加入到传送带列表中
    conveyorBlocks.append(group);
}

//鼠标按下事件:判断是否点中了传送带上的方块,以及处理拖拽时的右键旋转
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    //如果当前正在拖拽方块,且按下了鼠标右键,则进行顺时针旋转90度
    if(isDragging&&event->button()==Qt::RightButton) {
        for(int i=0;i<draggedGroup.blocks.size();++i) {
            //顺时针旋转90度的坐标变换公式:新x=-旧y,新y=旧x
            int oldRx=draggedGroup.blocks[i].rx;
            int oldRy=draggedGroup.blocks[i].ry;
            draggedGroup.blocks[i].rx=-oldRy;
            draggedGroup.blocks[i].ry=oldRx;
        }
        update();
        return; //旋转完毕后直接返回,不执行后续的抓取逻辑
    }

    if(event->button()==Qt::LeftButton) {
        //从后往前遍历,确保先抓取视觉上最上层的方块
        for(int i=conveyorBlocks.size()-1;i>=0;--i) {
            BlockGroup group=conveyorBlocks[i];
            bool hit=false;
            //检测鼠标坐标是否落在这个方块组的某个小方块内
            for(const Block &b:group.blocks) {
                QRectF blockRect(group.x+b.rx*cellSize,group.y+b.ry*cellSize,cellSize,cellSize);
                if(blockRect.contains(event->pos())) {
                    hit=true;
                    break;
                }
            }
            if(hit) {
                isDragging=true;
                draggedGroup=group;
                //记录鼠标点击位置与方块组左上角的偏移量,保证拖拽平滑
                dragOffset=event->pos()-QPoint(group.x,group.y);

                //记录抓取时的原始X和Y坐标
                originalDragX=group.x;
                originalDragY=group.y;

                conveyorBlocks.removeAt(i); //从传送带上取下
                update();
                return; //每次只能抓起一个方块组
            }
        }
    }
}

//鼠标移动事件:更新被拖拽方块的坐标
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isDragging) {
        draggedGroup.x=event->pos().x()-dragOffset.x();
        draggedGroup.y=event->pos().y()-dragOffset.y();
        update();
    }
}

//鼠标松开事件:计算网格吸附与放置逻辑
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton&&isDragging) {
        isDragging=false;

        //利用qRound实现四舍五入,计算最靠近哪个网格
        int targetCol=qRound((draggedGroup.x-battleAreaStartX)/(double)cellSize);
        int targetRow=qRound((draggedGroup.y-battleAreaStartY)/(double)cellSize);

        //检查合法性:不能超出网格边界,且目标格子必须为空
        bool canPlace=true;
        for(const Block &b:draggedGroup.blocks) {
            int r=targetRow+b.ry;
            int c=targetCol+b.rx;
            if(r<0||r>=gridRows||c<0||c>=gridCols||battleGrid[r][c]!=-1) {
                canPlace=false;
                break;
            }
        }

        if(canPlace) {
            //如果合法,将方块的颜色写入对应的网格数据中
            for(const Block &b:draggedGroup.blocks) {
                int r=targetRow+b.ry;
                int c=targetCol+b.rx;
                battleGrid[r][c]=b.color;
            }
            //放置成功后,检测是否有同色直线三消
            checkMatch();
        } else {
            //如果放置不合法,恢复抓取时的原始坐标,并放回传送带中
            draggedGroup.x=originalDragX;
            draggedGroup.y=originalDragY;
            conveyorBlocks.append(draggedGroup);
        }

        update();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //0.绘制全局背景底色
    painter.fillRect(0,0,width(),height(),QColor(245,235,215));

    //1.绘制上方传送带区域
    painter.fillRect(0,0,width(),topBeltHeight,QColor(220,240,235));
    painter.setPen(QPen(QColor(100,100,100),2));
    painter.drawLine(0,topBeltHeight,width(),topBeltHeight);

    //2.绘制左侧道具区边框
    painter.setPen(QPen(QColor(80,80,80),2,Qt::DashLine));
    painter.setBrush(QColor(235,225,205));
    painter.drawRoundedRect(20,topBeltHeight+50,sidePanelWidth-40,height()-topBeltHeight-100,10,10);
    painter.drawText(QRect(20,topBeltHeight+60,sidePanelWidth-40,30),Qt::AlignCenter,"道具区(SKILL)");

    //3.绘制右侧计分板与时间区边框
    painter.drawRoundedRect(width()-sidePanelWidth+20,topBeltHeight+50,sidePanelWidth-40,height()-topBeltHeight-100,10,10);
    painter.drawText(QRect(width()-sidePanelWidth+20,topBeltHeight+60,sidePanelWidth-40,30),Qt::AlignCenter,"状态(STATUS)");

    //4.绘制中间核心战斗区域(12x6网格)
    painter.setPen(QPen(QColor(50,50,50),3));
    painter.setBrush(QColor(250,245,230));
    painter.drawRect(battleAreaStartX,battleAreaStartY,gridCols*cellSize,gridRows*cellSize);

    //绘制内部网格线
    painter.setPen(QPen(QColor(200,200,200),1));
    for(int row=0;row<=gridRows;++row) {
        int y=battleAreaStartY+row*cellSize;
        painter.drawLine(battleAreaStartX,y,battleAreaStartX+gridCols*cellSize,y);
    }
    for(int col=0;col<=gridCols;++col) {
        int x=battleAreaStartX+col*cellSize;
        painter.drawLine(x,battleAreaStartY,x,battleAreaStartY+gridRows*cellSize);
    }

    //怪物刷新区标识
    painter.setBrush(QColor(255,200,200,50));
    painter.setPen(Qt::NoPen);
    painter.drawRect(battleAreaStartX+(gridCols-1)*cellSize,battleAreaStartY,cellSize,gridRows*cellSize);

    //绘制已经放置在战斗网格中的方块
    for(int r=0;r<gridRows;++r) {
        for(int c=0;c<gridCols;++c) {
            if(battleGrid[r][c]!=-1) {
                int drawX=battleAreaStartX+c*cellSize;
                int drawY=battleAreaStartY+r*cellSize;

                QColor fillColor;
                switch(battleGrid[r][c]){
                case Color_Red: fillColor=QColor(250,100,100); break;
                case Color_Yellow: fillColor=QColor(250,220,100); break;
                case Color_Blue: fillColor=QColor(100,150,250); break;
                case Color_Green: fillColor=QColor(100,200,100); break;
                }
                painter.setBrush(fillColor);
                painter.setPen(QPen(Qt::black,2));
                painter.drawRoundedRect(drawX,drawY,cellSize,cellSize,8,8);
            }
        }
    }

    //5.绘制传送带上的方块组
    for(const BlockGroup &group:conveyorBlocks){
        for(const Block &b:group.blocks){
            //计算每个小方块的绝对坐标
            double drawX=group.x+b.rx*cellSize;
            double drawY=group.y+b.ry*cellSize;

            QColor fillColor;
            switch(b.color){
            case Color_Red: fillColor=QColor(250,100,100); break;
            case Color_Yellow: fillColor=QColor(250,220,100); break;
            case Color_Blue: fillColor=QColor(100,150,250); break;
            case Color_Green: fillColor=QColor(100,200,100); break;
            }

            //绘制纯平面颜色的圆角方块
            painter.setBrush(fillColor);
            painter.setPen(QPen(Qt::black,2)); //保留黑色边框让方块轮廓清晰
            painter.drawRoundedRect(drawX,drawY,cellSize,cellSize,8,8);
        }
    }

    //绘制正在被鼠标拖拽的方块组(必须画在最上层所以放在最后)
    if(isDragging) {
        for(const Block &b:draggedGroup.blocks){
            double drawX=draggedGroup.x+b.rx*cellSize;
            double drawY=draggedGroup.y+b.ry*cellSize;

            QColor fillColor;
            switch(b.color){
            case Color_Red: fillColor=QColor(250,100,100); break;
            case Color_Yellow: fillColor=QColor(250,220,100); break;
            case Color_Blue: fillColor=QColor(100,150,250); break;
            case Color_Green: fillColor=QColor(100,200,100); break;
            }

            painter.setBrush(fillColor);
            painter.setPen(QPen(Qt::black,2));
            painter.drawRoundedRect(drawX,drawY,cellSize,cellSize,8,8);
        }
    }

    //绘制子弹
    for(const Bullet &bullet:bullets){
        QColor fillColor;
        switch(bullet.color){
        case Color_Red: fillColor=QColor(250,100,100); break;
        case Color_Yellow: fillColor=QColor(250,220,100); break;
        case Color_Blue: fillColor=QColor(100,150,250); break;
        case Color_Green: fillColor=QColor(100,200,100); break;
        }
        painter.setBrush(fillColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(bullet.x,bullet.y,15,15); //子弹是一个15x15的圆形
    }

    //绘制敌人(利用多态,直接调用虚函数)
    for(Enemy* enemy:enemies){
        enemy->draw(painter);
    }
}