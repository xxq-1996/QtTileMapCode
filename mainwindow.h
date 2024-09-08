#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common.h"

#include <QMainWindow>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#include <cmath>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QThread>
#include <QMessageBox>
#include <iomanip>
#include <iostream>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 将瓦片地图拼接起来
    QPixmap createMapPicture(int height,int width,double longtitude,double latitude);
    // 融合点、线、面等地图信息
    void integratePicture(QPixmap& m_pix);
    QImage GetImgSrc(int x, int y, int z);

private:
    Ui::MainWindow *ui;
    double  m_longtitude = 116.1700505;  // 初始经度
    double  m_latitude = 39.8611621;     // 初始纬度
    qint8   m_zoom = 20;
    double  m_resolution;
    double fullExtendHalf = 20037508.3427892;   // 地球半径 × PI
    double earthRadius =  6378137.0;
    QString jsonfile;
    QString filepath;
    QVector<TierParam> m_configInfo;
    bool    m_wheeling = true;      // 当滚轮滚动时，需要等待地图加载完成并显示完成后，才能继续采取滚动值，否则容易造成卡顿
    QPoint  m_lastMousePos;
    std::pair<double,double> m_mouseLoc;       // 鼠标当前的经纬度
    Mercator                 m_mouseMercator;  // 鼠标当前的墨卡托坐标值
    QPixmap m_pix;                // 当前QLabel中显示的地图pixmap
    double m_mercatorLeftUpX;      // 图片左上角的墨卡托坐标X值
    double m_mercatorLeftUpY;      // 图片左上角的墨卡托坐标Y值


    /* 地图上的点信息 */
    std::list<std::pair<Coordinate,Mercator>>  m_mapPoint;
    bool                                     m_isPointClick;


    // 根据经纬度计算墨卡托坐标
    Mercator lonlatTomercator(double lontitude,double latitude);
    // 根据瓦片编号与缩放等级计算经纬度
    std::pair<double, double> tileToLatLon(int x_tile, int y_tile, int zoom);
    // 根据经纬度计算瓦片编号
    std::pair<qint32,qint32> LatLonTotile(double longtitude,double latitude,int zoom);
    // 读取配置文件
    void readjson();
    // 墨卡托坐标计算瓦片编号
    void mercatorToTileNum(double mercX, double mercY, int zoom, int &tileX, int &tileY);
    // 展示地图那个地方的图片
    void showPicture();
    // 移动鼠标后新的中心位置
    void movetoNewCenter(double x, double y);
    // 墨卡托坐标转换经纬度
    std::pair<double,double> mercatorTolonlat(Mercator mercator);
    // 得到鼠标当前位置经纬度，通过与中心点的坐标获得
    std::pair<double,double>getMouseLocByMidPoint(double offsetX,double offsetY);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent * event) override;
private slots:
    void on_pushButton_point_clicked();
};
#endif // MAINWINDOW_H
