#pragma once

#include <vector>
#include <string>
#include <QMainWindow>
#include <QStackedWidget>
#include "../../common/Messages.hpp"

class UsernameScreen;
class MainMenuScreen;
class LobbyScreen;
class GameSelectScreen;
class RoleScreen;
class GameScreen;
class NotePiScreen;
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
    void updateLobbyPlayers(int count, const QString& hostName, const QString& guestName);
    void setUsernameError(const QString& msg);
    void setMainMenuError(const QString& msg);
    void showGameSelect(bool isHost);
    void showRole(const QString& role, const QString& game);
    void showGame(const QString& role);
    void showNotePiGame(const QString& role);
    void addDetectedNote(const QString& note);
    void updateNoteState(const MsgNoteState& state);
    void updateGameState(float ballX, float ballY, float pitch, float roll, int level);
    void showGameOver(bool win, int elapsedSecs, int levelsReached,
                      const std::vector<LeaderboardEntry>& leaderboard);
    void showDisconnect(const QString& msg);

signals:
    void usernameSubmitted(const QString& username);
    void hostRoomClicked();
    void joinRoomClicked(const QString& code);
    void startGameClicked();
    void gameSelected(const QString& gameType);
    void roleDisplayDone();
    void notePiNotesSubmitted(const std::vector<std::string>& notes);
    void playAgainClicked();
    void exitGameClicked();

private:
    QStackedWidget*  stack_;
    UsernameScreen*  usernameScreen_;
    MainMenuScreen*  mainMenuScreen_;
    LobbyScreen*     lobbyScreen_;
    GameSelectScreen* gameSelectScreen_;
    RoleScreen*      roleScreen_;
    GameScreen*      gameScreen_;
    NotePiScreen*    notePiScreen_;
    GameOverScreen*  gameOverScreen_;
};
