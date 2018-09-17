/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Session.h
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 * @author toxotguo
 * @date 2018
 */

#pragma once

#include "Common.h"
#include "SessionFace.h"
#include "SocketFace.h"
#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include <boost/heap/priority_queue.hpp>
#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
namespace dev
{
namespace p2p
{
class Peer;
class Host;
/**
 * @brief The Session class
 * @todo Document fully.
 */
class Session : public SessionFace, public std::enable_shared_from_this<Session>
{
public:
    Session(Host* _server, std::shared_ptr<SocketFace> const& _s, std::shared_ptr<Peer> const& _n,
        PeerSessionInfo const& _info);

    virtual ~Session();

    void start() override;
    void disconnect(DisconnectReason _reason) override;

    void ping() override;

    bool isConnected() const override { return m_socket->isConnected(); }

    NodeID id() const override;

    void sealAndSend(RLPStream& _s, uint16_t _protocolID) override;

    void addNote(std::string const& _k, std::string const& _v) override
    {
        Guard l(x_info);
        m_info.notes[_k] = _v;
    }

    PeerSessionInfo info() const override
    {
        Guard l(x_info);
        return m_info;
    }
    std::chrono::steady_clock::time_point connectionTime() override { return m_connect; }

    std::shared_ptr<Peer> peer() const override { return m_peer; }

    std::chrono::steady_clock::time_point lastReceived() const override { return m_lastReceived; }


private:
    struct Header
    {
        uint32_t length = 0;
        uint32_t protocolID = 0;
    };
    static RLPStream& prep(RLPStream& _s, PacketType _t, unsigned _args = 0);

    void send(std::shared_ptr<bytes> _msg, uint16_t _protocolID);

    /// Drop the connection for the reason @a _r.
    void drop(DisconnectReason _r);

    /// Perform a read on the socket.
    void doRead();

    /// Check error code after reading and drop peer if error code.
    bool checkRead(std::size_t _expected, boost::system::error_code _ec, std::size_t _length);

    /// Perform a single round of the write operation. This could end up calling itself
    /// asynchronously.
    void onWrite(boost::system::error_code ec, std::size_t length);
    void write();

    /// Deliver packet to Session or Capability for interpretation.
    bool readPacket(uint16_t _capId, PacketType _t, RLP const& _r);

    /// Interpret an incoming Session packet.
    bool interpret(PacketType _t, RLP const& _r);

    /// @returns true iff the _msg forms a valid message for sending or receiving on the network.
    static bool checkPacket(bytesConstRef _msg);

    Host* m_server;                        ///< The host that owns us. Never null.
    std::shared_ptr<SocketFace> m_socket;  ///< Socket of peer's connection.
    Mutex x_framing;                       ///< Mutex for the write queue.

#if 0
	std::deque<bytes> m_writeQueue;			///< The write queue.
	std::deque<u256> m_writeTimeQueue; ///< to stat queue time
#endif
    class QueueCompare
    {
    public:
        bool operator()(const boost::tuple<std::shared_ptr<bytes>, uint16_t, u256>& lhs,
            const boost::tuple<std::shared_ptr<bytes>, uint16_t, u256>& rhs) const
        {
            if (boost::get<1>(lhs) == 0x13 || boost::get<1>(lhs) == 0x15)
            {
                return true;
            }

            return false;
        }
    };

    boost::heap::priority_queue<boost::tuple<std::shared_ptr<bytes>, uint16_t, u256>,
        boost::heap::compare<QueueCompare>, boost::heap::stable<true>>
        m_writeQueue;

    std::vector<byte> m_data;  ///< Buffer for ingress packet data.

    bytes m_incoming;  ///< Read buffer for ingress bytes.

    std::shared_ptr<Peer> m_peer;  ///< The Peer object.
    bool m_dropped = false;  ///< If true, we've already divested ourselves of this peer. We're just
                             ///< waiting for the reads & writes to fail before the shared_ptr goes
                             ///< OOS and the destructor kicks in.

    mutable Mutex x_info;
    PeerSessionInfo m_info;  ///< Dynamic information about this peer.

    std::chrono::steady_clock::time_point m_connect;       ///< Time point of connection.
    std::chrono::steady_clock::time_point m_ping;          ///< Time point of last ping.
    std::chrono::steady_clock::time_point m_lastReceived;  ///< Time point of last message.

    unsigned m_start_t;

    boost::asio::io_service::strand* m_strand;
};

}  // namespace p2p
}  // namespace dev
