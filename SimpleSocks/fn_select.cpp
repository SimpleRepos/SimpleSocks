#include "fn_select.h"
#include <WS2tcpip.h>

const float SSocks::SELECT_FOREVER = -1.0f;


template<class T>
std::vector<T*> SSocks::select(const std::vector<T*>& sockets, float timeoutSeconds) {
  //prevent derp
  if(sockets.empty()) { return std::vector<T*>(); }

  //To select without a timeout a null pointer is passed.
  //I declare the timeval here and a pointer defaulted to null...
  timeval timeout;
  timeval* tvp = nullptr;

  //...If the user provided timeout value is legitimate then the struct
  //is populated and the pointer is set to its address.
  if(timeoutSeconds >= 0) {
    timeout.tv_sec = static_cast<long>(timeoutSeconds);

    timeoutSeconds -= timeout.tv_sec;
    const int USEC_PER_SEC = 1000000;
    timeoutSeconds *= USEC_PER_SEC;

    timeout.tv_usec = static_cast<long>(timeoutSeconds);

    tvp = &timeout;
  }

  //populate the fd_set from the provided vector
  fd_set set = {0};
  for(auto sock : sockets) {
    if(sock->isOpen()) {
      FD_SET(sock->sock, &set);
    }
  }

  //perform the selection - this removes non-ready sockets from the fd_set
  int result = ::select(0, &set, nullptr, nullptr, tvp);
  if(result == SOCKET_ERROR) { throw std::runtime_error(SSocks::Utility::lastErrStr(WSAGetLastError())); }

  //fill the result vector based on the results
  std::vector<T*> pending;
  for(auto sock : sockets) {
    if(FD_ISSET(sock->sock, &set)) {
      pending.push_back(sock);
    }
  }

  return pending;
}



//Force template instantiation for each of the relevant types
#include "cl_TCPSocket.h"
#include "cl_TCPServer.h"
#include "cl_UDPSocket.h"

namespace {
  auto a = SSocks::select(std::vector<SSocks::TCPSocket*>());
  auto b = SSocks::select(std::vector<SSocks::TCPServer*>());
  auto c = SSocks::select(std::vector<SSocks::UDPSocket*>());
}


