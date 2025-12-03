#include <QApplication>
#include "app_style.h"
#include "login_dialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // 启用高分屏支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    // 1. 加载全局样式 (一行代码，全应用变美)
    a.setStyleSheet(AppStyle::getQSS());
    a.setFont(QFont("Microsoft YaHei UI", 10));

    // 2. 显示登录窗口
    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        // 3. 登录成功，获取用户信息并进入主界面
        UserInfo user = login.getLoggedUser();

        MainWindow* w = new MainWindow(user);
        w->show();

        return a.exec();
    }

    return 0;
}
