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

    //记录上一ping有点采样点位置
    int prePortPing = -1;
    int preStarboardPing = -1;
    int maxJump = 150;

    // ---- 左舷 ----
    for (int ping = 0 ; ping < portDataAll.size(); ++ping) {
        const std::vector<uint8_t>& samples = portDataAll[ping];
        int idx = -1;
        int CustomStartIdx = static_cast<int>(samples.size() * 0.7);
        int PortPingPos = findAppropriateStartIdx(samples, CustomStartIdx);//(int)samples.size()*0.75
        // qDebug()<<ping<<":   "<<"PortPingPos: "<<PortPingPos;
        for (int i = PortPingPos; i < (int)samples.size(); ++i) {
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

        if(ping == 0){
            if(idx != -1)
                prePortPing = idx;
            else
                prePortPing = samples.size() - 1;
        }else{
            if(abs(idx - prePortPing) > maxJump || idx == -1){
                // 偏差过大 → 重新检测
                int candidate = -1;
                int minPos = (idx>=prePortPing) ? prePortPing : idx;
                minPos = minPos>=PortPingPos ? PortPingPos : minPos;
                int maxPos = (idx>=prePortPing) ? idx : prePortPing;
                int bestMean = INT_MAX;  // 当前最小均值，初始设为最大
                // std::vector<int> meanValues; // 可选：存放所有均值（调试或分析用）
                // for (int i = minPos; i <= maxPos; ++i) {
                for (int i = minPos; i <= maxPos; ++i) {
                    if (samples[i] <= 3) {
                        // 计算周围灰度均值
                        int localSum = 0;
                        int localCount = 0;
                        int win = 120;  // 窗口大小，可调
                        for (int k = 0; k < win; ++k) {
                            int pos = i + k;   // 从当前位置 i 往后
                            if(pos<0) pos = 0;
                            if (pos < (int)samples.size()) {
                                localSum += samples[pos];
                                localCount++;
                            }
                        }
                        int localMean = localSum / std::max(1, localCount);

                        // 经验阈值（越小越白）
                        if (localMean <= 5) {  // 阈值可调
                            // candidate = i;
                            // break;
                            if (localMean <= bestMean) {
                                bestMean = localMean;
                                candidate = i;
                            }
                        }
                    }
                }
                if (candidate != -1) {
                    idx = candidate;
                }
            }
        }
        qDebug()<<"idx："<<idx<<"  preport: "<<prePortPing;
        if (idx < 0) idx = samples.size() - 1;
        portBottomLine.append(idx);
        prePortPing = idx;

        if (!portDataAll.isEmpty()) {
            if (ping % 100 == 0 || ping == portDataAll.size()) {
                double progress = static_cast<double>(ping) / portDataAll.size() * 100.0;
                QString str = QString("左舷进度：%1%").arg(progress, 0, 'f', 2);
                qDebug() << str;
            }
        }
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
    //新增，移动平均平滑水线点
    portBottomLine = smoothLine(portBottomLine, 100);
    starboardBottomLine = smoothLine(starboardBottomLine, 100);

    // ---- 绘制水线 ----
    if (!scene) return;

    QPen pen(Qt::red, 5);  // 红色粗线
    int portWidth = portDataAll[0].size();

    // 左舷连线
    // if (!portBottomLine.isEmpty()) {
    //     QPainterPath path;
    //     path.moveTo(portBottomLine[0], 0);
    //     for (int ping = 1; ping < portBottomLine.size(); ++ping) {
    //         path.lineTo(portBottomLine[ping], ping);
    //     }
    //     auto item = scene->addPath(path, pen);
    //     bottomLineItems.append(item);
    // }
    //左舷绘制曲线
    if (!portBottomLine.isEmpty()) {
        QPainterPath path;
        path.moveTo(portBottomLine[0], 0);

        for (int ping = 1; ping < portBottomLine.size() - 1; ++ping) {
            // 当前点
            QPointF p1(portBottomLine[ping], ping);
            // 下一个点
            QPointF p2(portBottomLine[ping + 1], ping + 1);
            // 控制点取两点中点，实现平滑过渡
            QPointF ctrl((p1.x() + p2.x()) / 2.0, (p1.y() + p2.y()) / 2.0);

            path.quadTo(p1, ctrl);
        }

        auto item = scene->addPath(path, pen);
        bottomLineItems.append(item);
    }

    // 右舷连线
    // if (!starboardBottomLine.isEmpty()) {
    //     QPainterPath path;
    //     path.moveTo(portWidth + starboardBottomLine[0], 0);
    //     for (int ping = 1; ping < starboardBottomLine.size(); ++ping) {
    //         path.lineTo(portWidth + starboardBottomLine[ping], ping);
    //     }
    //     auto item = scene->addPath(path, pen);
    //     bottomLineItems.append(item);
    // }
    //右舷绘制曲线
    if (!starboardBottomLine.isEmpty()) {
        QPainterPath path;
        path.moveTo(portWidth + starboardBottomLine[0], 0);

        for (int ping = 1; ping < starboardBottomLine.size() - 1; ++ping) {
            QPointF p1(portWidth + starboardBottomLine[ping], ping);
            QPointF p2(portWidth + starboardBottomLine[ping + 1], ping + 1);
            QPointF ctrl((p1.x() + p2.x()) / 2.0, (p1.y() + p2.y()) / 2.0);

            path.quadTo(p1, ctrl);
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

QVector<int> shengpubutu::smoothLine(const QVector<int> &line, int window)
{
    QVector<int> smoothed(line.size());
    for (int i = 0; i < line.size(); ++i) {
        int sum = 0, count = 0;
        for (int j = std::max(0, i-window); j <= std::min(i+window, line.size()-1); ++j) {
            sum += line[j];
            count++;
        }
        smoothed[i] = sum / count;
    }
    return smoothed;
}

int shengpubutu::detectBottomByEnergyRobust(const std::vector<uint8_t> &samples, int smoothSampleWin, int smoothDeltaWin, double deltaThreshold, int consecRequired)
{
    int N = samples.size();
    if (N == 0) return -1;

    // 1) 将 samples 转为 double 并做样本平滑（只向后看的需求可以用单向滤波，此处用对称移动平均）
    std::vector<double> s(N);
    for (int i = 0; i < N; ++i) s[i] = static_cast<double>(samples[i]);

    std::vector<double> s_smooth = movingAverage(s, smoothSampleWin);

    // 2) 计算总能量（反转后直接使用 s_smooth）
    double totalEnergy = 0.0;
    for (double v : s_smooth) totalEnergy += v;
    if (totalEnergy <= 0.0) return N - 1;

    // 3) 计算累计比例 ratio[i]
    std::vector<double> ratio(N, 0.0);
    double cum = 0.0;
    for (int i = 0; i < N; ++i) {
        cum += s_smooth[i];
        ratio[i] = cum / totalEnergy;
    }

    // 4) 计算 delta（相邻 ratio 差分），从 i=1 开始
    std::vector<double> delta(N, 0.0);
    for (int i = 1; i < N; ++i) delta[i] = ratio[i] - ratio[i - 1];

    // 5) 对 delta 做短窗口平滑，减少尖刺影响
    std::vector<double> delta_smooth = movingAverage(delta, smoothDeltaWin);

    // 6) 找第一个连续 consecRequired 个点满足 delta_smooth < deltaThreshold
    int bottomIdx = -1;
    int consec = 0;
    for (int i = 1; i < N; ++i) {
        if (delta_smooth[i] < deltaThreshold) {
            consec++;
            if (consec >= consecRequired) {
                bottomIdx = i - (consecRequired - 1); // 回溯到第一个满足点
                break;
            }
        } else {
            consec = 0;
        }
    }

    // 7) fallback：若没找到，尝试更宽阈值或取最后
    if (bottomIdx == -1) {
        // 尝试用较大阈值再找一次（稳妥策略）
        double wider = deltaThreshold * 2.5;
        consec = 0;
        for (int i = 1; i < N; ++i) {
            if (delta_smooth[i] < wider) {
                consec++;
                if (consec >= consecRequired) {
                    bottomIdx = i - (consecRequired - 1);
                    break;
                }
            } else {
                consec = 0;
            }
        }
    }

    if (bottomIdx == -1) bottomIdx = N - 1;
    return bottomIdx;
}

std::vector<double> shengpubutu::movingAverage(const std::vector<double> &data, int win)
{
    int n = (int)data.size();
    std::vector<double> out(n, 0.0);
    if (n == 0) return out;
    if (win <= 1) {
        for (int i = 0; i < n; ++i) out[i] = data[i];
        return out;
    }
    int half = win / 2;
    double sum = 0.0;
    int left = 0;
    for (int i = 0; i < n; ++i) {
        // 增加 new right
        sum += data[i];
        // 移动 left 指针，保证窗口大小不超过 win
        if (i - left + 1 > win) {
            sum -= data[left];
            left++;
        }
        int curWindowSize = i - left + 1;
        out[i] = sum / curWindowSize;
    }
    // 可选：对前面不足窗口的区域再做一次 symmetric 平滑，但上面已处理边界
    return out;
}

int shengpubutu::findAppropriateStartIdx(const std::vector<uint8_t> &samples, int startIdx)
{
    if (samples.empty()) return -1;

    int idx = startIdx;

    // 如果当前位置强度值过小，则尝试往前寻找更合适的点
    if (samples[startIdx] == 0 || samples[startIdx-1] == 0) {
        int candidate = -1;

        // 1. 优先往前找第一个不为0的点
        for (int i = startIdx; i >= 0; --i) {
            if (samples[i] >= 3) {
                candidate = i+1;
                break;
            }
        }

        // 2. 如果没有严格为0的点，找一个接近0的点（阈值 >=5）
        if (candidate == -1) {
            for (int i = startIdx; i >= 0; --i) {
                if (samples[i] >= 5) {
                    candidate = i+1;
                    break;
                }
            }
        }

        if (candidate != -1) {
            idx = candidate;
        }
    }

    return idx;
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
