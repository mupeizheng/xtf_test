#include "shengtutest.h"
#include <QFileDialog>
#include <QDebug>
#include <fstream>
#include <cmath>

shengtutest::shengtutest(QObject* parent)
    : QObject(parent)
{
}

shengtutest::~shengtutest()
{
}

void shengtutest::parseXtfHeader(const QString &filePath,
                                 QVector<std::vector<uint8_t>> &portData,
                                 QVector<std::vector<uint8_t>> &starboardData)
{
    std::ifstream file(filePath.toStdString(), std::ios::binary);
    if (!file) {
        qWarning() << "无法打开文件：" << filePath;
        return;
    }

    XTFFILEHEADER header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(XTFFILEHEADER));

    if (header.FileFormat != 0x7B) {
        qWarning() << "非标准 XTF 文件！";
        return;
    }

    qDebug() << "Header.NumberOfSonarChannels:" << header.NumberOfSonarChannels;

    if (header.NumberOfSonarChannels > 6) {
        int moveIndex = static_cast<int>(ceil((header.NumberOfSonarChannels - 6) / 8.0) * 1024);
        file.seekg(moveIndex, std::ios::cur);
    }

    portData.clear();
    starboardData.clear();

    while (!file.eof()) {
        XTFCHANHEADER chanHeader{};
        file.read(reinterpret_cast<char*>(&chanHeader), sizeof(XTFCHANHEADER));
        if (file.gcount() != sizeof(XTFCHANHEADER)) break;
        if (chanHeader.MagicNumber != 0xFACE) break;
        // qDebug()<<"XTFCHAHEADER size:"<<chanHeader.NumBytesThisRecord;
        switch (chanHeader.HeaderType) {
            case 0: { // 侧扫数据
                XTFPINGHEADER xtfpingHeader{};
                file.seekg(-static_cast<int>(sizeof(XTFCHANHEADER)), std::ios::cur);
                file.read(reinterpret_cast<char*>(&xtfpingHeader), sizeof(XTFPINGHEADER));

                for (int i = 0; i < header.NumberOfSonarChannels && i < 6; i++) {
                    XTFPINGCHANHEADER xtfpingChanHeader{};
                    file.read(reinterpret_cast<char*>(&xtfpingChanHeader), sizeof(XTFPINGCHANHEADER));

                    std::vector<uint8_t> rawData(xtfpingChanHeader.NumSamples, 0);

                    if (header.ChanInfo[i].BytesPerSample == 1) {
                        file.read(reinterpret_cast<char*>(rawData.data()), xtfpingChanHeader.NumSamples);
                    } else {
                        std::vector<int16_t> rawSamples(xtfpingChanHeader.NumSamples);
                        file.read(reinterpret_cast<char*>(rawSamples.data()), xtfpingChanHeader.NumSamples * sizeof(int16_t));
                        for (int k = 0; k < xtfpingChanHeader.NumSamples; ++k) {
                            rawData[k] = 255 * rawSamples[k] / 32768;
                        }
                        // //2字节样本：16位有符号整数 [-32768, 32767] 归一化更准确的写法，颜色好像不一致,最低127
                        // std::vector<int16_t> rawSamples(xtfpingChanHeader.NumSamples);
                        // file.read(reinterpret_cast<char*>(rawSamples.data()),
                        //           xtfpingChanHeader.NumSamples * sizeof(int16_t));

                        // for (int k = 0; k < xtfpingChanHeader.NumSamples; ++k) {
                        //     // 将 [-32768, 32767] 映射到 [0, 255]
                        //     int normalized = static_cast<int>(
                        //         ( (rawSamples[k] + 32768) / 65535.0 ) * 255.0
                        //         );

                        //     // 防止溢出，确保范围在 [0, 255]
                        //     if (normalized < 0) normalized = 0;
                        //     if (normalized > 255) normalized = 255;

                        //     rawData[k] = static_cast<uint8_t>(normalized);
                        // }
                    }

                    if (i == 0) portData.append(rawData);       // 左舷
                    else if (i == 1) starboardData.append(rawData); // 右舷
                }
                break;
            }
            default:
                file.seekg(chanHeader.NumBytesThisRecord, std::ios::cur);
        }
    }

    printXtfHeader(header);
}

void shengtutest::printXtfHeader(const XTFFILEHEADER &header)
{
    QString outstr;

    outstr += QString("=== 文件头基本信息 ===\n");
    outstr += QString("文件格式标识: 0x%1\n").arg(header.FileFormat, 0, 16);
    outstr += QString("系统类型: %1\n").arg(header.SystemType);
    outstr += QString("记录程序: %1\n").arg(QString::fromUtf8(QByteArray(header.RecordingProgramName, 8).trimmed()));
    outstr += QString("程序版本: %1\n").arg(QString::fromUtf8(QByteArray(header.RecordingProgramVersion, 8).trimmed()));
    outstr += QString("声纳名称: %1\n").arg(QString::fromUtf8(QByteArray(header.SonarName, 16).trimmed()));

    outstr += QString("\n=== 声纳配置 ===\n");
    outstr += QString("声纳类型码: %1\n").arg(header.SonarType);
    outstr += QString("备注: %1\n").arg(QString::fromUtf8(QByteArray(header.NoteString, 64).trimmed()));
    outstr += QString("文件名: %1\n").arg(QString::fromUtf8(QByteArray(header.ThisFileName, 64).trimmed()));

    outstr += QString("\n=== 导航设置 ===\n");
    outstr += QString("导航单位: %1 (%2)\n")
                  .arg(header.NavUnits)
                  .arg(header.NavUnits == 0 ? "米" : "经纬度");
    outstr += QString("导航延迟: %1 ms\n").arg(header.NavigationLatency);

    outstr += QString("\n=== 通道配置 ===\n");
    outstr += QString("侧扫通道数: %1\n").arg(header.NumberOfSonarChannels);
    outstr += QString("测深通道数: %1\n").arg(header.NumberOfBathymetryChannels);
    outstr += QString("片段通道数: %1\n").arg(header.NumberOfSnippetChannels);
    outstr += QString("前视阵列数: %1\n").arg(header.NumberOfForwardLookArrays);
    outstr += QString("回波强度通道数: %1\n").arg(header.NumberOfEchoStrengthChannels);
    outstr += QString("干涉通道数: %1\n").arg(header.NumberOfInterferometryChannels);

    outstr += QString("\n=== 传感器偏移 (米/度) ===\n");
    outstr += QString("导航偏移 (X/Y/Z): %1 / %2 / %3\n").arg(header.NavOffsetX).arg(header.NavOffsetY).arg(header.NavOffsetZ);
    outstr += QString("导航角度 (Yaw/Pitch/Roll): %1\n").arg(header.NavOffsetYaw);
    outstr += QString("MRU偏移 (X/Y/Z): %1 / %2 / %3\n").arg(header.MRUOffsetX).arg(header.MRUOffsetY).arg(header.MRUOffsetZ);
    outstr += QString("MRU角度 (Yaw/Pitch/Roll): %1 / %2 / %3\n").arg(header.MRUOffsetYaw).arg(header.MRUOffsetPitch).arg(header.MRUOffsetRoll);

    outstr += QString("\n=== 通道详细信息 ===\n");
    for (int i = 0; i < 6; ++i) {
        if (i < header.NumberOfSonarChannels) {
            outstr += QString("--- 通道 %1 ---\n").arg(i);
            outstr += QString("  类型: %1\n").arg(header.ChanInfo[i].TypeOfChannel);
            outstr += QString("  名称: %1\n").arg(QString::fromUtf8(QByteArray(header.ChanInfo[i].ChannelName, 16).trimmed()));
            outstr += QString("  频率: %1 Hz\n").arg(header.ChanInfo[i].Frequency);
            outstr += QString("  字节数: %1 字节\n").arg(header.ChanInfo[i].BytesPerSample);
        }
    }

    // qDebug().noquote() << outstr;
}
