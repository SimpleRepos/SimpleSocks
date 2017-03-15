#pragma once
#include <string>
#include "ns_Utility.h"
#include "cl_TCPSocket.h"

namespace SSocks {
  //! Class representing a bound TCP socket that listens for incoming TCP connections.
  class TCPServer {
  public:
    //! Generate an inactive server object
    TCPServer();

    /**
     * Generate a server and start it with the provided arguments
     * @param port The port to listen on.
     * @param forceBind Whether to force the bind even if the indicated port appears to be in use.
     * @param localHostAddr Address of local interface to listen on.
     * @see start()
     */
    TCPServer(uint16_t port, bool forceBind = false, const std::string& localHostAddr = "0.0.0.0");

    //! Copying is prohibited, as sockets are unique resources.
    TCPServer(const TCPServer&) = delete;

    //! Copying is prohibited, as sockets are unique resources.
    TCPServer& operator=(const TCPServer&) = delete;

    /**
    * Move constructor to transfer ownership to a new TCPServer.
    * @param moveFrom The object to transfer the resource from.
    */
    TCPServer(TCPServer&& moveFrom);

    /**
    * Move-assign operator to transfer ownership to a new TCPServer.
    * @param moveFrom The object to transfer the resource from.
    */
    void operator=(TCPServer&& moveFrom);

    //! Destructor.
    ~TCPServer();

    /**
     * Bind to the indicated port on interface 'localHostAddr'.
     * @param port The port to listen on.
     * @param forceBind Whether to force the bind even if the indicated port appears to be in use.
     * This option is present because a bound port takes a certain amount of time to become free again after being released. In general it should not be necessary to use it.
     * @param localHostAddr Address of local interface to listen on.
     * This must be an IPv4 address in dot-quad notation, such as "127.0.0.1". The default address of "0.0.0.0" will listen on all available interfaces.
     */
    void start(uint16_t port, bool forceBind = false, const std::string& localHostAddr = "0.0.0.0");

    /**
     * Stop listening and release the socket.
     * Blocking will be reset to true.
     */
    void stop();

    /**
     * Accept an incoming connection and return it as a new TCPSocket object.
     * If blocking is on then this function will block until an incoming connection arrives.
     * Otherwise it may potentially return an unconnected socket object. Be sure to check isOpen()
     * on the returned socket in both cases, as sometimes the remote machine may not complete the
     * connection.\n
     * Note that TCPServer can be used with SSocks::select(). The vector returned by select() will
     * contain only those servers that have incoming connections pending.
     * @return A TCPSocket object representing the incoming connection.
     */
    TCPSocket accept();

    /**
     * Indicates whether the server is bound to a port and listening for connections.
     * @return true if the server is listening; false if it is not.
     */
    bool isOpen() const;

    /**
     * Indicates whether or not the server is in blocking mode.
     * @see setBlocking()
     * @return true if the server can block; false if it is in non-blocking mode
     */
    bool isBlocking() const;

    /**
     * Set whether or not the server is in blocking mode.
     * A blocking server will pause its thread on accept() until it catches an incoming
     * connection attempt. A non-blocking server will return immediately, even if no incoming
     * connections were available. All sockets, including servers, default to blocking.
     * @param block Set true to allow the server to block; false for non-blocking mode
     */
    void setBlocking(bool block);

  private:
    SSocks::Utility::Winsock wsa;

    int sock;
    bool blocking;

    template<class T> friend std::vector<T*> select(const std::vector<T*>& sockets, float timeoutSeconds);

  };

}