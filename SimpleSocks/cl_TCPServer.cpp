#include "cl_TCPServer.h"
#include <WS2tcpip.h>

//Set members to default values.
//SOCKET_ERROR is used here to indicate that the socket is closed
SSocks::TCPServer::TCPServer() : sock(SOCKET_ERROR), blocking(true) {
  //nothing
}

//invoke the default constructor and then call start()
SSocks::TCPServer::TCPServer(uint16_t port, bool forceBind, const std::string& localHostAddr) : TCPServer() {
  start(port, forceBind, localHostAddr);
}

//release the socket
SSocks::TCPServer::~TCPServer() {
  stop();
}

//copy values from source and then break its ownership of the socket
SSocks::TCPServer::TCPServer(TCPServer&& moveFrom) : sock(moveFrom.sock), blocking(moveFrom.blocking) {
  //force source to disown resource so that it won't be released when source destructs
  moveFrom.sock = SOCKET_ERROR;
}

void SSocks::TCPServer::operator=(TCPServer&& moveFrom) {
  sock = moveFrom.sock;
  blocking = moveFrom.blocking;

  moveFrom.sock = SOCKET_ERROR;
}

void SSocks::TCPServer::start(uint16_t port, bool forceBind, const std::string& localHostAddr) {
  //halt service if already running
  if(isOpen()) { stop(); }

  //We need a sockaddr_in to bind the server to a port and interface.
  sockaddr_in sain = {0};
  sain.sin_family = AF_INET;
  sain.sin_port = htons(port);

  if(port == 0) { throw std::runtime_error("SSocks::TCPSever does not support port zero."); }

  //This shouldn't fail unless the user types in gibberish, but let's be safe.
  int result = inet_pton(AF_INET, localHostAddr.c_str(), &sain.sin_addr);
  switch(result) {
  case 0: throw std::runtime_error("Attempted to start server on interaface with invalid address string.");
  case -1: throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  //TSock helps here because if an exception is thrown it ensures that the socket resource is released.
  Utility::TSock tsock(SOCK_STREAM, IPPROTO_TCP);

  //forceBind will allow the bind to take over from an existing bind. See comments in header.
  if(forceBind) {
    //Here's another nasty legacy call...
    BOOL temp = TRUE;
    result = setsockopt(tsock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&temp), sizeof(temp));
    if(result) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }
  }

  //bind the socket
  result = bind(tsock, reinterpret_cast<sockaddr*>(&sain), sizeof(sain));
  if(result) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }

  //start listening for connections
  result = listen(tsock, SOMAXCONN);
  if(result) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }

  //everything seems okay, so take ownership of the resource
  sock = tsock.validate();
}

void SSocks::TCPServer::stop() {
  //release the resource if it exists
  if(isOpen()) { closesocket(sock); }

  //and reset to defaults
  sock = SOCKET_ERROR;
  blocking = true;
}

SSocks::TCPSocket SSocks::TCPServer::accept() {
  if(!isOpen()) { throw std::runtime_error("Attemtped to wait for connections on closed TCPServer."); }

  //Create a TCPSocket object
  TCPSocket nuSock;

  //and inject the incoming connection into it
  nuSock.sock = ::accept(sock, nullptr, nullptr);
  if(nuSock.sock == SOCKET_ERROR) {
    //WSAEWOULDBLOCK happens on a non-blocking socket when there's no incoming connection.
    //We can just return the unconnected socket to indicate that. (It will simply be an unopened TCPSocket.)
    int err = WSAGetLastError();
    if(err != WSAEWOULDBLOCK) { throw std::runtime_error(Utility::lastErrStr(err)); }
  }

  return nuSock;
}

bool SSocks::TCPServer::isOpen() const {
  return sock != SOCKET_ERROR;
}

bool SSocks::TCPServer::isBlocking() const {
  return blocking;
}

void SSocks::TCPServer::setBlocking(bool block) {
  if(!isOpen()) { throw std::runtime_error("Attemtped to set blocking state on closed TCPServer."); }

  //avoid needless system calls
  if(block == blocking) { return; }

  //I really hate all this legacy crap that tries to make one function do everything.
  //It makes the interfaces really unpleasant.
  unsigned long temp = block ? 0 : 1;
  //set actual socket behavior according to state info
  int result = ioctlsocket(sock, FIONBIO, &temp);
  if(result == SOCKET_ERROR) {
    stop(); //assume socket is invalidated
    throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  //update state info
  blocking = block;

}
