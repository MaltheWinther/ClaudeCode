#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class MainMenuScreen : public QWidget {
    Q_OBJECT
public:
    explicit MainMenuScreen(QWidget* parent = nullptr);

    void setUsername(const QString& username);
    void setError(const QString& msg);
    void clearError();

signals:
    void hostClicked();
    void joinClicked(const QString& code);

private:
    QLabel*      usernameLabel_;
    QLabel*      errorLabel_;
    QWidget*     joinPanel_;
    QLineEdit*   codeInput_;
    QPushButton* joinConfirmBtn_;
};
