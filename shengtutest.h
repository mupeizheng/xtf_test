#pragma once

#include "xtf.h"
#include <QObject>
#include <QVector>
#include <vector>

class shengtutest : public QObject {
    Q_OBJECT

public:
    explicit shengtutest(QObject* parent = nullptr);
    ~shengtutest();

    // 解析 XTF 文件头和侧扫数据，返回左右舷数据
    void parseXtfHeader(const QString &filePath,
                        QVector<std::vector<uint8_t>> &portData,
                        QVector<std::vector<uint8_t>> &starboardData);

    void printXtfHeader(const XTFFILEHEADER &header);

};
