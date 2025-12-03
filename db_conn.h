#ifndef DB_CONN_H
#define DB_CONN_H

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QUuid> // 如果没有 uuid，可以用简单的计数器

class DbManager {
public:
    // 改为“每次返回新连接”，避免共享冲突
    // 参数 uniqueName 允许调用者指定是否需要唯一的连接名
    static QSqlDatabase getConn(bool unique = false) {
        QString connName;
        if (unique) {
            // 生成唯一连接名（防止重复）
            static int connCount = 0;
            connName = QString("skylink_odbc_conn_%1").arg(connCount++);
        } else {
            connName = QSqlDatabase::defaultConnection;
        }

        QSqlDatabase db;
        if (QSqlDatabase::contains(connName)) {
            db = QSqlDatabase::database(connName);
        } else {
            db = QSqlDatabase::addDatabase("QODBC", connName);
            db.setDatabaseName("skylink_db"); // 你的ODBC数据源名
            db.setUserName("root");           // MySQL用户名
            db.setPassword("1024miss");       // 密码
        }

        // 尝试打开连接
        if (!db.isOpen()) {
            if (!db.open()) {
                qCritical() << "连接失败 [" << connName << "]：" << db.lastError().text();
            }
        }

        return db;
    }

    // 新增：用完连接后主动移除（防止连接泄漏）
    // 专门用于那些 unique=true 的连接
    static void removeConn(QSqlDatabase& db) {
        QString connName = db.connectionName();
        db.close(); // 先关闭
        // 这是一个静态方法，移除数据库定义
        QSqlDatabase::removeDatabase(connName);
    }
};

#endif // DB_CONN_H
