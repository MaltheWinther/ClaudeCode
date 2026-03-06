CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread -fpermissive

# ── Qt setup ──────────────────────────────────────────────────────────────────
# On Raspberry Pi:  sudo apt install qt6-base-dev
# On macOS:         brew install qt && export PKG_CONFIG_PATH=...
QT_CFLAGS := $(shell pkg-config --cflags Qt6Core Qt6Widgets Qt6Gui 2>/dev/null)
QT_LIBS   := $(shell pkg-config --libs   Qt6Core Qt6Widgets Qt6Gui 2>/dev/null)

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

LWS_LIB    := $(BREW_LIB) -lwebsockets
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
MOC_HEADERS := \
    client/ui/AppWindow.hpp \
    client/ui/UsernameScreen.hpp \
    client/ui/MainMenuScreen.hpp \
    client/ui/LobbyScreen.hpp \
    client/ui/GameSelectScreen.hpp \
    client/ui/RoleScreen.hpp \
    client/ui/GameScreen.hpp \
    client/ui/NotePiScreen.hpp \
    client/ui/GameOverScreen.hpp

MOC_SRCS := $(MOC_HEADERS:%.hpp=%.moc.cpp)

# Pattern rule: generate moc source from header
client/ui/%.moc.cpp: client/ui/%.hpp
	$(MOC) $(QT_CFLAGS) $< -o $@

# ── Source lists ──────────────────────────────────────────────────────────────
SERVER_SRCS := \
    server/main.cpp \
    server/LobbyServer.cpp

ROOM_SRCS := \
    server/game_room/main.cpp \
    server/GameRoom.cpp \
    server/GyroPiGame.cpp \
    server/NotePiGame.cpp \
    server/Leaderboard.cpp

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
    $(MOC_SRCS)

# ── Object files (in obj/ to avoid naming conflicts) ──────────────────────────
SERVER_OBJS := $(SERVER_SRCS:%.cpp=$(OBJDIR)/%.o)
ROOM_OBJS   := $(ROOM_SRCS:%.cpp=$(OBJDIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.cpp=$(OBJDIR)/%.o)

# ── Targets ───────────────────────────────────────────────────────────────────
.PHONY: all server game_room client clean

all: server game_room client

server: $(BINDIR)/server

$(BINDIR)/server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LWS_LIB) $(THREAD_LIB)

game_room: $(BINDIR)/game_room

$(BINDIR)/game_room: $(ROOM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LWS_LIB) $(THREAD_LIB)

client: $(BINDIR)/client

$(BINDIR)/client: $(CLIENT_OBJS)
	$(CXX) $(CLIENT_CXXFLAGS) -o $@ $^ \
	    $(LWS_LIB) $(BMI_LIB) $(AUBIO_LIB) $(THREAD_LIB) $(QT_LIBS)

# ── Compile rules ─────────────────────────────────────────────────────────────
# Client rule (Qt flags) — more specific, matched before the generic rule below
$(OBJDIR)/client/%.o: client/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CLIENT_CXXFLAGS) $(CLIENT_INC) -c $< -o $@

# Generic rule (server + game_room — no Qt needed)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BINDIR) $(OBJDIR) $(MOC_SRCS)
