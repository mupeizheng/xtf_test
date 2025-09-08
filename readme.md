# xtf_test

## 项目简介
`xtf_test` 是一个基于 Qt 的 XTF 文件解析和测试项目，主要功能包括：
- 解析 XTF 声呐数据文件
- 左右舷数据可视化显示
- 多通道数据处理与测试

适用于 Windows 平台，Qt 5.15.2 + MinGW 构建。

---

## 项目结构

xtf_test/<br>
 ├── build/                  # 构建输出目录（忽略推送）<br>
 ├── main.cpp                # 主程序入口<br>
 ├── mainwindow.*            # 主窗口代码<br>
 ├── shengpubutu.*           # 声呐图像处理类<br>
 ├── shengtutest.*           # 测试类<br>
 ├── xtf.h                   # XTF 数据结构定义<br>
 ├── xtf_test.pro            # Qt 项目文件<br>
 └── README.md               # 项目说明<br>

---

## 使用说明
1. 使用 Qt Creator 打开 `xtf_test.pro`  
2. 选择构建套件（MinGW 或 MSVC）  
3. 构建并运行  
4. 导入 XTF 文件进行测试和可视化

---

## 注意事项
- `build/` 目录及生成文件不需要推送到 GitHub  
- 建议在 `.gitignore` 中忽略以下内容：

build/<br>
 *.o<br>
 *.exe<br>
 *.user<br>
 *.pro.user<br>
 *.qmake.stash

---

