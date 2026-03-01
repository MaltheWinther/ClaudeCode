#pragma once

#include <QWidget>

class QLabel;
class QTimer;

class RoleScreen : public QWidget {
    Q_OBJECT
public:
    explicit RoleScreen(QWidget* parent = nullptr);

    // Shows role then counts 3-2-1 (1s each), then emits done()
    void setRole(const QString& role, const QString& game);

signals:
    void done();

private:
    QLabel* roleLabel_;
    QLabel* gameLabel_;
    QLabel* descLabel_;
    QLabel* countdownLabel_;
    QTimer* countdownTimer_ = nullptr;
    int     countdown_      = 3;
};
