#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QBitmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //设置窗口固定大小1280x720
    setFixedSize(windowWidth,windowHeight);
    setWindowTitle("POPUCOM");

    //计算战斗区域起始坐标
    battleAreaStartX=sidePanelWidth+(windowWidth-2*sidePanelWidth-gridCols*cellSize)/2;
    battleAreaStartY=topBeltHeight+80;

    //初始化网格为空
    for(int r=0;r<gridRows;++r) {
        for(int c=0;c<gridCols;++c) {
            battleGrid[r][c]=-1;
        }
    }
    isDragging=false;

    //初始化定时器与变量
    spawnTimer=0;
    enemySpawnTimer=0; 
    
    //初始化分数与时间
    score=0;
    gameTime=0;
    frameCount=0;
    isGameOver=false; 
    isPaused=false; //初始不暂停
    isMenu=true;    //初始进入主菜单

    //初始化道具状态
    isDraggingSkill = false;
    draggedSkill = Skill_None;
    hammerCooldown = 0;
    bombCooldown = 0;
    laserCooldown = 0;

    //加载道具图片
    if (hammerIcon.load("D:/Qtproject/popucom/images/hammer.jpg")) {
        hammerIcon.setMask(hammerIcon.createMaskFromColor(Qt::white));
    }
    if (bombIcon.load("D:/Qtproject/popucom/images/bomb.jpg")) {
        bombIcon.setMask(bombIcon.createMaskFromColor(Qt::white));
    }
    if (laserIcon.load("D:/Qtproject/popucom/images/laser.jpg")) {
        laserIcon.setMask(laserIcon.createMaskFromColor(Qt::white));
    }

    //加载背景图
    menuBg.load("D:/Qtproject/popucom/images/menu_bg.jpg");
    battleBg.load("D:/Qtproject/popucom/images/battle_bg.jpg");

    gameTimer=new QTimer(this);
    //将定时器的超时信号连接到updateGame函数
    connect(gameTimer,&QTimer::timeout,this,&MainWindow::updateGame);
    //启动定时器,每16毫秒刷新一次(约60帧每秒)
    gameTimer->start(16);
}

MainWindow::~MainWindow()
{
    //释放敌人指针占用的内存
    for(Enemy* e:enemies){
        delete e;
    }
    delete ui;
}

void MainWindow::resetGame()
{
    // 清空所有容器
    conveyorBlocks.clear();
    bullets.clear();
    for(Enemy* e : enemies) {
        delete e;
    }
    enemies.clear();

    // 清空网格
    for(int r=0;r<gridRows;++r) {
        for(int c=0;c<gridCols;++c) {
            battleGrid[r][c] = -1;
        }
    }

    // 重置所有状态变量
    score = 0;
    gameTime = 0;
    frameCount = 0;
    spawnTimer = 0;
    enemySpawnTimer = 0;
    
    hammerCooldown = 0;
    bombCooldown = 0;
    laserCooldown = 0;
    currentSkill = Skill_None;
    isDragging = false;
    
    isGameOver = false;
    isPaused = false;
    
    update();
}

void MainWindow::updateGame()
{
    //冷却时间减少
    if(hammerCooldown > 0) hammerCooldown--;
    if(bombCooldown > 0) bombCooldown--;
    if(laserCooldown > 0) laserCooldown--;

    // 如果游戏已经结束，或者处于暂停状态，或者处于主菜单状态，就不再更新任何逻辑
    if(isGameOver || isPaused || isMenu) return;

    //存活时间
    frameCount++;
    if(frameCount>=62){
        frameCount=0;
        gameTime++; // 存活秒数+1
    }

    //1.让传送带上的方块组整体向右移动
    for(int i=0;i<conveyorBlocks.size();++i){
        conveyorBlocks[i].x+=2.0; 
    }

    if(isDragging){
        originalDragX+=2.0; //同步加速
    }

    //2.移除已经完全离开屏幕右侧的方块组
    if(!conveyorBlocks.isEmpty()&&conveyorBlocks.first().x>width()+200){
        conveyorBlocks.removeFirst();
    }

    //3.控制生成频率
    spawnTimer++;
    if(spawnTimer>=120){
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
        enemies[i]->move();
    }
    //6.游戏失败
    for(int i=enemies.size()-1;i>=0;--i){
        if(enemies[i]->x <= battleAreaStartX){
            isGameOver = true; // 游戏结束
            update(); // 触发最后一次重绘，画出“Game Over”
            return;   // 退出更新循环
        }
    }
    
    //7.怪物生成逻辑: 修改概率与修复Y坐标生成位置
    enemySpawnTimer++;
    if(enemySpawnTimer>187){ // 187 * 16ms ≈ 3000ms，即每隔约3秒生成一个敌人
        enemySpawnTimer=0;
        //随机在0到5行之间生成
        int spawnRow=QRandomGenerator::global()->bounded(gridRows);
        //X坐标
        double startX = battleAreaStartX + gridCols * cellSize;
        //Y坐标
        double startY=battleAreaStartY + spawnRow*cellSize + 10.0; 
        //怪物生成逻辑: 按照新设定的概率生成4种怪物之一
        int randType = QRandomGenerator::global()->bounded(100);
        if(randType < 55) {
            enemies.append(new NormalEnemy(startX,startY)); //55%普通
        } else if(randType < 80) {
            enemies.append(new EliteEnemy(startX,startY));  //25%精英
        } else if(randType < 90) {
            enemies.append(new HeavyEnemy(startX,startY));  //10%重装
        } else {
            enemies.append(new VanguardEnemy(startX,startY));   //10%先锋
        }
    }

    //8.处理子弹与敌人的碰撞检测
    for(int i=bullets.size()-1;i>=0;--i){
        bool hit=false;
        for(int j=enemies.size()-1;j>=0;--j){
            QRectF bulletRect(bullets[i].x,bullets[i].y,15,15);
            //取一个大概的碰撞范围(40x40)
            QRectF enemyRect(enemies[j]->x,enemies[j]->y,40,40); 
            if(bulletRect.intersects(enemyRect)){
                enemies[j]->hp -= 1;
                hit=true;
                
                if(enemies[j]->hp<=0){
                    score += enemies[j]->scoreValue;
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

//9.检测并处理同色直线三消
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
                    //每个被消除的方块都生成一颗子弹
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
        if(isSameColor) return mainColor; //同色直接返回主颜色
        //随机色独立随机生成一种颜色
        return static_cast<BlockColor>(QRandomGenerator::global()->bounded(4));
    };
    if(shapeRand<25){
        //25%概率(0-24):单独的一个方块
        group.blocks.append({0,0,getColor()});
    }else if(shapeRand<55){
        //30%概率(25-54):两个方块相连
        if(QRandomGenerator::global()->bounded(100)<50){
            group.blocks.append({0,0,getColor()});
            group.blocks.append({1,0,getColor()}); //横向
        } else {
            group.blocks.append({0,0,getColor()});
            group.blocks.append({0,1,getColor()}); //竖向
        }
    }else if(shapeRand<90){
        //35%概率(55-89):三个方块成直角(L型)
        //50%概率正L,50%概率反L(手性异构)
        if(QRandomGenerator::global()->bounded(100)<50){
            //正L型
            group.blocks.append({0,0,getColor()});
            group.blocks.append({1,0,getColor()});
            group.blocks.append({0,1,getColor()});
        }else{
            //反L型
            group.blocks.append({0,0,getColor()});
            group.blocks.append({1,0,getColor()});
            group.blocks.append({0,-1,getColor()});
        }
    }else{
        //10%概率(90-99):三个方块成一条直线
        group.blocks.append({0,0,getColor()});
        group.blocks.append({1,0,getColor()});
        group.blocks.append({2,0,getColor()});
    }
    //动态计算居中的初始Y坐标
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
    //传送带高度一半(75)-形状自身占用高度的中心偏移
    group.y=75-(minRy+maxRy+1)*(cellSize/2);
    //将生成好的方块组加入到传送带列表中
    conveyorBlocks.append(group);
}

//鼠标按下事件
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 1. 如果在主菜单界面
    if(isMenu) {
        if(event->button() == Qt::LeftButton) {
            QRect startRect(width()/2 - 100, height()/2, 200, 60);
            QRect exitRect(width()/2 - 100, height()/2 + 100, 200, 60);
            
            if(startRect.contains(event->pos())) {
                isMenu = false;
                resetGame();
            } else if(exitRect.contains(event->pos())) {
                close();
            }
        }
        return;
    }

    // 2. 如果游戏结束，点击左键重新开始
    if(isGameOver) {
        if(event->button() == Qt::LeftButton) {
            resetGame();
        }
        return;
    }

    // 3. 如果游戏暂停，只允许点击恢复按钮
    if(isPaused) {
        QRect pauseRect(width()/2 - 60, height() - 50, 120, 40);
        if(event->button() == Qt::LeftButton && pauseRect.contains(event->pos())) {
            isPaused = false;
            update();
        }
        return;
    }

    // 4. 正常游戏中的点击逻辑

    //暂停按钮
    QRect pauseRect(width()/2 - 60, height() - 50, 120, 40);
    if(event->button() == Qt::LeftButton && pauseRect.contains(event->pos())) {
        isPaused = !isPaused;
        update();
        return;
    }
    if(isPaused) return; // 暂停时禁止其他操作
    //道具按钮
    QRect hammerRect(55, topBeltHeight + 110, 90, 90);
    QRect bombRect(55, topBeltHeight + 240, 90, 90);
    QRect laserRect(55, topBeltHeight + 370, 90, 90);

    if(event->button() == Qt::LeftButton) {
        if(hammerRect.contains(event->pos()) && hammerCooldown <= 0 && score >= 3) {
            isDraggingSkill = true;
            draggedSkill = Skill_Hammer;
            skillDragPos = event->pos();
            update();
            return;
        }
        if(bombRect.contains(event->pos()) && bombCooldown <= 0 && score >= 10) {
            isDraggingSkill = true;
            draggedSkill = Skill_Bomb;
            skillDragPos = event->pos();
            update();
            return;
        }
        if(laserRect.contains(event->pos()) && laserCooldown <= 0 && score >= 20) {
            isDraggingSkill = true;
            draggedSkill = Skill_Laser;
            skillDragPos = event->pos();
            update();
            return;
        }
    }

    //如果当前正在拖拽方块,且按下了鼠标右键,则进行顺时针旋转90度
    if(isDragging&&event->button()==Qt::RightButton) {
        for(int i=0;i<draggedGroup.blocks.size();++i) {
            //新x=-旧y,新y=旧x
            int oldRx=draggedGroup.blocks[i].rx;
            int oldRy=draggedGroup.blocks[i].ry;
            draggedGroup.blocks[i].rx=-oldRy;
            draggedGroup.blocks[i].ry=oldRx;
        }
        update();
        return; //旋转完毕后直接返回,不执行后续的抓取逻辑
    }

    if(event->button()==Qt::LeftButton) {
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

//鼠标移动事件
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(isDraggingSkill) {
        skillDragPos = event->pos();
        update();
        return;
    }
    if(isDragging) {
        draggedGroup.x=event->pos().x()-dragOffset.x();
        draggedGroup.y=event->pos().y()-dragOffset.y();
        update();
    }
}

//鼠标松开事件
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && isDraggingSkill) {
        isDraggingSkill = false;
        
        // 判断是否落入网格
        if(event->pos().x() >= battleAreaStartX && event->pos().x() < battleAreaStartX + gridCols * cellSize &&
           event->pos().y() >= battleAreaStartY && event->pos().y() < battleAreaStartY + gridRows * cellSize) {
            
            int c = (event->pos().x() - battleAreaStartX) / cellSize;
            int r = (event->pos().y() - battleAreaStartY) / cellSize;

            if(draggedSkill == Skill_Hammer) {
                if(battleGrid[r][c] != -1) { // 只能锤有方块的格子
                    battleGrid[r][c] = -1;
                    hammerCooldown = 5 * 62; // 5s冷却
                    score -= 3;
                }
            } else if(draggedSkill == Skill_Bomb) {
                for(int i = r - 1; i <= r + 1; ++i) {
                    for(int j = c - 1; j <= c + 1; ++j) {
                        if(i >= 0 && i < gridRows && j >= 0 && j < gridCols) {
                            battleGrid[i][j] = -1;
                        }
                    }
                }
                bombCooldown = 12 * 62; // 12s冷却
                score -= 10;
            } else if(draggedSkill == Skill_Laser) {
                for(int j = 0; j < gridCols; ++j) {
                    battleGrid[r][j] = -1;
                }
                for(int i = enemies.size() - 1; i >= 0; --i) {
                    int enemyRow = qRound((enemies[i]->y - battleAreaStartY) / (double)cellSize);
                    if(enemyRow == r) {
                        delete enemies[i];
                        enemies.removeAt(i);
                    }
                }
                laserCooldown = 30 * 62; // 30s冷却
                score -= 20;
            }
        }
        draggedSkill = Skill_None;
        update();
        return;
    }

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

    //1.绘制全局背景底色或图片
    if(isMenu) {
        if(!menuBg.isNull()) {
            painter.drawPixmap(0, 0, width(), height(), menuBg);
        } else {
            painter.fillRect(0,0,width(),height(),QColor(245,235,215));
        }
    } else {
        if(!battleBg.isNull()) {
            painter.drawPixmap(0, 0, width(), height(), battleBg);
        } else {
            painter.fillRect(0,0,width(),height(),QColor(245,235,215));
        }
    }

    QFont mainFont = painter.font();
    mainFont.setPointSize(10);
    mainFont.setBold(true);
    painter.setFont(mainFont);

    // 如果在主菜单界面，只绘制主菜单
    if(isMenu) {
        QRect startRect(width()/2 - 100, height()/2, 200, 60);
        QRect exitRect(width()/2 - 100, height()/2 + 100, 200, 60);

        painter.setBrush(QColor(100, 200, 100));
        painter.setPen(QPen(Qt::black, 2));
        painter.drawRoundedRect(startRect, 10, 10);
        
        painter.setBrush(QColor(200, 100, 100));
        painter.drawRoundedRect(exitRect, 10, 10);

        QFont btnFont = mainFont;
        btnFont.setPointSize(20);
        painter.setFont(btnFont);
        painter.setPen(Qt::white);
        painter.drawText(startRect, Qt::AlignCenter, "开始游戏");
        painter.drawText(exitRect, Qt::AlignCenter, "退出游戏");

        // 绘制玩法说明简述
        QFont helpFont = mainFont;
        helpFont.setPointSize(12);
        painter.setFont(helpFont);
        painter.setPen(QColor(100, 100, 100));
        QString helpText = "玩法说明：\n1. 左键拖拽传送带上的方块\n2. 点击右键旋转方块\n3. 消除方块会发射子弹攻击怪物\n4. 灵活使用左侧道具防守";
        painter.drawText(QRect(0, height()/2 + 200, width(), 150), Qt::AlignCenter, helpText);
        
        return;
    }

    //2.绘制上方传送带区域
    painter.fillRect(0,0,width(),topBeltHeight,QColor(220,240,235, 180));
    painter.setPen(QPen(QColor(100,100,100),2));
    painter.drawLine(0,topBeltHeight,width(),topBeltHeight);

    //3.绘制左侧道具区边框
    painter.setPen(QPen(QColor(80,80,80),2,Qt::DashLine));
    painter.setBrush(QColor(235,225,205, 180));
    painter.drawRoundedRect(20,topBeltHeight+50,sidePanelWidth-40,height()-topBeltHeight-100,10,10);

    QFont panelFont = mainFont;
    panelFont.setPointSize(16);
    painter.setFont(panelFont);
    painter.setPen(Qt::black);
    painter.drawText(QRect(20,topBeltHeight+60,sidePanelWidth-40,30),Qt::AlignCenter,"skill");
    painter.setFont(mainFont);

    //绘制锤子按钮
    QRect hammerRect(55, topBeltHeight + 110, 90, 90);
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(hammerCooldown > 0 || score < 3 ? QColor(180,180,180) : QColor(200,150,100));
    painter.drawRoundedRect(hammerRect, 8, 8);
    if (!hammerIcon.isNull()) {
        painter.drawPixmap(hammerRect, hammerIcon);
    }
    if (hammerCooldown > 0 || score < 3) {
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRoundedRect(hammerRect, 8, 8);
    }
    painter.setPen(Qt::black);
    painter.drawText(QRect(20, hammerRect.bottom(), sidePanelWidth - 40, 25), Qt::AlignCenter, hammerCooldown > 0 ? QString("冷却: %1s").arg(hammerCooldown/62 + 1) : "花费: 3");

    //绘制炸弹按钮
    QRect bombRect(55, topBeltHeight + 240, 90, 90);
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(bombCooldown > 0 || score < 10 ? QColor(180,180,180) : QColor(250,100,100));
    painter.drawRoundedRect(bombRect, 8, 8);
    if (!bombIcon.isNull()) {
        painter.drawPixmap(bombRect, bombIcon);
    }
    if (bombCooldown > 0 || score < 10) {
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRoundedRect(bombRect, 8, 8);
    }
    painter.setPen(Qt::black);
    painter.drawText(QRect(20, bombRect.bottom(), sidePanelWidth - 40, 25), Qt::AlignCenter, bombCooldown > 0 ? QString("冷却: %1s").arg(bombCooldown/62 + 1) : "花费: 10");

    //绘制激光按钮
    QRect laserRect(55, topBeltHeight + 370, 90, 90);
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(laserCooldown > 0 || score < 20 ? QColor(180,180,180) : QColor(100,150,250));
    painter.drawRoundedRect(laserRect, 8, 8);
    if (!laserIcon.isNull()) {
        painter.drawPixmap(laserRect, laserIcon);
    }
    if (laserCooldown > 0 || score < 20) {
        painter.setBrush(QColor(0, 0, 0, 128));
        painter.drawRoundedRect(laserRect, 8, 8);
    }
    painter.setPen(Qt::black);
    painter.drawText(QRect(20, laserRect.bottom(), sidePanelWidth - 40, 25), Qt::AlignCenter, laserCooldown > 0 ? QString("冷却: %1s").arg(laserCooldown/62 + 1) : "花费: 20");

    //4.绘制右侧计分板与时间区边框
    painter.setPen(QPen(QColor(80,80,80),2,Qt::DashLine));
    painter.setBrush(QColor(235,225,205, 180));
    painter.drawRoundedRect(width()-sidePanelWidth+20,topBeltHeight+50,sidePanelWidth-40,height()-topBeltHeight-100,10,10);
    
    //设置面板标题的大号字体
    painter.setFont(panelFont);
    painter.setPen(Qt::black);
    painter.drawText(QRect(width()-sidePanelWidth+20,topBeltHeight+60,sidePanelWidth-40,40),Qt::AlignCenter,"status");

    //绘制分数
    painter.setFont(panelFont);
    painter.setPen(QColor(50, 100, 200));
    painter.drawText(QRect(width()-sidePanelWidth+20, topBeltHeight+130, sidePanelWidth-40, 50), Qt::AlignCenter, QString("score"));
    painter.setFont(panelFont);
    painter.setPen(QColor(50, 50, 50));
    painter.drawText(QRect(width()-sidePanelWidth+20, topBeltHeight+180, sidePanelWidth-40, 50), Qt::AlignCenter, QString::number(score));

    //绘制存活时间
    painter.setFont(panelFont);
    painter.setPen(QColor(50, 100, 200));
    painter.drawText(QRect(width()-sidePanelWidth+20, topBeltHeight+280, sidePanelWidth-40, 50), Qt::AlignCenter, QString("survive"));
    int min = gameTime / 60;
    int sec = gameTime % 60;
    painter.setFont(panelFont);
    painter.setPen(Qt::black);
    painter.drawText(QRect(width()-sidePanelWidth+20, topBeltHeight+330, sidePanelWidth-40, 50), Qt::AlignCenter, QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
    mainFont.setPointSize(9);
    mainFont.setBold(false);
    painter.setFont(mainFont);

    //绘制暂停按钮
    QRect pauseRect(width()/2 - 60, height() - 50, 120, 40);
    painter.setBrush(isPaused ? QColor(100,200,100) : QColor(200,100,100));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRoundedRect(pauseRect, 5, 5);
    painter.setPen(Qt::white);
    painter.drawText(pauseRect, Qt::AlignCenter, isPaused ? "▶ 继续 RESUME" : "⏸ 暂停 PAUSE");

    //5.绘制中间核心区域
    painter.setPen(QPen(QColor(50,50,50),3));
    painter.setBrush(QColor(250,245,230, 150));
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

    //6.绘制传送带上的方块组
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

    //绘制正在被鼠标拖拽的方块组
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

    //绘制子弹 (带拖尾特效)
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
        painter.drawEllipse(bullet.x,bullet.y,15,15); // 子弹头部
        
        // 绘制渐变拖尾
        QLinearGradient gradient(bullet.x, bullet.y + 7.5, bullet.x - 30, bullet.y + 7.5);
        gradient.setColorAt(0, fillColor);
        QColor tailColor = fillColor;
        tailColor.setAlpha(0);
        gradient.setColorAt(1, tailColor);
        painter.setBrush(gradient);
        painter.drawRect(bullet.x - 30, bullet.y + 3, 30, 9);
    }

    //绘制敌人(直接调用虚函数)
    for(Enemy* enemy:enemies){
        enemy->draw(painter);
    }

    //绘制正在拖拽的道具
    if(isDraggingSkill && draggedSkill != Skill_None) {
        QRect dragRect(skillDragPos.x() - 45, skillDragPos.y() - 45, 90, 90);
        if(draggedSkill == Skill_Hammer) {
            painter.setBrush(QColor(200,150,100));
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRoundedRect(dragRect, 5, 5);
            if(!hammerIcon.isNull()) painter.drawPixmap(dragRect, hammerIcon);
            else { painter.setPen(Qt::white); painter.drawText(dragRect, Qt::AlignCenter, "🔨"); }
        } else if(draggedSkill == Skill_Bomb) {
            painter.setBrush(QColor(250,100,100));
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRoundedRect(dragRect, 5, 5);
            if(!bombIcon.isNull()) painter.drawPixmap(dragRect, bombIcon);
            else { painter.setPen(Qt::white); painter.drawText(dragRect, Qt::AlignCenter, "💣"); }
        } else if(draggedSkill == Skill_Laser) {
            painter.setBrush(QColor(100,150,250));
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRoundedRect(dragRect, 5, 5);
            if(!laserIcon.isNull()) painter.drawPixmap(dragRect, laserIcon);
            else { painter.setPen(Qt::white); painter.drawText(dragRect, Qt::AlignCenter, "⚡"); }
        }
    }

    //7.如果游戏结束，在屏幕中央绘制半透明黑色遮罩和白色失败文字
    if(isGameOver){
        painter.fillRect(0, 0, width(), height(), QColor(0, 0, 0, 150));
        
        QFont overFont = mainFont;
        overFont.setPointSize(40);
        overFont.setBold(true);
        painter.setFont(overFont);
        painter.setPen(Qt::white);
        painter.drawText(QRect(0, height()/2 - 150, width(), 100), Qt::AlignCenter, "game over");
        
        QFont scoreFont = mainFont;
        scoreFont.setPointSize(30);
        painter.setFont(scoreFont);
        painter.drawText(QRect(0, height()/2 - 30, width(), 100), Qt::AlignCenter, QString("最终得分: %1").arg(score));
        
        QFont restartFont = mainFont;
        restartFont.setPointSize(20);
        painter.setFont(restartFont);
        painter.drawText(QRect(0, height()/2 + 100, width(), 50), Qt::AlignCenter, "- 点击鼠标左键重新开始 -");
    }
}