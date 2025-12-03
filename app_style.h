#ifndef APP_STYLE_H
#define APP_STYLE_H

#include <QString>

class AppStyle {
public:
    static QString getQSS() {
        return R"(
            /* 全局背景与字体 */
            QWidget {
                font-family: "Microsoft YaHei UI", "Segoe UI";
                font-size: 14px;
                color: #333333;
                background-color: #F5F7FA;
            }

            /* --- 输入框美化 --- */
            QLineEdit {
                border: 1px solid #DCDFE6;
                border-radius: 6px;
                padding: 10px 12px;
                background: #FFFFFF;
                selection-background-color: #409EFF;
            }
            QLineEdit:focus { border: 1px solid #409EFF; background: #ECF5FF; }
            QLineEdit::placeholder { color: #C0C4CC; }
            QLineEdit[readOnly="true"] { background: #FAFAFA; color: #606266; }

            /* --- 按钮美化 --- */
            /* 主按钮 (蓝色) */
            QPushButton[class="primary"] {
                background-color: #0086F6;
                color: white;
                border-radius: 6px;
                padding: 8px 20px;
                border: none;
                font-weight: bold;
            }
            QPushButton[class="primary"]:hover { background-color: #339DFA; }
            QPushButton[class="primary"]:pressed { background-color: #006AD6; }

            /* 次要按钮 (白色) */
            QPushButton[class="secondary"] {
                background-color: #FFFFFF;
                border: 1px solid #DCDFE6;
                color: #606266;
                border-radius: 6px;
                padding: 8px 20px;
            }
            QPushButton[class="secondary"]:hover { color: #409EFF; border-color: #C6E2FF; background-color: #ECF5FF; }

            /* 文本链接按钮 */
            QPushButton[class="textBtn"] {
                background: transparent; border: none; color: #0086F6;
            }
            QPushButton[class="textBtn"]:hover { text-decoration: underline; }

            /* --- QTabWidget 美化 --- */
            QTabWidget::pane { border: none; background: white; }
            QTabBar::tab {
                background: #E4E7ED; color: #909399;
                padding: 12px 30px; margin-right: 2px;
                border-top-left-radius: 6px; border-top-right-radius: 6px;
            }
            QTabBar::tab:selected { background: #FFFFFF; color: #0086F6; font-weight: bold; }

            /* --- 表格美化 --- */
            QTableWidget {
                background-color: white; border: none;
                gridline-color: #EBEEF5;
            }
            QHeaderView::section {
                background-color: #FAFAFA; border: none;
                border-bottom: 1px solid #EBEEF5;
                padding: 12px; color: #606266; font-weight: bold;
            }
            QTableWidget::item { padding: 10px; border-bottom: 1px solid #EBEEF5; }
            QTableWidget::item:selected { background-color: #ECF5FF; color: #409EFF; }
        )";
    }
};

#endif // APP_STYLE_H

