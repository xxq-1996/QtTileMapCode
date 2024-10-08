#include "mainwindow.h"
#include "ui_mainwindow.h"



// 第一步：拼接出地图  √
// 第二步：完整拼接出地图  √   只需要多放几个图片在pixmap的绘制上，超出部分不显示
// 第三步：给坐标，展现出地图，给一个坐标显示在当前缩放等级地图上
//        ①、根据坐标展示完整的地图： 中心经纬度——>中心墨菲托坐标——>根据缩放程度取得分辨率，计算左上角墨菲托坐标
//                                ——>计算左上角图片的编号——>计算瓦片数量，绘制完整的地图图片
//
//
// 第四步：实现地图放大缩小 √
// 第五步：实现地图拖拽    √
// 第六步：实时展现鼠标对应的经纬度 √
// 第七步：实现地图上的点线面绘制


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 读取配置文件（缩放程度对应的分辨率，瓦片图片存储位置）
    readjson();

    // 设置鼠标滚动捕获
    setMouseTracking(true);
    ui->centralwidget->setMouseTracking(true);
    ui->label->setMouseTracking(true);

    ui->label_loc->setText(QString("坐标值：%1,%2")
                           .arg(QString::number(m_longtitude,'f',7)).arg(QString::number(m_latitude,'f',7)));
    ui->label_loc->setAlignment(Qt::AlignLeft);


    m_isPointClick = false;  // 默认初始是不按下的
}

MainWindow::~MainWindow()
{
    delete ui;
}

//①、根据坐标展示完整的地图：
//      中心经纬度——>中心墨菲托坐标——>根据缩放程度取得分辨率，计算左上角墨菲托坐标
//      ——>计算左上角图片的编号——>计算瓦片数量，绘制完整的地图图片
QPixmap MainWindow::createMapPicture(int height,int width,double longtitude,double latitude)
{
    int mapSizeX = width;
    int mapSizeY = height;
    QPixmap m_pixmap(mapSizeX,mapSizeY);
    int tilePix=256;

    // 根据中心经纬度计算对应的墨卡托坐标
    Mercator mercatorLoc = lonlatTomercator(longtitude,latitude);

    // 根据缩放程度取得对应的分辨率，并计算出地图显示区域左上角的墨卡托坐标
    m_resolution = m_configInfo.at(m_zoom).resolution;
    m_mercatorLeftUpX = mercatorLoc.x - (m_resolution * mapSizeX / 2);
    m_mercatorLeftUpY = mercatorLoc.y + (m_resolution * mapSizeY / 2);

    // 根据左上角墨卡托坐标，计算其对应的瓦片地图编号
    int leftTopTitleRow = std::floor(qAbs(m_mercatorLeftUpX + fullExtendHalf) / m_resolution / tilePix);  // floor向下取整
    int leftTopTitleCol = std::floor(qAbs(m_mercatorLeftUpY - fullExtendHalf) / m_resolution / tilePix);

    //地理范围
    double realMercatorLeftUp_X = -fullExtendHalf + leftTopTitleRow * tilePix * m_resolution;
    double realMercatorLeftUp_Y = fullExtendHalf - leftTopTitleCol * tilePix * m_resolution;

    //左上角偏移像素 下面这两个值是小于零的
    double offSetX = (realMercatorLeftUp_X - m_mercatorLeftUpX) / m_resolution;
    double offSetY = (m_mercatorLeftUpY - realMercatorLeftUp_Y) / m_resolution;

    // 计算瓦片数量
    quint8 xClipnum = std::ceil((mapSizeX + qAbs(offSetX)) / tilePix);
    quint8 yClipnum = std::ceil((mapSizeY + qAbs(offSetY)) / tilePix);

    // 根据图片编号绘制地图
    QPainter painter(&m_pixmap);
    for(int i = 0;i < xClipnum ;i++)
    {
        for(int j = 0;j < yClipnum ;j++)
        {
            painter.drawImage(offSetX + i * tilePix,offSetY + j * tilePix,
                              GetImgSrc(leftTopTitleRow+i,leftTopTitleCol+j,m_zoom));
        }
    }

    return m_pixmap;
}

// 根据路径加载瓦片图片
QImage MainWindow::GetImgSrc(int x, int y, int z)
{
    QString str = QDir::currentPath() + QString("/debug/world/%1/%2/%3.jpg").arg(z).arg(x).arg(y);
    QFileInfo file(str);
    if(file.isFile())
    {
        return QImage(str);
    }
    else
    {
        QPixmap qp;
        qp.fill(Qt::red);
        return qp.toImage();
    }
}

// 根据经纬度计算墨卡托坐标
Mercator MainWindow::lonlatTomercator(double lontitude, double latitude)
{
    Mercator mercator{0,0};
    double y = std::log(std::tan((90 + latitude) * M_PI / 360)) / (M_PI / 180);
    y = y * fullExtendHalf / 180;
    mercator.x = lontitude * fullExtendHalf / 180;
    mercator.y = y;
    return mercator;
}

// 根据编号计算经纬度
std::pair<double, double> MainWindow::tileToLatLon(int x_tile, int y_tile, int zoom)
{
    double n = std::pow(2.0, zoom);
    // 计算经度
    double lon = x_tile / n * 360.0 - 180.0;
    // 计算纬度
    double lat_rad = std::atan(std::sinh(M_PI * (1 - 2 * y_tile / n)));
    double lat = lat_rad * (180.0 / M_PI);

    return std::make_pair(lat, lon);
}

// 根据经纬度计算瓦片编号
std::pair<qint32,qint32> MainWindow::LatLonTotile(double longtitude,double latitude,int zoom)
{
    double n = std::pow(2,zoom);
    qint32 tileX = (longtitude + 180.0)/360 * n;
    double temp = std::log(std::tan(latitude * M_PI / 180) + 1/std::cos(latitude * M_PI / 180))/(2 * M_PI);
    qint32 tileY = (0.5 - temp) * n;

    return std::make_pair(tileX,tileY);
}


// 读取配置文件
void MainWindow::readjson()
{
    jsonfile = QCoreApplication::applicationDirPath()+"/md.json";
    jsonfile.replace("\\","/");
    QFile file(jsonfile);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)){
        qDebug()<<"could not open file:"<<jsonfile;
        return;
    }
    QByteArray jsondata=file.readAll();
    file.close();
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsondata,&error);
    QJsonObject jsonObj = jsonDoc.object();
    filepath = jsonObj["filepath"].toString();

    QVector<double> resolution;

    QJsonArray jsonArray=jsonObj["data"].toArray();
    for(const QJsonValue& value:jsonArray){
        QJsonObject obj=value.toObject();
        m_configInfo.append({obj["tiernumber"].toInt(),obj["resolution"].toDouble()});
    }
}

// 根据墨卡托坐标计算瓦片编号
void MainWindow::mercatorToTileNum(double mercX, double mercY, int zoom, int &tileX, int &tileY)
{
    double worldSize = 2 * M_PI * earthRadius;
    double origin = worldSize / 2.0;

    // 计算瓦片编号
    tileX = static_cast<int>(floor((mercX + origin) / worldSize * (1 << zoom)));
    tileY = static_cast<int>(floor((origin - mercY) / worldSize * (1 << zoom)));
}

// 拼凑出完整的地图图片（瓦片+点线面信息）
void MainWindow::showPicture()
{
    QFile file(jsonfile);
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"could not open file:"<<jsonfile;
        return;
    }

    // 先将瓦片地图拼接起来
    m_pix = createMapPicture(ui->label->height(),ui->label->width(),m_longtitude,m_latitude);

    // 再将地图上变得点下面绘制到拼接的瓦片地图上
    integratePicture(m_pix);

    ui->label->setPixmap(m_pix);
}

void MainWindow::integratePicture(QPixmap& m_pix)
{
    // 计算图片左上角的墨卡托值，明确图片的显示范围
    double mercatorRightUpX = m_mercatorLeftUpX + ui->label->width() * m_resolution;
    double mercatorLeftDownY = m_mercatorLeftUpY - ui->label->height() * m_resolution;


    QPainter painter(&m_pix);
    painter.setBrush(Qt::red); // 设置填充颜色为红色

    // 融合点信息
    for(std::list<std::pair<Coordinate,Mercator>>::iterator start = m_mapPoint.begin();
               start != m_mapPoint.end();start++)
    {
        // 判断当前点是否在显示的范围内
        if((*start).second.x >= m_mercatorLeftUpX && (*start).second.x <= mercatorRightUpX
                && (*start).second.y >= mercatorLeftDownY && (*start).second.y <= m_mercatorLeftUpY)
        {
            // 重绘该点，得到这个点的位置，
            int pos_x = static_cast<int>(((*start).second.x - m_mercatorLeftUpX)/m_resolution);
            int pos_y = static_cast<int>((m_mercatorLeftUpY - (*start).second.y)/m_resolution);

            painter.drawEllipse(pos_x - 10,pos_y - 10,20,20);
            ui->label->setPixmap(m_pix);  // 将新的添加了点信息的图片进行显示
        }
    }

    // 融合线信息

    // 融合面信息
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    ui->label->setPixmap(QPixmap());   
    ui->label->setGeometry(ui->label->x(),ui->label->y(),event->size().width() - 50,event->size().height() - 120);
    ui->label_loc->setGeometry(event->size().width() - 600,event->size().height() - 40,600,40);
    showPicture();

    event->ignore();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if(m_wheeling)
    {
        m_wheeling = false;
        if(event->delta() > 0)  // 放大地图
        {
            if(m_zoom < 22)
            {
                m_zoom++;
            }
        }
        else                    // 缩小地图
        {
            if(m_zoom > 0)
            {
                m_zoom--;
            }
        }
        showPicture();
        m_wheeling = true;
    }
}

// 移动地图后，中心点经纬度的新位置
void MainWindow::movetoNewCenter(double x, double y)
{
    // 当前中心点的墨卡托坐标
    Mercator cpm=lonlatTomercator(m_longtitude,m_latitude);
    cpm.x = cpm.x + x * m_resolution;
    cpm.y = cpm.y - y * m_resolution;
    std::pair<double,double> moveLoc = mercatorTolonlat(cpm);

    // 将移动的新的中心点经纬度更新到类的成员变量上
    m_longtitude = moveLoc.first;
    m_latitude = moveLoc.second;
    // qDebug()<<m_longtitude<<"   "<<m_latitude;
}

// 鼠标移动槽函数  如果只是鼠标按下 没有滑动鼠标 不会进到这个槽函数中
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
        if(!m_isPointClick)   // 点的按钮没有拿下，此时可以移动地图
        {
            movetoNewCenter(m_lastMousePos.x() - event->x(),m_lastMousePos.y() - event->y());
            showPicture();
            m_lastMousePos = event->pos();
        }
    }
    else
    {
        double offsetX = event->x() - ui->label->pos().x();
        double offsetY = event->y() - ui->label->pos().y();

        if(offsetX >=0 && offsetY >= 0)
        {
            m_mouseLoc = getMouseLocByMidPoint(ui->label->width()/2 - offsetX,ui->label->height()/2 - offsetY);
            ui->label_loc->setText(QString("坐标值：%1,%2")
                                   .arg(QString::number(m_mouseLoc.first,'f',7))
                                   .arg(QString::number(m_mouseLoc.second,'f',7)));
        }
    }
}

// 得到鼠标当前位置经纬度，通过与中心点的坐标偏移获得
std::pair<double,double> MainWindow::getMouseLocByMidPoint(double offsetX,double offsetY)
{
    // 当前中心点的墨卡托坐标
    Mercator cpm = lonlatTomercator(m_longtitude,m_latitude);
    m_mouseMercator.x = cpm.x - offsetX * m_resolution;
    m_mouseMercator.y = cpm.y + offsetY * m_resolution;

    return mercatorTolonlat(m_mouseMercator);
}

// 鼠标释放槽函数
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    //    qDebug()<<"放开鼠标";
}

// 鼠标按下槽函数
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
    if(m_isPointClick)  // 点按钮点下了，此时需要将点信息增加进来
    {
        Coordinate loc;
        loc.longtitude = m_mouseLoc.first;
        loc.latitude = m_mouseLoc.second;
        m_mapPoint.push_back(std::make_pair(loc,m_mouseMercator));  // 将经纬度和墨卡托值存储

        QPainter painter(&m_pix);
        painter.setBrush(Qt::red); // 设置填充颜色为红色
        painter.drawEllipse(m_lastMousePos.x() - ui->label->x() - 10,m_lastMousePos.y() -ui->label->y() - 10,20,20);
        ui->label->setPixmap(m_pix);  // 将新的添加了点信息的图片进行显示
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QMessageBox::critical(nullptr, "提示","关闭窗口");
}

// 墨卡托坐标转换经纬度
std::pair<double,double> MainWindow::mercatorTolonlat(Mercator mercator)
{
    double longtitude = mercator.x/fullExtendHalf*180;
    double latitude = mercator.y/fullExtendHalf*180;
    latitude= std::atan(std::exp(latitude*M_PI/180))/(M_PI / 360) - 90;

    return std::make_pair(longtitude,latitude);
}


void MainWindow::on_pushButton_point_clicked()
{
    if(!m_isPointClick)
    {
        ui->pushButton_point->setStyleSheet(    "QPushButton {"
                                                "    background-color: gray;"
                                                "    border: 1px solid gray;"
                                                "    padding: 5px;"
                                                "}"
                                                "QPushButton:pressed {"
                                                "    background-color: red;"  // 按下时按钮背景变为红色
                                                "    border: 1px solid darkred;"
                                                "}");
        m_isPointClick = true;
    }
    else
    {
        ui->pushButton_point->setStyleSheet(    "QPushButton {"
                                                "    background-color: white;"
                                                "    border: 1px solid gray;"
                                                "    padding: 5px;"
                                                "}"
                                                "QPushButton:pressed {"
                                                "    background-color: red;"  // 按下时按钮背景变为红色
                                                "    border: 1px solid darkred;"
                                                "}");
        m_isPointClick = false;
    }
}
