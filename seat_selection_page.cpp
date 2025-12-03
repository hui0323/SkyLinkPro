#include "seat_selection_page.h"
#include "db_conn.h" // 保持你的数据库连接方式
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QDebug>

SeatSelectionPage::SeatSelectionPage(QWidget *parent) : QWidget(parent) {
    initUi();
}

void SeatSelectionPage::initUi() {
    QVBoxLayout* mainLay = new QVBoxLayout(this);

    // 1. 顶部提示
    m_infoLabel = new QLabel("请先在航班列表选择航班...");
    m_infoLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; margin-top: 10px;");
    m_infoLabel->setAlignment(Qt::AlignCenter);
    mainLay->addWidget(m_infoLabel);

    // 2. 屏幕指示条
    QLabel* screen = new QLabel("———— 机头方向 ————");
    screen->setAlignment(Qt::AlignCenter);
    screen->setStyleSheet("background: #f0f0f0; padding: 6px; color: #999; border-radius: 4px;");
    mainLay->addWidget(screen);

    // 3. 座位网格区域
    QWidget* gridWidget = new QWidget;
    QGridLayout* gl = new QGridLayout(gridWidget);
    gl->setSpacing(15);

    // 模拟 6排 x 4列 (A,B -过道- C,D)
    QStringList cols = {"A", "B", "C", "D"};
    for(int r=1; r<=6; ++r) {
        for(int c=0; c<4; ++c) {
            QPushButton* btn = new QPushButton(QString("%1%2").arg(r).arg(cols[c]));
            btn->setFixedSize(50, 50);
            btn->setCheckable(true); // 设为可选中状态
            btn->setCursor(Qt::PointingHandCursor);

            // 存入属性，方便点击时识别
            QString key = QString("%1_%2").arg(r).arg(c);
            btn->setProperty("seatKey", key);

            connect(btn, &QPushButton::clicked, this, &SeatSelectionPage::onSeatClicked);

            // 处理过道：如果是第2列(Index 2, 即C座)，往右移一点，或者在B后面加个空隙
            // 这里简单处理：直接加到Grid里
            gl->addWidget(btn, r-1, c);
            m_seatBtns[key] = btn;
        }
    }
    // 设置第2列(B座)和第3列(C座)之间的间距为过道
    gl->setColumnMinimumWidth(2, 40);

    mainLay->addWidget(gridWidget, 0, Qt::AlignHCenter);

    // 4. 底部图例
    QHBoxLayout* legendLay = new QHBoxLayout;
    auto addLegend = [](QString text, QString color) {
        QLabel* l = new QLabel(text);
        l->setStyleSheet("background:"+color+"; color: white; padding: 4px 8px; border-radius: 4px;");
        return l;
    };
    legendLay->addStretch();
    legendLay->addWidget(addLegend("可选", "#FFFFFF; color: #333; border: 1px solid #999"));
    legendLay->addWidget(addLegend("已选", "#0086F6"));
    legendLay->addWidget(addLegend("已售", "#CCCCCC"));
    legendLay->addStretch();
    mainLay->addLayout(legendLay);

    // 5. 底部确认按钮
    m_btnConfirm = new QPushButton("确认选座并支付");
    m_btnConfirm->setStyleSheet("background: #0086F6; color: white; font-size: 16px; padding: 12px; border-radius: 6px; font-weight: bold;");
    m_btnConfirm->setEnabled(false); // 初始不可点
    connect(m_btnConfirm, &QPushButton::clicked, this, &SeatSelectionPage::onConfirmClicked);

    mainLay->addWidget(m_btnConfirm);
    mainLay->addStretch();
}

void SeatSelectionPage::loadFlightSeatInfo(int flightId, QString flightCode, double price) {
    m_flightId = flightId;
    m_flightCode = flightCode;
    m_price = price;
    m_selectedSeatKey = "";

    m_infoLabel->setText(QString("正在为航班 %1 选座").arg(flightCode));
    m_btnConfirm->setText(QString("确认支付 ¥%1").arg(price));
    m_btnConfirm->setEnabled(false);

    // 1. 重置所有按钮为“可选”状态
    for(auto btn : m_seatBtns) {
        btn->setEnabled(true);
        btn->setChecked(false);
        btn->setStyleSheet("QPushButton { background: white; border: 1px solid #999; border-radius: 4px; color: #333; }"
                           "QPushButton:checked { background: #0086F6; color: white; border: none; }");
    }

    // 2. 查库：获取已占用的座位
    // 假设 bookings 表有: flight_id, seat_row (int), seat_col (int 0=A,1=B...)
    QSqlDatabase db = DbManager::getConn();
    if(db.open()) {
        QSqlQuery q(db);
        q.prepare("SELECT seat_row, seat_col FROM bookings WHERE flight_id = ? AND status != 'Cancelled'");
        q.addBindValue(flightId);

        if(q.exec()) {
            while(q.next()) {
                int r = q.value(0).toInt();
                int c = q.value(1).toInt();
                QString key = QString("%1_%2").arg(r).arg(c);

                if(m_seatBtns.contains(key)) {
                    QPushButton* btn = m_seatBtns[key];
                    btn->setEnabled(false); // 禁用
                    btn->setStyleSheet("background: #E0E0E0; border: 1px solid #DDD; color: #999;"); // 灰色外观
                    btn->setText(btn->text() + "\n(已售)"); // 可选：显示已售
                }
            }
        }
        db.close();
    }
}

void SeatSelectionPage::onSeatClicked() {
    QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());
    if(!clickedBtn) return;

    QString key = clickedBtn->property("seatKey").toString();

    // 如果点击的是当前已选的，则是取消选择
    if (m_selectedSeatKey == key && !clickedBtn->isChecked()) {
        m_selectedSeatKey = "";
        m_btnConfirm->setEnabled(false);
        return;
    }

    // 单选逻辑：取消上一个选中的按钮
    if (!m_selectedSeatKey.isEmpty() && m_seatBtns.contains(m_selectedSeatKey)) {
        m_seatBtns[m_selectedSeatKey]->setChecked(false);
    }

    m_selectedSeatKey = key;
    clickedBtn->setChecked(true);
    m_btnConfirm->setEnabled(true); // 激活支付按钮
}

void SeatSelectionPage::onConfirmClicked() {
    if(m_flightId == -1 || m_selectedSeatKey.isEmpty()) return;

    QStringList parts = m_selectedSeatKey.split("_");
    if(parts.size() != 2) return;

    int row = parts[0].toInt();
    int col = parts[1].toInt();

    emit confirmBooking(m_flightId, row, col, m_price);
}
