#include "shengpubutu.h"
#include <QWheelEvent>
#include <QTransform>
#include <QDebug>
#include <algorithm>

shengpubutu::shengpubutu(QObject *parent)
    : QObject(parent)
{
    scene = new QGraphicsScene(this);
}

shengpubutu::~shengpubutu()
{
}

void shengpubutu::setGraphicsView(QGraphicsView *v)
{
    view = v;
    if (view)
    {
        view->setScene(scene);
        view->setRenderHint(QPainter::Antialiasing);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void shengpubutu::createPingImages(const QVector<std::vector<uint8_t>> &portData,
                                   const QVector<std::vector<uint8_t>> &starboardData)
{

    scene->clear();

    portDataAll = portData;          // 保存原始数据
    starboardDataAll = starboardData;

    auto makeImage = [](const QVector<std::vector<uint8_t>> &data) {
        int height = data.size();
        int width  = data[0].size();
        QImage img(width, height, QImage::Format_Grayscale8);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int a = 255 - data[y][x];
                img.setPixel(x, y, qRgb(a, a, a));
            }
        }
        return img;
    };

    QImage portImg = makeImage(portData);
    QImage starImg = makeImage(starboardData);

    portItem = scene->addPixmap(QPixmap::fromImage(portImg));
    portItem->setPos(0, 0);

    starboardItem = scene->addPixmap(QPixmap::fromImage(starImg));
    starboardItem->setPos(portImg.width(), 0);

    scene->setSceneRect(0, 0, portImg.width() + starImg.width(),
                        std::max(portImg.height(), starImg.height()));

    currentScale = 1.0;
    updateViewScale();
}

void shengpubutu::doBottomTrack()
{
    if (portDataAll.isEmpty() || starboardDataAll.isEmpty())
        return;

    // 删除之前的水线（避免重复绘制）
    for (auto item : bottomLineItems) {
        scene->removeItem(item);
        delete item;
    }
    bottomLineItems.clear();

    // ---- 左舷 ----
    for (int ping = 0 ; ping < portDataAll.size(); ++ping) {
        const std::vector<uint8_t>& samples = portDataAll[ping];
        int idx = -1;
        for (int i = (int)samples.size()*0.8; i < (int)samples.size(); ++i) {
            if (samples[i] == 0) {
                int ZeroCount = 0;
                int checkRange = std::min(50, (int)samples.size() - i);
                for (int k = 0; k < checkRange; ++k) {
                    if (samples[i + k] < 1) ZeroCount++;
                }
                if (ZeroCount > checkRange*0.9) {
                    idx = i;
                    break;
                }
            }
        }

        if (idx < 0) idx = samples.size() - 1;
        portBottomLine.append(idx);
    }
    // ---- 右舷 ----
    for (int ping = 0; ping < starboardDataAll.size(); ++ping) {
        const std::vector<uint8_t>& samples = starboardDataAll[ping];
        int idx = -1;
        for (int i = 0; i < (int)samples.size()*0.4; ++i) {
            if (samples[i] > 0) {
                int nonZeroCount = 0;
                int checkRange = std::min(50, (int)(samples.size()*0.4) - i);
                for (int k = 0; k < checkRange; ++k) {
                    if (samples[i + k] > 0) nonZeroCount++;
                }
                if (nonZeroCount > checkRange*0.9) {
                    idx = i;
                    break;
                }
            }
        }
        if (idx < 0) idx = 0;
        starboardBottomLine.append(idx);
    }
    // ---- 绘制水线 ----
    if (!scene) return;

    QPen pen(Qt::red, 2);  // 红色粗线
    int portWidth = portDataAll[0].size();

    // 左舷连线
    if (!portBottomLine.isEmpty()) {
        QPainterPath path;
        path.moveTo(portBottomLine[0], 0);
        for (int ping = 1; ping < portBottomLine.size(); ++ping) {
            path.lineTo(portBottomLine[ping], ping);
        }
        auto item = scene->addPath(path, pen);
        bottomLineItems.append(item);
    }

    // 右舷连线
    if (!starboardBottomLine.isEmpty()) {
        QPainterPath path;
        path.moveTo(portWidth + starboardBottomLine[0], 0);
        for (int ping = 1; ping < starboardBottomLine.size(); ++ping) {
            path.lineTo(portWidth + starboardBottomLine[ping], ping);
        }
        auto item = scene->addPath(path, pen);
        bottomLineItems.append(item);
    }
    // qDebug()<<"====================";
    // for(int i=0;i<50;++i){
    //     qDebug()<<"portBottomLine ["<<i<<"]: "<<portBottomLine[i];
    // }
    // qDebug()<<"====================";
    // for(int i=0;i<50;++i){
    //     qDebug()<<"starboardBottomLine ["<<i<<"]: "<<starboardBottomLine[i];
    // }
    qDebug() << "底部追踪完成，已绘制曲线";
}

void shengpubutu::wheelEvent(QWheelEvent *event)
{
    if (!view) return;

    QPointF mouseViewPos = event->position();
    QPointF mouseScenePos = view->mapToScene(mouseViewPos.toPoint());

    qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    currentScale *= factor;
    currentScale = std::clamp(currentScale, 0.1, 10.0);

    view->scale(factor, factor);

    QPointF delta = mouseScenePos - view->mapToScene(mouseViewPos.toPoint());
    view->translate(delta.x(), delta.y());

    event->accept();
}

void shengpubutu::updateViewScale()
{
    if (!view || !scene) return;
    QRectF rect = scene->sceneRect();
    if (rect.width() <= 0 || rect.height() <= 0) return;

    qreal viewWidth = view->viewport()->width();
    qreal scale = viewWidth / rect.width(); // 宽度铺满

    currentScale = scale;

    QTransform transform;
    transform.scale(currentScale, currentScale);
    view->setTransform(transform);
}
