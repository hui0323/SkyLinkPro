#include "login_dialog.h"
#include "db_conn.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::FramelessWindowHint); // 无边框更高级
    setAttribute(Qt::WA_TranslucentBackground); // 透明背景
    resize(420, 550);
    setupUi();
}

void LoginDialog::setupUi() {
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(10, 10, 10, 10); // 留出阴影空间

    // 卡片背景
    QWidget* bg = new QWidget;
    bg->setObjectName("Card");
    bg->setStyleSheet("#Card { background: white; border-radius: 12px; }");

    // 阴影
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20); shadow->setColor(QColor(0,0,0,50));
    bg->setGraphicsEffect(shadow);

    QVBoxLayout* bgLay = new QVBoxLayout(bg);
    bgLay->setContentsMargins(40, 50, 40, 40);
    bgLay->setSpacing(20);

    // 标题区
    QLabel* logo = new QLabel("✈ SkyLink Pro");
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size: 28px; font-weight: bold; color: #0086F6;");

    // 关闭按钮
    QPushButton* btnClose = new QPushButton("×", bg);
    btnClose->setGeometry(370, 10, 30, 30);
    btnClose->setStyleSheet("border:none; font-size: 24px; color: #999;");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    m_stack = new QStackedWidget;
    m_stack->addWidget(createLoginPage());
    m_stack->addWidget(createRegPage());

    bgLay->addWidget(logo);
    bgLay->addWidget(m_stack);

    mainLay->addWidget(bg);
}

QWidget* LoginDialog::createLoginPage() {
    QWidget* p = new QWidget;
    QVBoxLayout* l = new QVBoxLayout(p);
    l->setSpacing(15);

    m_loginUser = new QLineEdit; m_loginUser->setPlaceholderText("用户名");
    m_loginPass = new QLineEdit; m_loginPass->setPlaceholderText("密码");
    m_loginPass->setEchoMode(QLineEdit::Password);

    QPushButton* btnLogin = new QPushButton("登 录");
    btnLogin->setProperty("class", "primary");
    btnLogin->setFixedHeight(45);
    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::doLogin);

    QPushButton* btnToReg = new QPushButton("没有账号？去注册");
    btnToReg->setProperty("class", "textBtn");
    connect(btnToReg, &QPushButton::clicked, [=](){ m_stack->setCurrentIndex(1); });

    l->addWidget(new QLabel("欢迎回来，请登录您的账户"));
    l->addWidget(m_loginUser);
    l->addWidget(m_loginPass);
    l->addSpacing(10);
    l->addWidget(btnLogin);
    l->addWidget(btnToReg);
    l->addStretch();
    return p;
}

QWidget* LoginDialog::createRegPage() {
    QWidget* p = new QWidget;
    QVBoxLayout* l = new QVBoxLayout(p);
    l->setSpacing(15);

    m_regUser = new QLineEdit; m_regUser->setPlaceholderText("设置用户名");
    m_regPass = new QLineEdit; m_regPass->setPlaceholderText("设置密码");
    m_regPass->setEchoMode(QLineEdit::Password);
    m_regName = new QLineEdit; m_regName->setPlaceholderText("真实姓名");

    QPushButton* btnReg = new QPushButton("立即注册");
    btnReg->setProperty("class", "primary");
    btnReg->setFixedHeight(45);
    connect(btnReg, &QPushButton::clicked, this, &LoginDialog::doRegister);

    QPushButton* btnBack = new QPushButton("返回登录");
    btnBack->setProperty("class", "textBtn");
    connect(btnBack, &QPushButton::clicked, [=](){ m_stack->setCurrentIndex(0); });

    l->addWidget(new QLabel("创建新账户"));
    l->addWidget(m_regUser);
    l->addWidget(m_regPass);
    l->addWidget(m_regName);
    l->addSpacing(10);
    l->addWidget(btnReg);
    l->addWidget(btnBack);
    l->addStretch();
    return p;
}

void LoginDialog::doLogin() {
    QString u = m_loginUser->text().trimmed();
    QString p = m_loginPass->text();

    if(u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名和密码");
        return;
    }

    QSqlDatabase db = DbManager::getConn();
    if(db.open()) {
        QSqlQuery q(db);
        q.prepare("SELECT id, username, real_name, balance FROM users WHERE username=:u AND password=:p");
        q.bindValue(":u", u);
        q.bindValue(":p", p);

        if(q.exec() && q.next()) {
            m_userInfo.id = q.value(0).toInt();
            m_userInfo.username = q.value(1).toString();
            m_userInfo.realName = q.value(2).toString();
            m_userInfo.balance = q.value(3).toDouble();
            accept(); // 关闭窗口，返回 QDialog::Accepted
        } else {
            QMessageBox::warning(this, "失败", "用户名或密码错误");
        }
        db.close();
    } else {
        QMessageBox::critical(this, "错误", "数据库连接失败: " + db.lastError().text());
    }
}

void LoginDialog::doRegister() {
    QString u = m_regUser->text().trimmed();
    QString p = m_regPass->text();
    QString n = m_regName->text().trimmed();

    if(u.isEmpty() || p.isEmpty()) return;

    QSqlDatabase db = DbManager::getConn();
    if(db.open()) {
        QSqlQuery q(db);
        q.prepare("INSERT INTO users (username, password, real_name) VALUES (:u, :p, :n)");
        q.bindValue(":u", u);
        q.bindValue(":p", p);
        q.bindValue(":n", n);

        if(q.exec()) {
            QMessageBox::information(this, "成功", "注册成功，请登录");
            m_stack->setCurrentIndex(0);
        } else {
            QMessageBox::warning(this, "失败", "注册失败，用户名可能已存在");
        }
        db.close();
    }
}
