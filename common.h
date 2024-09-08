#ifndef COMMON_H
#define COMMON_H


// 墨卡托坐标
typedef struct _MERCATOR_
{
    double x;
    double y;
}Mercator;

typedef struct _COORDINATE_
{
    double longtitude;
    double latitude;
}Coordinate;

// 配置文件参数
typedef struct _TIERPARAM_
{
    int tierNum;
    double resolution;
}TierParam;

#endif // COMMON_H
