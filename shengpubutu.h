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
    QVector<int> portBottomLine;
    QVector<int> starboardBottomLine;
    QVector<QGraphicsItem*> bottomLineItems;
};

#endif // SHENGPUBUTU_H
