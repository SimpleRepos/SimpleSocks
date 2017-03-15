#include "cl_TCPSocket.h"
#include <WS2tcpip.h>

//Set default values
SSocks::TCPSocket::TCPSocket() : sock(SOCKET_ERROR), blocking(true) {
  //nothing
}

//invoke default constructor and then call connect()
SSocks::TCPSocket::TCPSocket(const HostAddress& host) : TCPSocket() {
  connect(host);
}

//close the socket
SSocks::TCPSocket::~TCPSocket() {
  close();
}

//copy values from the other object and then break its ownership of the socket
SSocks::TCPSocket::TCPSocket(TCPSocket&& moveFrom) : sock(moveFrom.sock), blocking(moveFrom.blocking) {
  //remove ownership from source so the resource won't be released when the source destructs
  moveFrom.sock = SOCKET_ERROR;
}

//copy values from the other object and then break its ownership of the socket
void SSocks::TCPSocket::operator=(TCPSocket&& moveFrom) {
  sock = moveFrom.sock;
  blocking = moveFrom.blocking;

  //remove ownership from source so the resource won't be released when the source destructs
  moveFrom.sock = SOCKET_ERROR;
}

void SSocks::TCPSocket::connect(const HostAddress& host) {
  //discard any existing connection and reset state
  if(isOpen()) { close(); }

  //Using TSock here will ensure that the socket is released even if the funciton doesn't succeed
  Utility::TSock tsock(SOCK_STREAM, IPPROTO_TCP);

  //try to connect to 'host'
  int err = ::connect(tsock, host, host.size());
  if(err) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }

  //everything looks okay, so take ownership of the resource and return
  sock = tsock.validate();
}

void SSocks::TCPSocket::close() {
  //release the resource if it exists
  if(isOpen()) { closesocket(sock); }

  //and reset to defaults
  sock = SOCKET_ERROR;
  blocking = true;
}

size_t SSocks::TCPSocket::send(const void* data, size_t len) {
  if(!isOpen()) { throw std::runtime_error("Attempted send on closed TCPSocket."); }

  //We can't advance a void*, and ::send() wants a char* anyway,
  //so we'll cast to and work with one.
  const char* datap = reinterpret_cast<const char*>(data);

  //keep track of how many bytes are sent
  int totalSent = 0;

  //If we're blocking then send everything, even if it takes multiple attempts.
  //Otherwise send what it will take for now and then indicate how much it was.
  //This may actually not be necessary in practical terms, as actually filling
  //the outbound buffer appears to be rather difficult, but because of the way
  //the documentation is written on MSDN I've added it just in case.
  do {
    //send the data
    int sent = ::send(sock, datap, len, 0);
    if(sent == SOCKET_ERROR) {
      //EWOULDBLOCK indicates that we're non-blocking and the outbound buffer
      //is full, so just return and indicate that no bytes were sent
      if(WSAGetLastError() == EWOULDBLOCK) { return 0; }
      //for all other errors...
      close(); //close the socket (assume it's now invalid ) and throw
      throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
    }
    
    totalSent += sent; //update return value
    len -= sent; //adjust length remaining
    datap += sent; //advance the pointer
  } while(len > 0 && blocking);

  return totalSent;
}

//overloads for send()
size_t SSocks::TCPSocket::send(const std::string& data)      { return send(data.data(), data.size()); }
size_t SSocks::TCPSocket::send(const std::vector<char> data) { return send(data.data(), data.size()); }

std::vector<char> SSocks::TCPSocket::recv(size_t len) {
  if(blocking) { return fullRecv(len); }
  else { return singlePassRecv(len, 0); }
}

std::vector<char> SSocks::TCPSocket::peek(size_t len) {
  return singlePassRecv(len, MSG_PEEK);
}

bool SSocks::TCPSocket::isOpen() const {
  return sock != SOCKET_ERROR;
}

bool SSocks::TCPSocket::isBlocking() const {
  return blocking;
}

void SSocks::TCPSocket::setBlocking(bool block) {
  if(!isOpen()) { throw std::runtime_error("Attemtped to set blocking state on closed TCPSocket."); }

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

std::vector<char> SSocks::TCPSocket::fullRecv(size_t len) {
  if(!isOpen()) { throw std::runtime_error("Attempted recv on closed TCPSocket."); }

  //allocate buffer to store data
  std::vector<char> data(len);
  
  //Need to be able to advance the data pointer in case we have to
  //read more than once. (If we want 20 bytes and we only get 10 the first time
  //then we want to advance the pointer by 10 so the next read continues where
  //the last one left off.)
  char* readTo = data.data();
  //and keep track of how much has been read
  size_t totalRead = 0;

  //If blocking and not peeking then read continuously until 'len' bytes or socket closure.
  //Otherwise (peeking OR non-blocking) read once and just return whatever comes back.
  do {
    //read to pointer position
    int got = ::recv(sock, readTo, len, 0);
    //if recv() returns zero it means that the remote host closed the connection
    //so we close the socket and break the loop
    if(got == 0) { close(); break; }

    if(got == SOCKET_ERROR) {
      close(); //assume the socket is invalidated and throw
      throw std::runtime_error(Utility::lastErrStr(WSAGetLastError()));
    }

    totalRead += got; //update our counter
    len -= got; //reduce the amount left to read
    readTo += got; //advance the pointer to the next read position

  } while(len);

  //we may not have filled the whole buffer, so resize it to match what we read
  data.resize(totalRead);

  return data;
}

std::vector<char> SSocks::TCPSocket::singlePassRecv(size_t len, int flags) {
  if(!isOpen()) { throw std::runtime_error("Attempted recv on closed TCPSocket."); }

  //allocate buffer to store data
  std::vector<char> data(len);

  //read to buffer
  int got = ::recv(sock, data.data(), data.size(), 0);

  //if recv() returns zero it means that the remote host closed the connection
  if(got == 0) { close(); }
  else if(got == SOCKET_ERROR) {
    int err = WSAGetLastError();
    if(err == WSAEWOULDBLOCK) {
      //blocking socket had no data pending, so just return empty vector
      got = 0;
    }
    else {
      close(); //assume the socket is invalidated and throw
      throw std::runtime_error(Utility::lastErrStr(err));
    }
  }

  //we may not have filled the whole buffer, so resize it to match what we read
  data.resize(got);

  return data;
}
