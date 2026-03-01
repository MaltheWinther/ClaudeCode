#pragma once

#include <QWidget>

class QLabel;

class RoleScreen : public QWidget {
    Q_OBJECT
public:
    explicit RoleScreen(QWidget* parent = nullptr);

    // Shows role for 2 seconds, then emits done()
    void setRole(const QString& role, const QString& game);

signals:
    void done();

private:
    QLabel* roleLabel_;
    QLabel* gameLabel_;
    QLabel* descLabel_;
};
