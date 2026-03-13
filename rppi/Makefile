CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread -fpermissive

# ── Qt setup ──────────────────────────────────────────────────────────────────
# On Raspberry Pi:  sudo apt install qt6-base-dev
# On macOS:         brew install qt && export PKG_CONFIG_PATH=...
QT_CFLAGS := $(shell pkg-config --cflags Qt6Core Qt6Widgets Qt6Gui Qt6Network 2>/dev/null)
QT_LIBS   := $(shell pkg-config --libs   Qt6Core Qt6Widgets Qt6Gui Qt6Network 2>/dev/null)

# Server only needs Core + Network (no Widgets/Gui)
SERVER_QT_CFLAGS := $(shell pkg-config --cflags Qt6Core Qt6Network 2>/dev/null)
SERVER_QT_LIBS   := $(shell pkg-config --libs   Qt6Core Qt6Network 2>/dev/null)

# moc binary: override with  make MOC=/path/to/moc  if needed
# macOS homebrew path: /opt/homebrew/share/qt/libexec/moc
# RPi (apt):           moc
MOC := $(shell which /opt/homebrew/share/qt/libexec/moc 2>/dev/null \
              || which /usr/lib/qt6/libexec/moc 2>/dev/null \
              || which moc6 2>/dev/null \
              || which moc  2>/dev/null \
              || echo moc)

# ── Libraries ─────────────────────────────────────────────────────────────────
# Homebrew lib/include paths (macOS only — no-op on Linux)
BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
BREW_INC    := $(if $(BREW_PREFIX),-I$(BREW_PREFIX)/include)
BREW_LIB    := $(if $(BREW_PREFIX),-L$(BREW_PREFIX)/lib)

# ── Include paths ─────────────────────────────────────────────────────────────
INC        := -Icommon -I. $(BREW_INC)
CLIENT_INC := $(INC) -Iclient

# ── Compile flags ─────────────────────────────────────────────────────────────
CLIENT_CXXFLAGS := $(CXXFLAGS) $(QT_CFLAGS)
SERVER_CXXFLAGS := $(CXXFLAGS) $(SERVER_QT_CFLAGS)

# BMI160Wrapper is only available on RPi (ecesyshat install); skip on macOS
UNAME := $(shell uname)
BMI_LIB    := $(if $(filter Linux,$(UNAME)),-lBMI160Wrapper)
ifeq ($(UNAME),Linux)
  AUBIO_LIB := -laubio -lasound
else ifeq ($(UNAME),Darwin)
  AUBIO_LIB := $(shell pkg-config --libs aubio 2>/dev/null || echo -laubio) -framework AudioToolbox
else
  AUBIO_LIB :=
endif
THREAD_LIB := -lpthread

# ── Output directories ────────────────────────────────────────────────────────
BINDIR := bin
OBJDIR := obj
$(shell mkdir -p $(BINDIR) $(OBJDIR))

# ── Q_OBJECT headers that need moc processing ─────────────────────────────────
# Client UI moc headers
MOC_CLIENT_UI_HEADERS := \
    client/ui/AppWindow.hpp \
    client/ui/UsernameScreen.hpp \
    client/ui/MainMenuScreen.hpp \
    client/ui/LobbyScreen.hpp \
    client/ui/GameSelectScreen.hpp \
    client/ui/RoleScreen.hpp \
    client/ui/GameScreen.hpp \
    client/ui/NotePiScreen.hpp \
    client/ui/GameOverScreen.hpp

# Client network moc headers (new QObject classes)
MOC_CLIENT_NET_HEADERS := \
    client/LobbyClient.hpp \
    client/GameClient.hpp

# Server moc headers (new QObject classes)
MOC_SERVER_HEADERS := \
    server/LobbyServer.hpp \
    server/GameRoom.hpp

MOC_CLIENT_UI_SRCS  := $(MOC_CLIENT_UI_HEADERS:%.hpp=%.moc.cpp)
MOC_CLIENT_NET_SRCS := $(MOC_CLIENT_NET_HEADERS:%.hpp=%.moc.cpp)

MOC_LOBBY_SERVER_SRC := server/LobbyServer.moc.cpp
MOC_GAME_ROOM_SRC   := server/GameRoom.moc.cpp

# Pattern rules: generate moc source from header
client/ui/%.moc.cpp: client/ui/%.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@

client/%.moc.cpp: client/%.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@

server/%.moc.cpp: server/%.hpp
	$(MOC) $(SERVER_QT_CFLAGS) $< -o $@

# ── Source lists ──────────────────────────────────────────────────────────────
SERVER_SRCS := \
    server/main.cpp \
    server/LobbyServer.cpp \
    $(MOC_LOBBY_SERVER_SRC)

ROOM_SRCS := \
    server/game_room/main.cpp \
    server/GameRoom.cpp \
    server/GyroPiGame.cpp \
    server/NotePiGame.cpp \
    server/Leaderboard.cpp \
    $(MOC_GAME_ROOM_SRC)

CLIENT_SRCS := \
    client/main.cpp \
    client/LobbyClient.cpp \
    client/GameClient.cpp \
    client/GyroReader.cpp \
    client/NoteReader.cpp \
    client/Credentials.cpp \
    client/ui/AppWindow.cpp \
    client/ui/UsernameScreen.cpp \
    client/ui/MainMenuScreen.cpp \
    client/ui/LobbyScreen.cpp \
    client/ui/GameSelectScreen.cpp \
    client/ui/RoleScreen.cpp \
    client/ui/GameScreen.cpp \
    client/ui/NotePiScreen.cpp \
    client/ui/GameOverScreen.cpp \
    $(MOC_CLIENT_UI_SRCS) \
    $(MOC_CLIENT_NET_SRCS)

# ── Object files (in obj/ to avoid naming conflicts) ──────────────────────────
SERVER_OBJS := $(SERVER_SRCS:%.cpp=$(OBJDIR)/%.o)
ROOM_OBJS   := $(ROOM_SRCS:%.cpp=$(OBJDIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.cpp=$(OBJDIR)/%.o)

# ── Targets ───────────────────────────────────────────────────────────────────
.PHONY: all server game_room client clean

all: server game_room client

server: $(BINDIR)/server

$(BINDIR)/server: $(SERVER_OBJS)
	$(CXX) $(SERVER_CXXFLAGS) -o $@ $^ $(SERVER_QT_LIBS) $(THREAD_LIB)

game_room: $(BINDIR)/game_room

$(BINDIR)/game_room: $(ROOM_OBJS)
	$(CXX) $(SERVER_CXXFLAGS) -o $@ $^ $(SERVER_QT_LIBS) $(THREAD_LIB)

client: $(BINDIR)/client

$(BINDIR)/client: $(CLIENT_OBJS)
	$(CXX) $(CLIENT_CXXFLAGS) -o $@ $^ \
	    $(BREW_LIB) $(BMI_LIB) $(AUBIO_LIB) $(THREAD_LIB) $(QT_LIBS)

# ── Compile rules ─────────────────────────────────────────────────────────────
# Client rule (Qt flags) — more specific, matched before the generic rule below
$(OBJDIR)/client/%.o: client/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CLIENT_CXXFLAGS) $(CLIENT_INC) -c $< -o $@

# Server rule (Qt Core + Network flags)
$(OBJDIR)/server/%.o: server/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(SERVER_CXXFLAGS) $(INC) -c $< -o $@

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BINDIR) $(OBJDIR) $(MOC_CLIENT_UI_SRCS) $(MOC_CLIENT_NET_SRCS) $(MOC_LOBBY_SERVER_SRC) $(MOC_GAME_ROOM_SRC)
