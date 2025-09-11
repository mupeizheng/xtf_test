#ifndef SHENGPUBUTU_H
#define SHENGPUBUTU_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <vector>

struct BottomPoint {
    int pingIndex;   // 第几个 ping
    int portSample;  // 左舷海底位置索引
    int starSample;  // 右舷海底位置索引
};

class shengpubutu : public QObject
{
    Q_OBJECT
public:
    explicit shengpubutu(QObject *parent = nullptr);
    ~shengpubutu();

    // 设置显示的外部 graphicsView
    void setGraphicsView(QGraphicsView *view);

    // 创建图像
    void createPingImages(const QVector<std::vector<uint8_t>> &portData,
                          const QVector<std::vector<uint8_t>> &starboardData);

    void doBottomTrack();  // 新增：底部追踪
    //移动平均平滑水线点
    QVector<int> smoothLine(const QVector<int>& line, int window = 3);
    //寻找更合适的定位
    int detectBottomByEnergyRobust(const std::vector<uint8_t>& samples,int smoothSampleWin = 7,int smoothDeltaWin = 5, double deltaThreshold = 0.001, int consecRequired = 3);

    //简单移动平均
    static std::vector<double> movingAverage(const std::vector<double>& data, int win);
    //
    int findAppropriateStartIdx(const std::vector<uint8_t>& samples, int startIdx);

    //新增融合
    QImage createMergedImage();

    void showMergedImage();

protected:
    //不会生效
    void wheelEvent(QWheelEvent *event);

    void updateViewScale();

private:
    QGraphicsView *view = nullptr;
    QGraphicsScene *scene = nullptr;
    QGraphicsPixmapItem *portItem = nullptr;
    QGraphicsPixmapItem *starboardItem = nullptr;
    qreal currentScale = 1.0;

    QVector<std::vector<uint8_t>> portDataAll;      // 存储原始数据
    QVector<std::vector<uint8_t>> starboardDataAll; // 存储原始数据

    QVector<BottomPoint> bottomTrack;  // 存储底部追踪结果
    QVector<int> portBottomLine;       //存储左舷原始索引位置
    QVector<int> starboardBottomLine;   //存储右舷原始索引位置
    QVector<int> portsmoothLine;       //存储左舷平滑后索引位置
    QVector<int> starboardsmoothLine;   //存储右舷平滑后索引位置
    QVector<QGraphicsItem*> bottomLineItems;
};

#endif // SHENGPUBUTU_H
