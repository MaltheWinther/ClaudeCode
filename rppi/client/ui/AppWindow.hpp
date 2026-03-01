#pragma once

#include <QMainWindow>
#include <QStackedWidget>

class UsernameScreen;
class MainMenuScreen;
class LobbyScreen;
class RoleScreen;
class GameScreen;
class GameOverScreen;

class AppWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit AppWindow(QWidget* parent = nullptr);

    // Screen transitions (call from Qt main thread only)
    void showUsername();
    void showMainMenu(const QString& username);
    void showLobbyHost(const QString& code, int playerCount);
    void showLobbyGuest(const QString& code);
    void updateLobbyPlayerCount(int count);
    void setUsernameError(const QString& msg);
    void setMainMenuError(const QString& msg);
    void showRole(const QString& role, const QString& game);
    void showGame(const QString& role);
    void updateGameState(float ballX, float ballY, float pitch, float roll, int level);
    void showGameOver(bool win);

signals:
    void usernameSubmitted(const QString& username);
    void hostRoomClicked();
    void joinRoomClicked(const QString& code);
    void startGameClicked();
    void roleDisplayDone();
    void playAgainClicked();

private:
    QStackedWidget* stack_;
    UsernameScreen* usernameScreen_;
    MainMenuScreen* mainMenuScreen_;
    LobbyScreen*    lobbyScreen_;
    RoleScreen*     roleScreen_;
    GameScreen*     gameScreen_;
    GameOverScreen* gameOverScreen_;
};
