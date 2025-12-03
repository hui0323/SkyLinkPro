#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QStackedWidget>

struct UserInfo {
    int id;
    QString username;
    QString realName;
    double balance;
};

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    UserInfo getLoggedUser() const { return m_userInfo; }

private slots:
    void doLogin();
    void doRegister();

private:
    QStackedWidget* m_stack;
    UserInfo m_userInfo;

    // 登录控件
    QLineEdit *m_loginUser, *m_loginPass;
    // 注册控件
    QLineEdit *m_regUser, *m_regPass, *m_regName;

    void setupUi();
    QWidget* createLoginPage();
    QWidget* createRegPage();
};

#endif // LOGIN_DIALOG_H
