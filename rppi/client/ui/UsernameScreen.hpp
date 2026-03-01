#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;

class UsernameScreen : public QWidget {
    Q_OBJECT
public:
    explicit UsernameScreen(QWidget* parent = nullptr);

    void setError(const QString& msg);
    void clearError();

signals:
    void submitted(const QString& username);

private:
    QLineEdit* input_;
    QLabel*    errorLabel_;
};
