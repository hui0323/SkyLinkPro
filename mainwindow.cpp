#include "mainwindow.h"
#include "db_conn.h"
#include "city_selector.h"
#include "seat_selection_page.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QToolButton>

MainWindow::MainWindow(const UserInfo& user, QWidget *parent)
    : QMainWindow(parent), m_user(user)
{
    resize(1200, 800);
    setWindowTitle("SkyLink Pro - 欢迎您，" + m_user.realName);
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget* central = new QWidget;
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);

    // 顶部栏
    QWidget* topBar = new QWidget;
    topBar->setStyleSheet("background: white; border-bottom: 1px solid #ddd;");
    QHBoxLayout* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(20, 10, 20, 10);

    QLabel* lblLogo = new QLabel("✈ SkyLink Pro");
    lblLogo->setStyleSheet("font-size: 18px; font-weight: bold; color: #0086F6;");

    m_lblUserBalance = new QLabel;
    updateBalanceUI();

    topLay->addWidget(lblLogo);
    topLay->addStretch();
    topLay->addWidget(m_lblUserBalance);
    mainLayout->addWidget(topBar);

    // 主Tab
    m_mainTab = new QTabWidget;
    m_mainTab->setDocumentMode(true);
    connect(m_mainTab, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // 初始化选座页
    m_seatPage = new SeatSelectionPage;
    setupSeatTab();

    // 手动控制Tab顺序（0:搜索 1:选座 2:订单 3:动态）
    setupSearchTab(); // 搜索页（会自动添加到m_mainTab）
    m_mainTab->addTab(m_seatPage, "💺 在线选座");
    setupOrderTab();  // 订单页（会自动添加到m_mainTab）
    setupStatusTab(); // 动态页（会自动添加到m_mainTab）

    mainLayout->addWidget(m_mainTab);
    m_mainTab->setCurrentIndex(0); // 默认显示搜索页
}

void MainWindow::onTabChanged(int index) {
    if (index == 2) { // 订单页是第2个Tab（索引从0开始）
        loadOrderList();
    }
}

// 我的订单
void MainWindow::setupOrderTab() {
    QWidget* page = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(page);

    QLabel* title = new QLabel("我的历史订单");
    title->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");

    m_orderTable = new QTableWidget;
    m_orderTable->setColumnCount(6);
    m_orderTable->setHorizontalHeaderLabels({"订单号", "航班号", "出发 - 到达", "起飞时间", "座位", "金额"});
    m_orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_orderTable->verticalHeader()->setVisible(false);
    m_orderTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    lay->addWidget(title);
    lay->addWidget(m_orderTable);

    m_mainTab->addTab(page, "📋 我的订单");
}

void MainWindow::loadOrderList() {
    QSqlDatabase db = DbManager::getConn();
    if(!db.open()) return;

    QString sql = "SELECT b.id, f.flight_code, f.origin, f.destination, f.dep_time, b.seat_row, b.seat_col, b.price "
                  "FROM bookings b "
                  "JOIN flights f ON b.flight_id = f.id "
                  "WHERE b.user_id = :uid "
                  "ORDER BY b.id DESC";

    QSqlQuery q(db);
    q.prepare(sql);
    q.bindValue(":uid", m_user.id);
    q.exec();

    m_orderTable->setRowCount(0);
    int row = 0;
    while(q.next()) {
        m_orderTable->insertRow(row);

        QString orderId = q.value(0).toString();
        QString flightCode = q.value(1).toString();
        QString route = q.value(2).toString() + " -> " + q.value(3).toString();
        QString time = q.value(4).toDateTime().toString("yyyy-MM-dd HH:mm");

        // 修复：删除未定义的getSeatColName，直接用数字转字母
        QString colChar = QString(QChar('A' + q.value(6).toInt())); // 0→A、1→B
        QString seat = QString("%1排%2座").arg(q.value(5).toString()).arg(colChar);

        QString price = "¥" + q.value(7).toString();

        m_orderTable->setItem(row, 0, new QTableWidgetItem(orderId));
        m_orderTable->setItem(row, 1, new QTableWidgetItem(flightCode));
        m_orderTable->setItem(row, 2, new QTableWidgetItem(route));
        m_orderTable->setItem(row, 3, new QTableWidgetItem(time));
        m_orderTable->setItem(row, 4, new QTableWidgetItem(seat));
        m_orderTable->setItem(row, 5, new QTableWidgetItem(price));

        row++;
    }
    db.close();
}

void MainWindow::setupSeatTab() {
    connect(m_seatPage, &SeatSelectionPage::confirmBooking, this, [=](int fId, int row, int col, double price){
        if (m_user.balance < price) {
            QMessageBox::warning(this, "余额不足", "您的余额不足，请充值。");
            return;
        }
        // 【核心修改】这里请求一个 unique=true 的独立连接
        // 这样这个 db 对象就独占一个 ODBC 连接，绝对不会报“函数序列错误”
        QSqlDatabase db = DbManager::getConn(true);

        if (!db.isOpen()) {
            QMessageBox::critical(this, "错误", "无法连接数据库！");
            return;
        }
        // 2. 开启事务（必须在连接打开后执行）
        if (!db.transaction()) {
            QMessageBox::critical(this, "错误", "事务开启失败：" + db.lastError().text());
            DbManager::removeConn(db); // 记得移除
            return;
        }
        bool ok = true;
        QSqlError lastErr;
        // ========== 操作1：插入订单（使用这个独立的db连接） ==========
        QSqlQuery insertQuery(db); // 传入 db
        insertQuery.prepare("INSERT INTO bookings (user_id, flight_id, seat_row, seat_col, status, price) "
                            "VALUES (:uid, :fid, :row, :col, :status, :price)");
        insertQuery.bindValue(":uid", m_user.id);
        insertQuery.bindValue(":fid", fId);
        insertQuery.bindValue(":row", row);
        insertQuery.bindValue(":col", col);
        insertQuery.bindValue(":status", "Paid");
        insertQuery.bindValue(":price", price);
        if (!insertQuery.exec()) {
            ok = false;
            lastErr = insertQuery.lastError();
            qDebug() << "Insert failed:" << lastErr.text();
        }
        // ========== 操作2：更新余额（使用这个独立的db连接） ==========
        if (ok) {
            QSqlQuery updateQuery(db); // 传入 db
            updateQuery.prepare("UPDATE users SET balance = balance - :price WHERE id = :uid");
            updateQuery.bindValue(":price", price);
            updateQuery.bindValue(":uid", m_user.id);
            if (!updateQuery.exec()) {
                ok = false;
                lastErr = updateQuery.lastError();
                qDebug() << "Update failed:" << lastErr.text();
            }
        }
        // ========== 事务收尾 ==========
        if (ok) {
            if (db.commit()) {
                // 更新内存余额+UI
                m_user.balance -= price;
                updateBalanceUI();
                QMessageBox::information(this, "支付成功", "预订成功！\n即将跳转到订单列表查看详情。");
                m_mainTab->setCurrentIndex(2); // 跳转到订单页
            } else {
                ok = false;
                lastErr = db.lastError();
                qDebug() << "Commit failed:" << lastErr.text();
            }
        }
        if (!ok) {
            db.rollback();
            QMessageBox::critical(this, "订票失败", "数据库错误：" + lastErr.text());
        }
        // 3. 【关键】用完必须移除这个临时连接
        DbManager::removeConn(db);
    });
}
// 搜索与列表
void MainWindow::setupSearchTab() {
    QWidget* tabContainer = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tabContainer);
    layout->setContentsMargins(20, 20, 20, 20);

    m_searchStack = new QStackedWidget;

    // 搜索表单页
    QWidget* formPage = new QWidget;
    QVBoxLayout* formLay = new QVBoxLayout(formPage);
    formLay->setAlignment(Qt::AlignTop);

    QWidget* searchCard = new QWidget;
    searchCard->setObjectName("SearchCard");
    searchCard->setStyleSheet("#SearchCard { background: white; border-radius: 8px; border: 1px solid #E4E7ED; }");
    QHBoxLayout* cardLayout = new QHBoxLayout(searchCard);

    m_leOrigin = new QLineEdit; m_leOrigin->setPlaceholderText("出发"); m_leOrigin->setReadOnly(true);
    QPushButton* btnSelFrom = new QPushButton("选择"); connect(btnSelFrom, &QPushButton::clicked, this, &MainWindow::onSelectOrigin);
    m_leDest = new QLineEdit; m_leDest->setPlaceholderText("到达"); m_leDest->setReadOnly(true);
    QPushButton* btnSelTo = new QPushButton("选择"); connect(btnSelTo, &QPushButton::clicked, this, &MainWindow::onSelectDest);
    QPushButton* btnSwap = new QPushButton("⇌"); connect(btnSwap, &QPushButton::clicked, this, &MainWindow::onSwapCity);
    m_dateEdit = new QDateEdit(QDate::currentDate()); m_dateEdit->setCalendarPopup(true);

    QPushButton* btnSearch = new QPushButton("立即查询");
    btnSearch->setProperty("class", "primary"); btnSearch->setFixedSize(120, 40);
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::onSearchFlights);

    cardLayout->addWidget(new QLabel("出发:")); cardLayout->addWidget(m_leOrigin); cardLayout->addWidget(btnSelFrom); cardLayout->addWidget(btnSwap);
    cardLayout->addWidget(new QLabel("到达:")); cardLayout->addWidget(m_leDest); cardLayout->addWidget(btnSelTo);
    cardLayout->addWidget(new QLabel("日期:")); cardLayout->addWidget(m_dateEdit); cardLayout->addWidget(btnSearch);
    formLay->addWidget(searchCard);

    // 结果列表页
    QWidget* resultPage = new QWidget;
    QVBoxLayout* resLay = new QVBoxLayout(resultPage);

    QPushButton* btnBack = new QPushButton("← 返回搜索");
    btnBack->setStyleSheet("border:none; text-align:left; color:#666; font-size:14px;");
    connect(btnBack, &QPushButton::clicked, [=](){ m_searchStack->setCurrentIndex(0); });

    m_dateBarContainer = new QWidget;
    m_dateBarContainer->setStyleSheet("background: #F4F8FF; border-radius: 6px;");
    m_dateBarContainer->setFixedHeight(70);
    m_dateBarLayout = new QHBoxLayout(m_dateBarContainer);

    m_flightListTable = new QTableWidget;
    m_flightListTable->setColumnCount(8);
    m_flightListTable->setHorizontalHeaderLabels({"ID", "航班号", "出发", "到达", "起降", "机型", "价格", "操作"});
    m_flightListTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_flightListTable->verticalHeader()->setVisible(false);
    m_flightListTable->setSelectionMode(QAbstractItemView::NoSelection);

    resLay->addWidget(btnBack);
    resLay->addWidget(m_dateBarContainer);
    resLay->addWidget(m_flightListTable);

    m_searchStack->addWidget(formPage);
    m_searchStack->addWidget(resultPage);
    layout->addWidget(m_searchStack);

    // 添加搜索页到主Tab（仅这里添加一次）
    m_mainTab->addTab(tabContainer, "✈ 航班预订");
}

void MainWindow::updateBalanceUI() {
    m_lblUserBalance->setText(QString("👤 %1  |  余额: ¥%2")
                                  .arg(m_user.realName)
                                  .arg(QString::number(m_user.balance, 'f', 2)));
}

void MainWindow::onSearchFlights() {
    QString from = m_leOrigin->text().trimmed();
    QString to = m_leDest->text().trimmed();
    if (from.endsWith("市")) from.chop(1);
    if (to.endsWith("市")) to.chop(1);

    if (from.isEmpty() || to.isEmpty()) {
        QMessageBox::warning(this, "提示", "请选择城市");
        return;
    }
    m_searchStack->setCurrentIndex(1);
    loadFlightList(from, to, m_dateEdit->date());
}

void MainWindow::loadFlightList(QString from, QString to, const QDate& date) {
    updateDateBar(from, to, date);

    QSqlDatabase db = DbManager::getConn();
    if(!db.open()) return;
    QSqlQuery q(db);
    q.exec("SET NAMES 'utf8mb4'");

    QString sql = "SELECT id, flight_code, dep_time, arr_time, model, price, origin, destination "
                  "FROM flights WHERE DATE(dep_time) = :dt";
    if(!from.isEmpty()) sql += " AND origin LIKE :f";
    if(!to.isEmpty()) sql += " AND destination LIKE :t";
    sql += " ORDER BY dep_time ASC";

    q.prepare(sql);
    q.bindValue(":dt", date.toString("yyyy-MM-dd"));
    if(!from.isEmpty()) q.bindValue(":f", "%" + from + "%");
    if(!to.isEmpty()) q.bindValue(":t", "%" + to + "%");
    q.exec();

    m_flightListTable->setRowCount(0);
    int row = 0;
    while(q.next()) {
        m_flightListTable->insertRow(row);

        QString fId = q.value("id").toString();
        QString fCode = q.value("flight_code").toString();
        double price = q.value("price").toDouble();
        QDateTime dep = q.value("dep_time").toDateTime();

        m_flightListTable->setItem(row, 0, new QTableWidgetItem(fId));
        m_flightListTable->setItem(row, 1, new QTableWidgetItem(fCode));
        m_flightListTable->setItem(row, 2, new QTableWidgetItem(q.value("origin").toString()));
        m_flightListTable->setItem(row, 3, new QTableWidgetItem(q.value("destination").toString()));
        m_flightListTable->setItem(row, 4, new QTableWidgetItem(dep.toString("HH:mm") + "-" + q.value("arr_time").toDateTime().toString("HH:mm")));
        m_flightListTable->setItem(row, 5, new QTableWidgetItem(q.value("model").toString()));

        QTableWidgetItem* pItem = new QTableWidgetItem(QString("¥%1").arg(price));
        pItem->setForeground(QColor("#FF4D4F")); pItem->setFont(QFont("Arial", 12, QFont::Bold));
        m_flightListTable->setItem(row, 6, pItem);

        QWidget* w = new QWidget;
        QHBoxLayout* hl = new QHBoxLayout(w); hl->setContentsMargins(5,5,5,5);
        QPushButton* btn = new QPushButton("选座购票");
        btn->setStyleSheet("background-color: #FFA500; color: white; border-radius: 4px; padding: 5px; font-weight: bold;");

        connect(btn, &QPushButton::clicked, [=](){
            onPreBookClicked(fId, fCode, price, dep);
        });

        hl->addWidget(btn);
        m_flightListTable->setCellWidget(row, 7, w);
        m_flightListTable->setRowHeight(row, 60);
        row++;
    }
    db.close();
}

void MainWindow::onPreBookClicked(const QString& flightId, const QString& flightCode, double price, const QDateTime& depTime) {
    m_mainTab->setCurrentIndex(1); // 跳转到选座页
    m_seatPage->loadFlightSeatInfo(flightId.toInt(), flightCode, price);
}

void MainWindow::updateDateBar(QString from, QString to, const QDate& centerDate) {
    // 清空旧日期按钮
    QLayoutItem *item;
    while ((item = m_dateBarLayout->takeAt(0)) != nullptr) { delete item->widget(); delete item; }

    QSqlDatabase db = DbManager::getConn();
    if(!db.open()) return;

    // 生成前后3天的日期按钮
    for (int i = -3; i <= 3; ++i) {
        QDate d = centerDate.addDays(i);
        double minPrice = 0;
        QSqlQuery q(db);
        q.exec("SET NAMES 'utf8mb4'");
        QString sql = QString("SELECT MIN(price) FROM flights WHERE origin LIKE '%%1%' AND destination LIKE '%%2%' AND DATE(dep_time) = '%3'")
                          .arg(from).arg(to).arg(d.toString("yyyy-MM-dd"));
        if(q.exec(sql) && q.next()) minPrice = q.value(0).toDouble();

        QToolButton* btn = new QToolButton;
        btn->setCheckable(true);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (i == 0) {
            btn->setChecked(true);
            btn->setStyleSheet("background: #0086F6; color: white; border: none; border-radius: 4px; font-weight: bold;");
        } else {
            btn->setStyleSheet("background: white; color: #333; border: 1px solid #ddd; border-radius: 4px;");
        }
        QString priceStr = (minPrice > 0) ? QString("¥%1").arg(minPrice) : "无";
        btn->setText(QString("%1\n%2").arg(d.toString("MM-dd")).arg(priceStr));

        connect(btn, &QToolButton::clicked, [=](){
            m_dateEdit->setDate(d);
            loadFlightList(from, to, d);
        });
        m_dateBarLayout->addWidget(btn);
    }
    db.close();
}

void MainWindow::setupStatusTab() {
    m_mainTab->addTab(new QWidget, "🕒 航班动态");
}

void MainWindow::onSelectOrigin() {
    CitySelector dlg(this);
    if(dlg.exec()) m_leOrigin->setText(dlg.getSelectedCity());
}

void MainWindow::onSelectDest() {
    CitySelector dlg(this);
    if(dlg.exec()) m_leDest->setText(dlg.getSelectedCity());
}

void MainWindow::onSwapCity() {
    QString t = m_leOrigin->text();
    m_leOrigin->setText(m_leDest->text());
    m_leDest->setText(t);
}
