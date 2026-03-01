CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread

# ── Include paths ─────────────────────────────────────────────────────────────
# common/          — Messages.hpp
# ..               — so client/ can reach ../../gyro-pi/spi_interface.hpp
INC := -Icommon -I.

# ── Libraries ─────────────────────────────────────────────────────────────────
LWS_LIB    := -lwebsockets
BMI_LIB    := -lBMI160Wrapper           # installed at /usr/lib by ecesyshat
THREAD_LIB := -lpthread

# ── Output directories ────────────────────────────────────────────────────────
BINDIR := bin
OBJDIR := obj
$(shell mkdir -p $(BINDIR) $(OBJDIR))

# ── Source lists ──────────────────────────────────────────────────────────────
SERVER_SRCS := \
    server/main.cpp \
    server/LobbyServer.cpp

ROOM_SRCS := \
    server/game_room/main.cpp \
    server/GameRoom.cpp \
    server/GyroPiGame.cpp

CLIENT_SRCS := \
    client/main.cpp \
    client/LobbyClient.cpp \
    client/GameClient.cpp \
    client/GyroReader.cpp \
    client/LocalBrowser.cpp

# ── Object files (in obj/ to avoid naming conflicts with bin/) ────────────────
SERVER_OBJS := $(SERVER_SRCS:%.cpp=$(OBJDIR)/%.o)
ROOM_OBJS   := $(ROOM_SRCS:%.cpp=$(OBJDIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.cpp=$(OBJDIR)/%.o)

ALL_OBJS := $(SERVER_OBJS) $(ROOM_OBJS) $(CLIENT_OBJS)

# ── Targets ───────────────────────────────────────────────────────────────────
.PHONY: all server game_room client clean

all: server game_room client

# Server — lobby process
server: $(BINDIR)/server

$(BINDIR)/server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LWS_LIB) $(THREAD_LIB)

# game_room — spawned by server via fork/exec; must live next to the server binary
game_room: $(BINDIR)/game_room

$(BINDIR)/game_room: $(ROOM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LWS_LIB) $(THREAD_LIB)

# Client — runs on both player RPis
client: $(BINDIR)/client

$(BINDIR)/client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LWS_LIB) $(BMI_LIB) $(THREAD_LIB)

# ── Generic compile rule ──────────────────────────────────────────────────────
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BINDIR) $(OBJDIR)
