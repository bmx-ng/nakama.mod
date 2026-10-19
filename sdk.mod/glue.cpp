#include <chrono>
#include <memory>
#include <string>

#include <nakama-cpp/Nakama.h>

namespace {

struct NakamaClientHandle {
    Nakama::NClientPtr client;
    Nakama::NSessionPtr session;
    std::unique_ptr<Nakama::NRtDefaultClientListener> realtimeListener;
    Nakama::NRtClientPtr realtimeClient;
    int authenticationState = 0;
    int socketState = 0;
    int matchState = 0;
    int errorCode = 0;
    std::string errorMessage;
    std::string matchmakerTicket;
    std::string matchId;
    std::int64_t receivedOpCode = 0;
    std::string receivedData;
    std::string receivedSenderId;
    int receivedCount = 0;

    ~NakamaClientHandle() {
        if (realtimeClient) {
            realtimeClient->setListener(nullptr);
            realtimeClient->disconnect();
            realtimeClient.reset();
        }
    }
};

enum AuthenticationState {
    AuthenticationIdle = 0,
    AuthenticationPending = 1,
    AuthenticationSucceeded = 2,
    AuthenticationFailed = 3
};

enum SocketState {
    SocketIdle = 0,
    SocketConnecting = 1,
    SocketConnected = 2,
    SocketDisconnected = 3,
    SocketFailed = 4
};

enum MatchState {
    MatchIdle = 0,
    MatchSearching = 1,
    MatchJoining = 2,
    MatchJoined = 3,
    MatchFailed = 4
};

void setRealtimeError(NakamaClientHandle* handle, const Nakama::NRtError& error) {
    handle->errorCode = static_cast<int>(error.code);
    handle->errorMessage = error.message;
}

}
extern "C" {

const char* bmx_nakama_sdk_version() {
    return Nakama::getNakamaSdkVersion();
}

void* bmx_nakama_client_create(
    const char* host,
    int port,
    const char* serverKey,
    int useSSL,
    int timeoutMs) {
    try {
        Nakama::NClientParameters parameters;
        parameters.host = host ? host : "127.0.0.1";
        parameters.port = port;
        parameters.serverKey = serverKey ? serverKey : "defaultkey";
        parameters.ssl = useSSL != 0;
        parameters.timeout = std::chrono::milliseconds(timeoutMs);

        auto handle = std::make_unique<NakamaClientHandle>();
        handle->client = Nakama::createDefaultClient(parameters);
        if (!handle->client) {
            return nullptr;
        }
        return handle.release();
    } catch (...) {
        return nullptr;
    }
}

void bmx_nakama_client_destroy(void* value) {
    delete static_cast<NakamaClientHandle*>(value);
}

void bmx_nakama_client_tick(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (handle && handle->client) {
        handle->client->tick();
        if (handle->realtimeClient) {
            handle->realtimeClient->tick();
        }
    }
}

int bmx_nakama_client_authenticate_device(
    void* value,
    const char* deviceId,
    const char* username,
    int create) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->client || !deviceId || !deviceId[0] ||
        handle->authenticationState == AuthenticationPending) {
        return 0;
    }

    handle->session.reset();
    handle->authenticationState = AuthenticationPending;
    handle->errorCode = 0;
    handle->errorMessage.clear();

    try {
        std::optional<std::string> requestedUsername;
        if (username && username[0]) {
            requestedUsername = std::string(username);
        }

        handle->client->authenticateDevice(
            deviceId,
            requestedUsername,
            create != 0,
            {},
            [handle](Nakama::NSessionPtr session) {
                handle->session = std::move(session);
                handle->authenticationState = AuthenticationSucceeded;
            },
            [handle](const Nakama::NError& error) {
                handle->errorCode = static_cast<int>(error.code);
                handle->errorMessage = error.message;
                handle->authenticationState = AuthenticationFailed;
            });
        return 1;
    } catch (const std::exception& error) {
        handle->errorCode = static_cast<int>(Nakama::ErrorCode::InternalError);
        handle->errorMessage = error.what();
        handle->authenticationState = AuthenticationFailed;
        return 0;
    } catch (...) {
        handle->errorCode = static_cast<int>(Nakama::ErrorCode::InternalError);
        handle->errorMessage = "Unknown exception while starting device authentication.";
        handle->authenticationState = AuthenticationFailed;
        return 0;
    }
}

int bmx_nakama_client_authentication_state(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->authenticationState : AuthenticationFailed;
}

int bmx_nakama_client_error_code(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->errorCode : static_cast<int>(Nakama::ErrorCode::InternalError);
}

const char* bmx_nakama_client_error_message(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->errorMessage.c_str() : "Invalid Nakama client.";
}

const char* bmx_nakama_client_user_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle && handle->session ? handle->session->getUserId().c_str() : "";
}

const char* bmx_nakama_client_username(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle && handle->session ? handle->session->getUsername().c_str() : "";
}

int bmx_nakama_client_session_created(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle && handle->session && handle->session->isCreated();
}

int bmx_nakama_client_socket_connect(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->client || !handle->session ||
        handle->socketState == SocketConnecting || handle->socketState == SocketConnected) {
        return 0;
    }

    try {
        handle->realtimeListener = std::make_unique<Nakama::NRtDefaultClientListener>();
        handle->realtimeClient = handle->client->createRtClient();
        if (!handle->realtimeClient) {
            handle->socketState = SocketFailed;
            handle->errorMessage = "Could not create Nakama realtime client.";
            return 0;
        }

        handle->realtimeListener->setConnectCallback([handle]() {
            handle->socketState = SocketConnected;
        });
        handle->realtimeListener->setDisconnectCallback([handle](const Nakama::NRtClientDisconnectInfo& info) {
            handle->socketState = SocketDisconnected;
            handle->errorCode = info.code;
            handle->errorMessage = info.reason;
        });
        handle->realtimeListener->setErrorCallback([handle](const Nakama::NRtError& error) {
            setRealtimeError(handle, error);
            if (handle->socketState != SocketConnected) {
                handle->socketState = SocketFailed;
            }
            if (handle->matchState != MatchIdle && handle->matchState != MatchJoined) {
                handle->matchState = MatchFailed;
            }
        });
        handle->realtimeListener->setMatchmakerMatchedCallback([handle](Nakama::NMatchmakerMatchedPtr matched) {
            if (!matched || !handle->realtimeClient) {
                handle->matchState = MatchFailed;
                handle->errorMessage = "Nakama returned an empty matchmaker result.";
                return;
            }

            handle->matchState = MatchJoining;
            auto joined = [handle](const Nakama::NMatch& match) {
                handle->matchId = match.matchId;
                handle->matchState = MatchJoined;
            };
            auto failed = [handle](const Nakama::NRtError& error) {
                setRealtimeError(handle, error);
                handle->matchState = MatchFailed;
            };

            if (!matched->token.empty()) {
                handle->realtimeClient->joinMatchByToken(matched->token, joined, failed);
            } else {
                handle->realtimeClient->joinMatch(matched->matchId, {}, joined, failed);
            }
        });
        handle->realtimeListener->setMatchDataCallback([handle](const Nakama::NMatchData& data) {
            handle->receivedOpCode = data.opCode;
            handle->receivedData = data.data;
            handle->receivedSenderId = data.presence.userId;
            ++handle->receivedCount;
        });

        handle->realtimeClient->setListener(handle->realtimeListener.get());
        handle->socketState = SocketConnecting;
        handle->errorCode = 0;
        handle->errorMessage.clear();
        handle->realtimeClient->connect(handle->session, true);
        return 1;
    } catch (const std::exception& error) {
        handle->socketState = SocketFailed;
        handle->errorMessage = error.what();
        return 0;
    } catch (...) {
        handle->socketState = SocketFailed;
        handle->errorMessage = "Unknown exception while connecting realtime socket.";
        return 0;
    }
}

int bmx_nakama_client_socket_state(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->socketState : SocketFailed;
}

int bmx_nakama_client_add_matchmaker(void* value, int minCount, int maxCount) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->realtimeClient || handle->socketState != SocketConnected ||
        minCount < 2 || maxCount < minCount || handle->matchState == MatchSearching ||
        handle->matchState == MatchJoining) {
        return 0;
    }

    handle->matchState = MatchSearching;
    handle->matchmakerTicket.clear();
    handle->matchId.clear();
    handle->errorCode = 0;
    handle->errorMessage.clear();
    handle->realtimeClient->addMatchmaker(
        minCount,
        maxCount,
        std::nullopt,
        {},
        {},
        std::nullopt,
        [handle](const Nakama::NMatchmakerTicket& ticket) {
            handle->matchmakerTicket = ticket.ticket;
        },
        [handle](const Nakama::NRtError& error) {
            setRealtimeError(handle, error);
            handle->matchState = MatchFailed;
        });
    return 1;
}

int bmx_nakama_client_match_state(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->matchState : MatchFailed;
}

const char* bmx_nakama_client_match_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->matchId.c_str() : "";
}

const char* bmx_nakama_client_matchmaker_ticket(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->matchmakerTicket.c_str() : "";
}

int bmx_nakama_client_send_match_data(void* value, long long opCode, const char* data) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->realtimeClient || handle->matchState != MatchJoined || !data) {
        return 0;
    }
    handle->realtimeClient->sendMatchData(handle->matchId, opCode, data);
    return 1;
}

int bmx_nakama_client_received_count(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->receivedCount : 0;
}

long long bmx_nakama_client_received_opcode(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->receivedOpCode : 0;
}

const char* bmx_nakama_client_received_data(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->receivedData.c_str() : "";
}

const char* bmx_nakama_client_received_sender_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->receivedSenderId.c_str() : "";
}

}
