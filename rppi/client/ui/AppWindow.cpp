#include "AppWindow.hpp"
#include "UsernameScreen.hpp"
#include "MainMenuScreen.hpp"
#include "LobbyScreen.hpp"
#include "RoleScreen.hpp"
#include "GameScreen.hpp"
#include "GameOverScreen.hpp"

AppWindow::AppWindow(QWidget* parent) : QMainWindow(parent) {
    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    usernameScreen_ = new UsernameScreen;
    mainMenuScreen_ = new MainMenuScreen;
    lobbyScreen_    = new LobbyScreen;
    roleScreen_     = new RoleScreen;
    gameScreen_     = new GameScreen;
    gameOverScreen_ = new GameOverScreen;

    stack_->addWidget(usernameScreen_);
    stack_->addWidget(mainMenuScreen_);
    stack_->addWidget(lobbyScreen_);
    stack_->addWidget(roleScreen_);
    stack_->addWidget(gameScreen_);
    stack_->addWidget(gameOverScreen_);

    // Forward child signals to AppWindow signals
    connect(usernameScreen_, &UsernameScreen::submitted,   this, &AppWindow::usernameSubmitted);
    connect(mainMenuScreen_, &MainMenuScreen::hostClicked, this, &AppWindow::hostRoomClicked);
    connect(mainMenuScreen_, &MainMenuScreen::joinClicked, this, &AppWindow::joinRoomClicked);
    connect(lobbyScreen_,    &LobbyScreen::startClicked,   this, &AppWindow::startGameClicked);
    connect(roleScreen_,     &RoleScreen::done,            this, &AppWindow::roleDisplayDone);
    connect(gameOverScreen_, &GameOverScreen::playAgain,   this, &AppWindow::playAgainClicked);
}

void AppWindow::showUsername() {
    usernameScreen_->clearError();
    stack_->setCurrentWidget(usernameScreen_);
}

void AppWindow::showMainMenu(const QString& username) {
    if (!username.isEmpty())
        mainMenuScreen_->setUsername(username);
    mainMenuScreen_->clearError();
    stack_->setCurrentWidget(mainMenuScreen_);
}

void AppWindow::showLobbyHost(const QString& code, int playerCount) {
    lobbyScreen_->setupHost(code, playerCount);
    stack_->setCurrentWidget(lobbyScreen_);
}

void AppWindow::showLobbyGuest(const QString& code) {
    lobbyScreen_->setupGuest(code);
    stack_->setCurrentWidget(lobbyScreen_);
}

void AppWindow::updateLobbyPlayerCount(int count) {
    lobbyScreen_->setPlayerCount(count);
}

void AppWindow::updateLobbyPlayers(int count, const QString& hostName,
                                   const QString& guestName) {
    lobbyScreen_->setPlayers(count, hostName, guestName);
}

void AppWindow::setUsernameError(const QString& msg) {
    usernameScreen_->setError(msg);
}

void AppWindow::setMainMenuError(const QString& msg) {
    mainMenuScreen_->setError(msg);
}

void AppWindow::showRole(const QString& role, const QString& game) {
    roleScreen_->setRole(role, game);
    stack_->setCurrentWidget(roleScreen_);
}

void AppWindow::showGame(const QString& role) {
    gameScreen_->setRole(role);
    stack_->setCurrentWidget(gameScreen_);
}

void AppWindow::updateGameState(float ballX, float ballY,
                                float pitch, float roll, int level) {
    gameScreen_->updateGameState(ballX, ballY, pitch, roll, level);
}

void AppWindow::showGameOver(bool win, int elapsedSecs, int levelsReached,
                             const std::vector<LeaderboardEntry>& leaderboard) {
    gameOverScreen_->setResult(win, elapsedSecs, levelsReached, leaderboard);
    stack_->setCurrentWidget(gameOverScreen_);
}

void AppWindow::showDisconnect(const QString& msg) {
    gameOverScreen_->setDisconnect(msg);
    stack_->setCurrentWidget(gameOverScreen_);
}
