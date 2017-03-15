#include "cl_UDPSocket.h"
#include <WS2tcpip.h>

//set default values
SSocks::UDPSocket::UDPSocket() : sock(SOCKET_ERROR), blocking(true), connected(false) {
  //nothing
}

//copy source object values and then break its ownership
SSocks::UDPSocket::UDPSocket(UDPSocket&& moveFrom) : sock(moveFrom.sock), blocking(moveFrom.blocking), connected(moveFrom.connected) {
  //remove resource ownership from the source
  moveFrom.sock = SOCKET_ERROR;
}


//copy source object values and then break its ownership
void SSocks::UDPSocket::operator=(UDPSocket&& moveFrom) {
  sock = moveFrom.sock;
  blocking = moveFrom.blocking;
  connected = moveFrom.connected;

  //remove resource ownership from the source
  moveFrom.sock = SOCKET_ERROR;
}

//close socket
SSocks::UDPSocket::~UDPSocket() {
  close();
}

void SSocks::UDPSocket::open(uint16_t port, bool forceBind, const std::string& localHostAddr) {
  //release any existing socket
  close();

  //We need a sockaddr_in to bind the port and interface.
  sockaddr_in sain = { 0 };
  sain.sin_family = AF_INET;
  sain.sin_port = htons(port);

  //This shouldn't fail unless you type in gibberish, but let's be safe.
  int result = inet_pton(AF_INET, localHostAddr.c_str(), &sain.sin_addr);
  switch(result) {
  case 0: throw std::runtime_error("Attempted to bind UDPSocket on interaface with invalid address string.");
  case -1: throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  //TSock will release the resource if the bind fails.
  Utility::TSock temp(SOCK_DGRAM, IPPROTO_UDP);
  
  if(port != 0) { //ephemeral binding is assumed, so if port is zero then we can skip this.
    result = bind(temp, reinterpret_cast<sockaddr*>(&sain), sizeof(sain));
    if(result) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }
  }

  //looks like we're okay, so take ownership of the resource
  sock = temp.validate();
}

bool SSocks::UDPSocket::isOpen() const {
  return sock != SOCKET_ERROR;
}

void SSocks::UDPSocket::close() {
  //close socket and reset all state
  if(isOpen()) { closesocket(sock); }

  sock = SOCKET_ERROR;
  blocking = true;
  connected = false;
}

size_t SSocks::UDPSocket::sendTo(const HostAddress& host, const char* data, size_t len) {
  if(!isOpen()) { throw std::runtime_error("Attempted sendTo on unopened UDP socket."); }

  size_t sent = ::sendto(sock, data, len, 0, host, host.size());
  if(sent == SOCKET_ERROR) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }

  return sent;
}

//delegate string call to char*
size_t SSocks::UDPSocket::sendTo(const HostAddress& host, const std::string& data) {
  return sendTo(host, data.data(), data.size());
}

//delegate vector call to char*
size_t SSocks::UDPSocket::sendTo(const HostAddress& host, const std::vector<char>& data) {
  return sendTo(host, data.data(), data.size());
}

std::pair<std::vector<char>, SSocks::HostAddress> SSocks::UDPSocket::recvFrom() {
  if(!isOpen()) { throw std::runtime_error("Attempted recvFrom on unopened UDP socket."); }

  //create an address object for storing data about the origin of the datagram
  sockaddr_in from = { 0 };
  int fromLen = sizeof(from);

  //prepare a buffer that can hold the largest possible datagram
  const size_t MAX_UDP_DATAGRAM_LENGTH = 0xFFFF; //(this is actually slightly larger than needed)
  std::vector<char> buffer(MAX_UDP_DATAGRAM_LENGTH);

  //read into the buffer
  int result = ::recvfrom(sock, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
  if(result == SOCKET_ERROR) {
    int err = WSAGetLastError();
    if(err == WSAEWOULDBLOCK) {
      //non-blocking socket had no data incoming, so just return an empty result
      result = 0;
    }
    else {
      //otherwise assume the socket is invalidated and throw
      close();
      throw std::runtime_error(Utility::lastErrStr(err));
    }
  }

  //size down the buffer to fit the recieved data
  buffer.resize(result);

  //return the buffer and a HostAddress pointing to the sender
  return std::make_pair(buffer, HostAddress(&from));
}

void SSocks::UDPSocket::connect(const HostAddress& host) {
  if(!isOpen()) { throw std::runtime_error("Attempted connection on unopened UDP socket."); }

  //Any existing connection will simply be overridden by ::connect().

  int result = ::connect(sock, host, sizeof(host));
  if(result) {
    close(); //assume socket is invalidated
    throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  connected = true;
}

bool SSocks::UDPSocket::isConnected() const {
  return connected;
}

void SSocks::UDPSocket::disconnect() {
  if(isConnected()) {
    connect(HostAddress("0.0.0.0", 0)); //connecting to zero removes the connection behavior
  }

  connected = false;
}

//delegate std::string to (char*, size_t)
int SSocks::UDPSocket::send(const std::string& data) {
  return send(data.data(), data.size());
}

//delegate std::vector<char> to (char*, size_t)
int SSocks::UDPSocket::send(const std::vector<char>& data) {
  return send(data.data(), data.size());
}

int SSocks::UDPSocket::send(const void* data, size_t len) {
  if(!connected) { throw std::runtime_error("Attempted send on unconnected UDP socket. (did you mean to use sendTo?)"); }

  int result = ::send(sock, reinterpret_cast<const char*>(data), len, 0);
  if(result == SOCKET_ERROR) {
    close(); //assume socket is invalidated
    throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  return result;
}

std::vector<char> SSocks::UDPSocket::recv() {
  if(!connected) { throw std::runtime_error("Attempted recv on unconnected UDP socket. (did you mean to use recvFrom?)"); }

  //allocate a buffer to hold the data
  const size_t MAXIMUM_DATAGRAM_LENGTH = 0xFFFF;
  std::vector<char> buffer(MAXIMUM_DATAGRAM_LENGTH);

  //read into it
  int result = ::recv(sock, buffer.data(), buffer.size(), 0);
  if(result == SOCKET_ERROR) {
    int err = WSAGetLastError();
    if(err == WSAEWOULDBLOCK) {
      //this is a non-blocking socket and no data was pending, so return an empty vector
      result = 0;
    }
    else {
      close(); //assume socket is invalidated
      throw std::runtime_error(Utility::lastErrStr(err));
    }
  }

  //shrink the vector to fit the data that was read
  buffer.resize(result);

  return buffer;
}

bool SSocks::UDPSocket::isBlocking() const {
  return blocking;
}

void SSocks::UDPSocket::setBlocking(bool block) {
  if(!isOpen()) { throw std::runtime_error("Attemtped to set blocking state on closed UDPSocket."); }

  //avoid needless system calls
  if(block == blocking) { return; }

  //I really hate all this legacy crap that tries to make one function do everything.
  //It makes the interfaces really unpleasant.
  unsigned long temp = block ? 0 : 1;
  //set actual socket behavior according to state info
  int result = ioctlsocket(sock, FIONBIO, &temp);
  if(result == SOCKET_ERROR) {
    close();  //assume socket is invalidated
    throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
  }

  //update state info
  blocking = block;

}

