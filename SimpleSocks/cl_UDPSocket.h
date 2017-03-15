#pragma once
#include <string>
#include <vector>
#include "cl_HostAddress.h"
#include "ns_Utility.h"

namespace SSocks {
  /**
   * Class representing a UDP socket
   */
  class UDPSocket {
  public:
    //! Generate an inactive UDP socket object
    UDPSocket();

    //! Copying is prohibited, as sockets are unique resources.
    UDPSocket(const UDPSocket&) = delete;

    //! Copying is prohibited, as sockets are unique resources.
    UDPSocket& operator=(const UDPSocket&) = delete;

    /**
     * Move constructor to transfer ownership to a new UDPSocket.
     * @param moveFrom The object to transfer the resource from.
     */
    UDPSocket(UDPSocket&& moveFrom);

    /**
    * Move-assign operator to transfer ownership to a new UDPSocket.
    * @param moveFrom The object to transfer the resource from.
    */
    void operator=(UDPSocket&& moveFrom);

    //! Destructor.
    ~UDPSocket();

    /**
     * Ready the socket for use.
     * @param port The port to bind the socket to.\n
     * Port 0 requests an 'ephemeral' port; that is, the port number will be assigned by the
     * system when the first operation requiring it takes place. If you plan to act as a listener
     * then you should bind a specific port. If you intend to start out by sending then binding an
     * ephemeral port is fine. Note that after sending to a remote host that host will have the 
     * port number you are bound to and can thus respond, even if you yourself don't know what
     * port you are on.
     * @param forceBind Whether to force the bind even if the indicated port appears to be in use.\n
     * This option is present because a bound port takes a certain amount of time to become free again after being released. In general it should not be necessary to use it.
     * @param localHostAddr Address of local interface to listen on.\n
     * This must be an IPv4 address in dot-quad notation, such as "127.0.0.1". The default address of "0.0.0.0" will listen on all available interfaces.
     */
    void open(uint16_t port = 0, bool forceBind = false, const std::string& localHostAddr = "0.0.0.0");

    /**
     * Indicates whether the socket is active and may be used to send and recieve.
     * @return true if the socket is active; false if it is not.
     */
    bool isOpen() const;

    /**
     * Close and restore socket to initial state.
     * Socket will revert to blocking, disconnected state.
     */
    void close();

    /**
    * Attempt to send data to the indicated host.
    * UDP is a 'fire-and-forget' protocol. Data sent is not guaranteed to arrive.
    * The remote host may not even exist, and no error will be indicated.
    * UDP is a datagram-oriented protocol. The data sent should be short. The maximum length
    * of a datagram is 65535 bytes, but this is drastically reduced by the internet. You should
    * probably try to send less than 1024 bytes per datagram.
    * @param host The host/port to send to.
    * @param data A pointer to the data to send.
    * @param len The number of bytes to send.
    * @return The number of bytes sent, which may be fewer than requested.
    */
    size_t sendTo(const HostAddress& host, const char* data, size_t len);

    /**
     * Attempt to send data to the indicated host.
     * UDP is a 'fire-and-forget' protocol. Data sent is not guaranteed to arrive.
     * The remote host may not even exist, and no error will be indicated.
     * UDP is a datagram-oriented protocol. The data sent should be short. The maximum length
     * of a datagram is 65535 bytes, but this is drastically reduced by the internet. You should
     * probably try to send less than 1024 bytes per datagram.
     * @param host The host/port to send to.
     * @param data The data to send. Note that std::string is not null terminated.
     * @return The number of bytes sent, which may be fewer than requested.
     */
    size_t sendTo(const HostAddress& host, const std::string& data);

    /**
     * Attempt to send data to the indicated host.
     * UDP is a 'fire-and-forget' protocol. Data sent is not guaranteed to arrive.
     * The remote host may not even exist, and no error will be indicated.
     * UDP is a datagram-oriented protocol. The data sent should be short. The maximum length
     * of a datagram is 65535 bytes, but this is drastically reduced by the internet. You should
     * probably try to send less than 1024 bytes per datagram.
     * @param host The host/port to send to.
     * @param data The data to send.
     * @return The number of bytes sent, which may be fewer than requested.
     */
    size_t sendTo(const HostAddress& host, const std::vector<char>& data);

    /**
     * Attempt to read an incoming datagram.
     * @return The data recieved and the address of the sender.\n
     * In non-blocking mode the data may be empty, indicating that no incoming datagram was pending.
     */
    std::pair<std::vector<char>, HostAddress> recvFrom();

    /**
     * Associate the socket with specific host.
     * UDP sockets do not 'connect' in the sense that TCP sockets do, but a UDP socket
     * can be associated with a particular remote address such that it will ignore traffic
     * from other addresses and can send to that address without specifying a recipient.
     * @param host Host/port to associate with.
     */
    void connect(const HostAddress& host);

    /**
     * Check whether or not a specific host is associated.
     * @see connect()
     * @return true if connected; false if not
     */
    bool isConnected() const;

    /**
     * Remove association with a specific host.
     * @see connect()
     */
    void disconnect();

    /**
     * Send data to the associated host.
     * UDPSocket::send() will throw an exception if the socket is not connected to a specific host.
     * @see connect()
     * @see sendTo()
     * @param data The data to send, note that std::string is not null-terminated.
     * @return the number of bytes sent.
     */
    int send(const std::string& data);

    /**
     * Send data to the associated host.
     * UDPSocket::send() will throw an exception if the socket is not connected to a specific host.
     * @see connect()
     * @see sendTo()
     * @param data The data to send.
     * @return the number of bytes sent.
     */
    int send(const std::vector<char>& data);

    /**
     * Send data to the associated host.
     * UDPSocket::send() will throw an exception if the socket is not connected to a specific host.
     * @see connect()
     * @see sendTo()
     * @param data The data to send.
     * @param len The length in bytes of the data to send.
     * @return the number of bytes sent.
     */
    int send(const void* data, size_t len);

    /**
     * Recieve a datagram from the associated host.
     * UDPSocket::recv() will throw an exception if the socket is not connected to a specific host.
     * @see connect()
     * @see recvFrom()
     * @return Vector containing the recieved data.
     */
    std::vector<char> recv();

    /**
    * Indicates whether or not the socket is in blocking mode.
    * @see setBlocking()
    * @return true if the socket can block; false if it is in non-blocking mode
    */
    bool isBlocking() const;

    /**
    * Set whether or not the socket is in blocking mode.
    * A blocking socket will pause its thread on send/recv until it can complete its task.
    * That is, a blocking recvFrom() will wait until it recieves a datagram before returning.
    * A non-blocking socket will return immediately from send/recv, even if it was unable to
    * actually send or recieve any data. All sockets default to blocking.
    * @param block Set true to allow the socket to block; false for non-blocking mode
    */
    void setBlocking(bool block);


  private:
    SSocks::Utility::Winsock wsa;

    int sock;
    bool blocking;
    bool connected;

    template<class T> friend std::vector<T*> select(const std::vector<T*>& sockets, float timeoutSeconds);

  };

}
