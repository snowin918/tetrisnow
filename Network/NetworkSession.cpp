#include "Network/NetworkSession.h"

#include <cstdio>
#include <cstdlib>

#include <enet/enet.h>

namespace
{
bool g_enetInitialized = false;

bool ensureEnetInitialized()
{
    if (!g_enetInitialized) {
        if (enet_initialize() != 0) {
            std::fprintf(stderr, "NetworkSession: enet_initialize failed\n");
            return false;
        }
        g_enetInitialized = true;
        std::atexit([] { enet_deinitialize(); });
    }
    return true;
}

constexpr size_t kChannelCount = 2;
constexpr enet_uint8 kReliableChannel = 0;
constexpr enet_uint8 kUnreliableChannel = 1;
} // namespace

NetworkSession::~NetworkSession()
{
    if (m_host != nullptr) {
        enet_host_destroy(reinterpret_cast<ENetHost*>(m_host));
    }
}

std::unique_ptr<NetworkSession> NetworkSession::createHost(uint16_t port)
{
    if (!ensureEnetInitialized()) {
        return nullptr;
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    ENetHost* host = enet_host_create(&address, 1, kChannelCount, 0, 0);
    if (host == nullptr) {
        std::fprintf(stderr, "NetworkSession: failed to host on port %u (already in use?)\n", port);
        return nullptr;
    }

    auto session = std::unique_ptr<NetworkSession>(new NetworkSession());
    session->m_host = reinterpret_cast<_ENetHost*>(host);
    return session;
}

std::unique_ptr<NetworkSession> NetworkSession::createClient(const std::string& hostAddress, uint16_t port)
{
    if (!ensureEnetInitialized()) {
        return nullptr;
    }

    ENetHost* host = enet_host_create(nullptr, 1, kChannelCount, 0, 0);
    if (host == nullptr) {
        std::fprintf(stderr, "NetworkSession: failed to create client host\n");
        return nullptr;
    }

    ENetAddress address;
    if (enet_address_set_host(&address, hostAddress.c_str()) != 0) {
        std::fprintf(stderr, "NetworkSession: could not resolve host '%s'\n", hostAddress.c_str());
        enet_host_destroy(host);
        return nullptr;
    }
    address.port = port;

    ENetPeer* peer = enet_host_connect(host, &address, kChannelCount, 0);
    if (peer == nullptr) {
        std::fprintf(stderr, "NetworkSession: enet_host_connect failed\n");
        enet_host_destroy(host);
        return nullptr;
    }

    auto session = std::unique_ptr<NetworkSession>(new NetworkSession());
    session->m_host = reinterpret_cast<_ENetHost*>(host);
    session->m_peer = reinterpret_cast<_ENetPeer*>(peer);
    return session;
}

void NetworkSession::poll()
{
    if (m_host == nullptr) {
        return;
    }

    ENetEvent event;
    while (enet_host_service(reinterpret_cast<ENetHost*>(m_host), &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                m_peer = reinterpret_cast<_ENetPeer*>(event.peer);
                m_connected = true;
                if (m_onConnected) {
                    m_onConnected();
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE: {
                if (m_onPacket) {
                    const std::vector<uint8_t> bytes(
                        event.packet->data, event.packet->data + event.packet->dataLength);
                    m_onPacket(bytes);
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                m_connected = false;
                m_peer = nullptr;
                if (m_onDisconnected) {
                    m_onDisconnected();
                }
                break;
            default:
                break;
        }
    }
}

void NetworkSession::flush()
{
    if (m_host != nullptr) {
        enet_host_flush(reinterpret_cast<ENetHost*>(m_host));
    }
}

void NetworkSession::sendReliable(const std::vector<uint8_t>& bytes)
{
    if (m_peer == nullptr) {
        return;
    }
    ENetPacket* packet = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(reinterpret_cast<ENetPeer*>(m_peer), kReliableChannel, packet);
}

void NetworkSession::sendUnreliable(const std::vector<uint8_t>& bytes)
{
    if (m_peer == nullptr) {
        return;
    }
    ENetPacket* packet = enet_packet_create(bytes.data(), bytes.size(), 0);
    enet_peer_send(reinterpret_cast<ENetPeer*>(m_peer), kUnreliableChannel, packet);
}
