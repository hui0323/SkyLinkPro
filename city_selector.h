#ifndef CITY_SELECTOR_H
#define CITY_SELECTOR_H

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMap>

class CitySelector : public QDialog {
    Q_OBJECT

public:
    explicit CitySelector(QWidget *parent = nullptr);
    QString getSelectedCity() const { return m_selectedCity; }

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget* m_tree;
    QString m_selectedCity;
    QMap<QString, QStringList> m_cityData;

    void initData(); // 加载那几百个城市
    void setupUi();
    void loadTree();
};

#endif // CITY_SELECTOR_H

