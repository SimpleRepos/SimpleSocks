#include "ns_Utility.h"
#include <WS2tcpip.h>

//link the winsock library
#pragma comment(lib, "Ws2_32.lib")

/////////////////////////LASTERRSTR/////////////////////////

std::string SSocks::Utility::lastErrStr(int code) {
  //creating a large buffer for the message
  std::string msg;
  const size_t maxFormatMessageLength = 1024 * 60;
  msg.resize(maxFormatMessageLength);

  //Welcome to "How to Write a Ridiculous Error System; Microsoft Edition":
  size_t len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, code,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &msg[0], msg.size(), 0);

  //trim the buffer to fit the message
  msg.resize(len);

  //Break if debug build
  #ifdef _DEBUG
  OutputDebugStringA(msg.c_str());
  DebugBreak();
  #endif

  return msg;
}


/////////////////////////WINSOCK/////////////////////////

SSocks::Utility::Winsock::Winsock() {
  WSAStartup(MAKEWORD(2,2), &WSAData());
}

SSocks::Utility::Winsock::~Winsock() {
  WSACleanup();
}


/////////////////////////TSOCK/////////////////////////

//generate socket and check for errors
SSocks::Utility::TSock::TSock(int type, int proto) : sock(::socket(AF_INET, type, proto)) {
  if(sock == SOCKET_ERROR) { throw std::runtime_error(lastErrStr(WSAGetLastError())); }
}

//release the resource if its owned by this object
SSocks::Utility::TSock::~TSock() {
  if(sock != SOCKET_ERROR) {
    closesocket(sock);
  }
}

//stop owning the resource and return it to the caller
int SSocks::Utility::TSock::validate() {
  int temp = sock;
  //disown the resource so that it won't be released when the destructor executes
  sock = SOCKET_ERROR;
  //return the handle
  return temp;
}

