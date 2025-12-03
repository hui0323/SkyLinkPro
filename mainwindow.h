#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QTableWidget>
#include <QStackedWidget>
#include <QLabel>
#include "login_dialog.h"
#include <QHBoxLayout>
#include "db_conn.h" // 保持你的数据库连接引用

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const UserInfo& user, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectOrigin();
    void onSelectDest();
    void onSwapCity();
    void onSearchFlights();

    // 点击列表中的“去选座”
    void onPreBookClicked(const QString& flightId, const QString& flightCode, double price, const QDateTime& depTime);

    // 切换Tab时触发，用于刷新“我的订单”
    void onTabChanged(int index);

private:
    UserInfo m_user;

    QTabWidget* m_mainTab;
    QStackedWidget* m_searchStack;

    // --- 搜索页控件 ---
    QLineEdit *m_leOrigin, *m_leDest;
    QDateEdit *m_dateEdit;
    QTableWidget *m_flightListTable;
    QWidget* m_dateBarContainer;
    QHBoxLayout* m_dateBarLayout;

    // --- 订单页控件 (新增) ---
    QTableWidget* m_orderTable;

    // --- 公共控件 ---
    QLabel* m_lblUserBalance;
    class SeatSelectionPage* m_seatPage;

    // --- 初始化函数 ---
    void setupUi();
    void setupSearchTab(); // 搜索
    void setupSeatTab();   // 选座
    void setupOrderTab();  // 我的订单 (新增)
    void setupStatusTab(); // 动态

    // --- 逻辑函数 ---
    void loadFlightList(QString from, QString to, const QDate& date);
    void loadOrderList(); // 加载我的订单 (新增)
    void updateDateBar(QString from, QString to, const QDate& centerDate);
    void updateBalanceUI();
};

#endif // MAINWINDOW_H
