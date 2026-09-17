#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct _ENetHost;
struct _ENetPeer;

// Thin wrapper over ENet: owns one ENet host with exactly one remote peer
// (this project is strictly one host + one client). Knows nothing about
// the game or the wire format — Network/Protocol.h defines message
// shapes, GameWindow decides what to send and when.
class NetworkSession
{
public:
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using PacketCallback = std::function<void(const std::vector<uint8_t>&)>;

    ~NetworkSession();

    NetworkSession(const NetworkSession&) = delete;
    NetworkSession& operator=(const NetworkSession&) = delete;

    // Starts listening on `port` for exactly one incoming connection.
    // Returns nullptr on failure (e.g. the port is already in use).
    static std::unique_ptr<NetworkSession> createHost(uint16_t port);

    // Begins connecting to a host at hostAddress:port. Connection is
    // asynchronous over ENet — poll() this session each frame and watch
    // isConnected() (or setOnConnected). Returns nullptr only if the
    // address string itself can't be resolved locally.
    static std::unique_ptr<NetworkSession> createClient(const std::string& hostAddress, uint16_t port);

    // Pumps ENet's event queue; call once per frame. Invokes whichever
    // callback below fits each event received since the last call.
    void poll();

    // Pushes any packets queued by send*() out onto the socket now,
    // instead of waiting for the next poll()'s enet_host_service call to
    // do it implicitly. Call once per frame after sending.
    void flush();

    // channel 0: reliable and ordered.
    void sendReliable(const std::vector<uint8_t>& bytes);
    // channel 1: unreliable but sequenced — a late/stale packet is
    // dropped rather than applied out of order. For per-tick state where
    // only the most recent value matters.
    void sendUnreliable(const std::vector<uint8_t>& bytes);

    bool isConnected() const { return m_connected; }

    void setOnConnected(ConnectedCallback callback) { m_onConnected = std::move(callback); }
    void setOnDisconnected(DisconnectedCallback callback) { m_onDisconnected = std::move(callback); }
    void setOnPacket(PacketCallback callback) { m_onPacket = std::move(callback); }

private:
    NetworkSession() = default;

    _ENetHost* m_host = nullptr;
    _ENetPeer* m_peer = nullptr;
    bool m_connected = false;

    ConnectedCallback m_onConnected;
    DisconnectedCallback m_onDisconnected;
    PacketCallback m_onPacket;
};
