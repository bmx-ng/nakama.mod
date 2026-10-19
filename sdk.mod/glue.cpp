#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <nakama-cpp/Nakama.h>

namespace {

enum EventKind {
    EventNone = 0,
    EventSocketConnected = 1,
    EventSocketDisconnected = 2,
    EventError = 3,
    EventMatchmakerTicket = 4,
    EventMatchmakerMatched = 5,
    EventMatchJoined = 6,
    EventMatchPresenceJoined = 7,
    EventMatchPresenceLeft = 8,
    EventMatchData = 9,
    EventMatchmakingCancelled = 10,
    EventMatchLeft = 11
};

struct NakamaEvent {
    int kind = EventNone;
    int code = 0;
    std::int64_t opCode = 0;
    std::string subject;
    std::string userId;
    std::string sessionId;
    std::string username;
    std::string data;
};

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
    std::string matchSelfSessionId;
    std::vector<Nakama::NMatchmakerUser> matchedUsers;
    bool matchmakerCancellationPending = false;
    std::deque<NakamaEvent> events;
    NakamaEvent currentEvent;
    std::uint64_t droppedEvents = 0;
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
    MatchFailed = 4,
    MatchCancelled = 5
};

constexpr std::size_t EventCapacity = 256;

void queueEvent(NakamaClientHandle* handle, NakamaEvent event) {
    if (handle->events.size() >= EventCapacity) {
        ++handle->droppedEvents;
        return;
    }
    handle->events.emplace_back(std::move(event));
}

NakamaEvent presenceEvent(int kind, const std::string& matchId, const Nakama::NUserPresence& presence) {
    NakamaEvent event;
    event.kind = kind;
    event.subject = matchId;
    event.userId = presence.userId;
    event.sessionId = presence.sessionId;
    event.username = presence.username;
    return event;
}

void setRealtimeError(NakamaClientHandle* handle, const Nakama::NRtError& error) {
    handle->errorCode = static_cast<int>(error.code);
    handle->errorMessage = error.message;
    NakamaEvent event;
    event.kind = EventError;
    event.code = static_cast<int>(error.code);
    event.subject = error.message;
    queueEvent(handle, std::move(event));
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
            NakamaEvent event;
            event.kind = EventSocketConnected;
            queueEvent(handle, std::move(event));
        });
        handle->realtimeListener->setDisconnectCallback([handle](const Nakama::NRtClientDisconnectInfo& info) {
            handle->socketState = SocketDisconnected;
            handle->errorCode = info.code;
            handle->errorMessage = info.reason;
            NakamaEvent event;
            event.kind = EventSocketDisconnected;
            event.code = info.code;
            event.subject = info.reason;
            queueEvent(handle, std::move(event));
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

            handle->matchedUsers = matched->users;
            handle->matchSelfSessionId = matched->self.presence.sessionId;
            bool hasSelf = false;
            for (const auto& user : handle->matchedUsers) {
                if (user.presence.sessionId == matched->self.presence.sessionId) {
                    hasSelf = true;
                    break;
                }
            }
            if (!hasSelf && !matched->self.presence.sessionId.empty()) {
                handle->matchedUsers.emplace_back(matched->self);
            }
            NakamaEvent matchedEvent;
            matchedEvent.kind = EventMatchmakerMatched;
            matchedEvent.subject = !matched->token.empty() ? matched->token : matched->matchId;
            queueEvent(handle, std::move(matchedEvent));

            if (handle->matchmakerCancellationPending) {
                handle->matchmakerCancellationPending = false;
                handle->matchState = MatchCancelled;
                handle->matchmakerTicket.clear();
                NakamaEvent cancelledEvent;
                cancelledEvent.kind = EventMatchmakingCancelled;
                queueEvent(handle, std::move(cancelledEvent));
                return;
            }

            handle->matchState = MatchJoining;
            auto joined = [handle](const Nakama::NMatch& match) {
                handle->matchId = match.matchId;
                handle->matchState = MatchJoined;
                NakamaEvent event;
                event.kind = EventMatchJoined;
                event.subject = match.matchId;
                queueEvent(handle, std::move(event));
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
            NakamaEvent event = presenceEvent(EventMatchData, data.matchId, data.presence);
            event.opCode = data.opCode;
            event.data = data.data;
            queueEvent(handle, std::move(event));
        });
        handle->realtimeListener->setMatchPresenceCallback([handle](const Nakama::NMatchPresenceEvent& presence) {
            for (const auto& joined : presence.joins) {
                queueEvent(handle, presenceEvent(EventMatchPresenceJoined, presence.matchId, joined));
            }
            for (const auto& left : presence.leaves) {
                queueEvent(handle, presenceEvent(EventMatchPresenceLeft, presence.matchId, left));
            }
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
    handle->matchSelfSessionId.clear();
    handle->matchedUsers.clear();
    handle->matchmakerCancellationPending = false;
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
            NakamaEvent event;
            event.kind = EventMatchmakerTicket;
            event.subject = ticket.ticket;
            queueEvent(handle, std::move(event));
            if (handle->matchmakerCancellationPending && handle->realtimeClient) {
                handle->realtimeClient->removeMatchmaker(
                    ticket.ticket,
                    [handle]() {
                        if (!handle->matchmakerCancellationPending) return;
                        handle->matchmakerCancellationPending = false;
                        handle->matchmakerTicket.clear();
                        handle->matchState = MatchCancelled;
                        NakamaEvent cancelledEvent;
                        cancelledEvent.kind = EventMatchmakingCancelled;
                        queueEvent(handle, std::move(cancelledEvent));
                    },
                    [handle](const Nakama::NRtError& error) {
                        if (!handle->matchmakerCancellationPending) return;
                        handle->matchmakerCancellationPending = false;
                        setRealtimeError(handle, error);
                        handle->matchState = MatchFailed;
                    });
            }
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

int bmx_nakama_client_cancel_matchmaker(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->realtimeClient || handle->matchState != MatchSearching ||
        handle->matchmakerCancellationPending) {
        return 0;
    }

    handle->matchmakerCancellationPending = true;
    if (handle->matchmakerTicket.empty()) {
        return 1;
    }

    const std::string ticket = handle->matchmakerTicket;
    handle->realtimeClient->removeMatchmaker(
        ticket,
        [handle]() {
            if (!handle->matchmakerCancellationPending) return;
            handle->matchmakerCancellationPending = false;
            handle->matchmakerTicket.clear();
            handle->matchState = MatchCancelled;
            NakamaEvent event;
            event.kind = EventMatchmakingCancelled;
            queueEvent(handle, std::move(event));
        },
        [handle](const Nakama::NRtError& error) {
            if (!handle->matchmakerCancellationPending) return;
            handle->matchmakerCancellationPending = false;
            setRealtimeError(handle, error);
            handle->matchState = MatchFailed;
        });
    return 1;
}

int bmx_nakama_client_match_player_count(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? static_cast<int>(handle->matchedUsers.size()) : 0;
}

const char* bmx_nakama_client_match_self_session_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->matchSelfSessionId.c_str() : "";
}

const char* bmx_nakama_client_match_player_user_id(void* value, int index) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || index < 0 || index >= static_cast<int>(handle->matchedUsers.size())) return "";
    return handle->matchedUsers[static_cast<std::size_t>(index)].presence.userId.c_str();
}

const char* bmx_nakama_client_match_player_session_id(void* value, int index) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || index < 0 || index >= static_cast<int>(handle->matchedUsers.size())) return "";
    return handle->matchedUsers[static_cast<std::size_t>(index)].presence.sessionId.c_str();
}

const char* bmx_nakama_client_match_player_username(void* value, int index) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || index < 0 || index >= static_cast<int>(handle->matchedUsers.size())) return "";
    return handle->matchedUsers[static_cast<std::size_t>(index)].presence.username.c_str();
}

int bmx_nakama_client_send_match_data_bytes(
    void* value,
    long long opCode,
    const void* data,
    std::size_t length,
    const char* targetSessionId) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->realtimeClient || handle->matchState != MatchJoined ||
        (length > 0 && !data)) {
        return 0;
    }

    Nakama::NBytes bytes;
    if (length > 0) {
        bytes.assign(static_cast<const char*>(data), length);
    }

    std::vector<Nakama::NUserPresence> targets;
    if (targetSessionId && targetSessionId[0]) {
        for (const auto& user : handle->matchedUsers) {
            if (user.presence.sessionId == targetSessionId) {
                targets.emplace_back(user.presence);
                break;
            }
        }
        if (targets.empty()) return 0;
    }

    handle->realtimeClient->sendMatchData(handle->matchId, opCode, bytes, targets);
    return 1;
}

int bmx_nakama_client_leave_match(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || !handle->realtimeClient || handle->matchId.empty()) return 0;
    const std::string matchId = handle->matchId;
    handle->realtimeClient->leaveMatch(
        matchId,
        [handle, matchId]() {
            handle->matchId.clear();
            handle->matchSelfSessionId.clear();
            handle->matchedUsers.clear();
            handle->matchState = MatchIdle;
            NakamaEvent event;
            event.kind = EventMatchLeft;
            event.subject = matchId;
            queueEvent(handle, std::move(event));
        },
        [handle](const Nakama::NRtError& error) {
            setRealtimeError(handle, error);
        });
    return 1;
}

void bmx_nakama_client_socket_disconnect(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (handle && handle->realtimeClient) {
        handle->realtimeClient->disconnect();
    }
}

int bmx_nakama_client_poll_event(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    if (!handle || handle->events.empty()) return 0;
    handle->currentEvent = std::move(handle->events.front());
    handle->events.pop_front();
    return 1;
}

int bmx_nakama_client_event_kind(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.kind : EventNone;
}

int bmx_nakama_client_event_code(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.code : 0;
}

long long bmx_nakama_client_event_opcode(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.opCode : 0;
}

const char* bmx_nakama_client_event_subject(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.subject.c_str() : "";
}

const char* bmx_nakama_client_event_user_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.userId.c_str() : "";
}

const char* bmx_nakama_client_event_session_id(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.sessionId.c_str() : "";
}

const char* bmx_nakama_client_event_username(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.username.c_str() : "";
}

const void* bmx_nakama_client_event_data(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle && !handle->currentEvent.data.empty() ? handle->currentEvent.data.data() : nullptr;
}

std::size_t bmx_nakama_client_event_data_length(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->currentEvent.data.size() : 0;
}

unsigned long long bmx_nakama_client_dropped_events(void* value) {
    auto* handle = static_cast<NakamaClientHandle*>(value);
    return handle ? handle->droppedEvents : 0;
}

}
