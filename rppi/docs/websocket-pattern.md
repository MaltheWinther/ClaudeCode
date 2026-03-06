# WebSocket-moensteret med libwebsockets (lws)

## Hvorfor dette er vigtigt

ALLE 4 netvaerksklasser (LobbyServer, LobbyClient, GameRoom, GameClient)
bruger det SAMME moenser. Forstaar du et, forstaar du alle fire.

## Det grundlaeggende moenser

libwebsockets er callback-baseret. Du registrerer en callback-funktion,
og lws kalder den naar der sker noget (ny forbindelse, besked modtaget, klar til at skrive).

### 1. Opret context

```cpp
lws_protocols protocols[] = {
    { "protocol-name", MyClass::wsCallback, 0, 4096, 0, nullptr, 0 },
    { nullptr, nullptr, 0, 0, 0, nullptr, 0 }  // terminator
};

lws_context_creation_info info{};
info.port      = 9000;        // server: lyt-port, client: CONTEXT_PORT_NO_LISTEN
info.protocols = protocols;

context_ = lws_create_context(&info);
```

### 2. Event loop

```cpp
void MyClass::run() {
    running_ = true;
    while (running_)
        lws_service(context_, 50);  // blokkerer op til 50ms, processer events
}
```

`lws_service()` er hjertet: den checker for nye forbindelser, modtager data,
og kalder callbacks. Uden den sker der ingenting.

### 3. Static callback (det svaereste koncept)

lws kraever en **static** callback fordi det er et C-bibliotek (ikke C++).
Vi bruger et globalt pointer-trick:

```cpp
static MyClass* s_instance = nullptr;  // global

MyClass::MyClass() {
    s_instance = this;
    // ...
}

// Static -- ingen 'this' pointer
int MyClass::wsCallback(lws* wsi, lws_callback_reasons reason,
                         void* user, void* in, size_t len) {
    if (!s_instance) return 0;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:       // server: ny client forbundet
        case LWS_CALLBACK_CLIENT_ESTABLISHED: // client: forbundet til server
            s_instance->onConnect(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:            // server: modtaget besked
        case LWS_CALLBACK_CLIENT_RECEIVE:     // client: modtaget besked
            s_instance->onMessage(wsi, string(static_cast<char*>(in), len));
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:   // server: klar til at sende
        case LWS_CALLBACK_CLIENT_WRITEABLE:   // client: klar til at sende
            s_instance->onWritable(wsi);
            break;

        case LWS_CALLBACK_CLOSED:             // server: client disconnected
        case LWS_CALLBACK_CLIENT_CLOSED:      // client: disconnected
            s_instance->onDisconnect(wsi);
            break;
    }
    return 0;
}
```

**Vigtigt**: `wsi` (WebSocket Instance) er en pointer der identificerer
en specifik forbindelse. Serveren har mange wsi'er (en per client),
clienten har typisk en.

### 4. Sende beskeder (write queue)

Man kan IKKE skrive til en WebSocket naar som helst. Man skal:
1. Laegge beskeden i en koe
2. Fortaelle lws at vi vil skrive: `lws_callback_on_writable(wsi)`
3. Vente paa `LWS_CALLBACK_SERVER_WRITEABLE` callback
4. Skrive med `lws_write()` og LWS_PRE padding

```cpp
// Koere besked
void sendTo(lws* wsi, const json& msg) {
    writeQueues_[wsi].push(msg.dump());
    lws_callback_on_writable(wsi);  // "sig til lws at vi vil skrive"
}

// Callback naar lws er klar
void onWritable(lws* wsi) {
    auto& queue = writeQueues_[wsi];
    if (queue.empty()) return;

    const string& msg = queue.front();

    // LWS_PRE: lws kraever padding FOER data (til interne headers)
    vector<unsigned char> buf(LWS_PRE + msg.size());
    memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
    lws_write(wsi, buf.data() + LWS_PRE, msg.size(), LWS_WRITE_TEXT);

    queue.pop();
    if (!queue.empty())
        lws_callback_on_writable(wsi);  // der er mere at sende
}
```

**Hvorfor LWS_PRE?** lws bruger plads foer din data til WebSocket frame headers.
I stedet for at kopiere din data, kraever den at du allokerer ekstra plads foran.

### 5. sendTo vs queueTo

Systemet har to maader at sende pa:

- **sendTo()**: Rydder koeen foerst, behold kun nyeste besked.
  Bruges til streaming data (game_state ved 40Hz) -- kun seneste position matters.

- **queueTo()**: Tilfoej til koeen uden at rydde.
  Bruges til vigtige enkelt-beskeder (note_state, game_over) der IKKE maa droppes.

## Traad-sikkerhed med lws

- `lws_service()` er IKKE traadsikker -- kun en traad maa kalde den
- For at sende fra en anden traad: laeg i queue (mutex) + kald `lws_cancel_service()`
- `lws_cancel_service()` er den ENESTE traadsikre lws-funktion
- Den vaekker `lws_service()` sa den checker koeen

Eksempel (LobbyClient::requestStart fra Qt main thread):
```cpp
void LobbyClient::requestStart() {
    {
        lock_guard<mutex> lk(queueMutex_);
        outQueue_.push(MsgStartGame::build().dump());
        pendingWake_ = true;
    }
    lws_cancel_service(context_);  // vaek lws traaden
}
```

## De 4 netvaerksklasser

| Klasse | Rolle | Port | Traad |
|--------|-------|------|-------|
| LobbyServer | Server, modtager lobby-beskeder | 9000 | Main (eneste traad) |
| LobbyClient | Client, forbinder til lobby | 9000 | Network thread |
| GameRoom | Server, driver spillet | 900x | Main + game loop traad |
| GameClient | Client, forbinder til spil | 900x | Network thread |

Alle fire folger praecis det samme moenser ovenfor.
