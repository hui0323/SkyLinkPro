#ifndef SEAT_SELECTION_PAGE_H
#define SEAT_SELECTION_PAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>

class SeatSelectionPage : public QWidget {
    Q_OBJECT
public:
    explicit SeatSelectionPage(QWidget *parent = nullptr);

    // 核心：加载航班座位数据（查库看哪些被占了）
    void loadFlightSeatInfo(int flightId, QString flightCode, double price);

signals:
    // 当用户点击“确认并支付”时，发信号给 MainWindow 处理扣款
    void confirmBooking(int flightId, int row, int col, double price);

private slots:
    void onSeatClicked();    // 点击座位时
    void onConfirmClicked(); // 点击底部确认按钮时

private:
    void initUi(); // 初始化界面布局

    int m_flightId = -1;
    QString m_flightCode;
    double m_price = 0.0;

    QLabel* m_infoLabel;     // 顶部提示信息
    QPushButton* m_btnConfirm; // 底部确认按钮

    // 存储座位按钮的 Map，Key格式为 "行_列" (例如 "1_0" 代表1排A座)
    QMap<QString, QPushButton*> m_seatBtns;

    QString m_selectedSeatKey; // 当前选中的座位Key
};

#endif // SEAT_SELECTION_PAGE_H
