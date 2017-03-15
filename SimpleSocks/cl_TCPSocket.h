#pragma once
#include <vector>
#include <string>
#include "ns_Utility.h"
#include "cl_HostAddress.h"

namespace SSocks {

  //! Class representing a TCP connection.
  class TCPSocket {
  public:
    //! Generate socket without connection.
    TCPSocket();

    /**
     * Generate socket and connect to indicated host.
     * @param host A HostAddress object indicating the host and port to connect to.
     */
    TCPSocket(const HostAddress& host);

    //! Copying is prohibited, as sockets are unique resources.
    TCPSocket(const TCPSocket&) = delete;

    //! Copying is prohibited, as sockets are unique resources.
    TCPSocket& operator=(const TCPSocket&) = delete;

    /**
     * Move constructor to transfer ownership to a new TCPSocket.
     * @param moveFrom The object to transfer the resource from.
     */
    TCPSocket(TCPSocket&& moveFrom);

    /**
    * Move-assign operator to transfer ownership to a new TCPSocket.
    * @param moveFrom The object to transfer the resource from.
    */
    void operator=(TCPSocket&& moveFrom);

    //! Destructor.
    ~TCPSocket();

    /**
     * Connect to indicated host.
     * @param host A HostAddress object indicating the host and port to connect to.
     */
    void connect(const HostAddress& host);

    /**
     * Close the present connection.
     * If no connection exists then no action will be taken.
     * Blocking will be set to true on closure.
     */
    void close();

    /**
     * Send data through the socket to the connected machine.
     * If the socket is blocking then all data will be sent. Otherwise
     * a single send will be issued and the function will return.
     * @param data A pointer to the data to be sent.
     * @param len The number of bytes to send.
     * @return The number of bytes sent.
     */
    size_t send(const void* data, size_t len);

    /**
    * Send data through the socket to the connected machine.
    * If the socket is blocking then all data will be sent. Otherwise
    * a single send will be issued and the function will return.
    * @param data A std::string to send. Note that this will not send a null terminator.
    * @return The number of bytes sent.
    */
    size_t send(const std::string& data);

    /**
    * Send data through the socket to the connected machine.
    * If the socket is blocking then all data will be sent. Otherwise
    * a single send will be issued and the function will return.
    * @param data A vector of char holding the data to send.
    * @return The number of bytes sent.
    */
    size_t send(const std::vector<char> data);

    /**
     * Recieve up to 'len' bytes of data from the remote machine.
     * If the socket is set to block then this function will continue
     * reading until it reaches 'len' bytes or the socket is closed by
     * the remote machine. Otherwise it will attempt to read and return
     * whatever it gets in a single pass (may be empty). Make sure to
     * check isOpen() after recv() to ensure that the remote host has
     * not closed the connection.
     * @param len The maximum number of bytes to read.
     * @return A vector of char containing the recived data.
     */
    std::vector<char> recv(size_t len);

    /**
     * Recieve up to 'len' bytes of data from the remote machine without removing them from the buffer.
     * Similar to recv(), but does not remove data from the hardware socket buffer.
     * This means that if you peek(100) three times in a row you should get the same
     * data three times in a row, assuming there are 100 or more bytes available to
     * read. SSocks::peek() will not attempt to continually read in order to fill its
     * returned buffer as SSocks::recv() does, even if it's set to block. It will simply
     * grab what's immediately available and return it.\n
     * NOTE THAT PEEK CAN CLOSE THE SOCKET JUST AS RECV WOULD.\n
     * Make sure to react appropriately to zero-length returns.
     * @param len The maximum number of bytes to read.
     * @return A vector of char containing the recived data.
     */
    std::vector<char> peek(size_t len);

    /**
     * Indicates whether the socket is connected to a remote host.
     * Calls to send() and recv() can update this value if the remote host closed the connection.
     * @return true if the socket is connected; false if it is not.
     */
    bool isOpen() const;

    /**
     * Indicates whether or not the socket is in blocking mode.
     * @see setBlocking()
     * @return true if the socket can block; false if it is in non-blocking mode
     */
    bool isBlocking() const;

    /**
    * Set whether or not the socket is in blocking mode.
    * A blocking socket will pause its thread on send/recv until it can complete its
    * task. That is, a blocking recv() will wait until it has recieved some data before
    * returning. A non-blocking socket will return immediately from send/recv, even if
    * it was unable to actually send or recieve any data. All sockets default to blocking.
    * @param block Set true to allow the socket to block; false for non-blocking mode
    */
    void setBlocking(bool block);

  private:
    Utility::Winsock wsa;

    int sock;
    bool blocking;

    std::vector<char> fullRecv(size_t len);
    std::vector<char> singlePassRecv(size_t len, int flags);

    template<class T> friend std::vector<T*> select(const std::vector<T*>& sockets, float timeoutSeconds);

    friend class TCPServer;

  };

}
