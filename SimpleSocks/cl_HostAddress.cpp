#include "cl_HostAddress.h"
#include "ns_Utility.h"
#include <WS2tcpip.h>

////////////////////////////NSLOOKUP////////////////////////////

//this 'anonymous namespace' restricts the scope of its contents to the current file
namespace {
  //this struct provides exception safety by calling freeaddrinfo()
  struct GAI_RAII {
    ~GAI_RAII() { if(root) { freeaddrinfo(root); } }
    addrinfo* root = nullptr;
  };
}

std::vector<SSocks::HostAddress> SSocks::nsLookup(const std::string& hostName, uint16_t port) {
  //winsock must be loaded for getaddrinfo() to work
  Utility::Winsock wsa;
  
  //getaddrinfo will return a linked list of addresses.
  //this pointer will indicate the root node
  GAI_RAII data;

  //The hints structure restricts the address search based on what
  //values are set in it. In this case I only want internet-type addresses.
  addrinfo hints = { 0 };
  hints.ai_family = AF_INET;

  //In certain cases the name server may be busy updating records momentarily
  //and getaddrinfo will signal WSATRY_AGAIN. We'll retry a couple times if that
  //happens.
  int attemptsRemaining = 3;
  while(1) {
    //attempt to fetch the address list into 'data.root'
    int result = getaddrinfo(hostName.c_str(), nullptr, &hints, &data.root);

    if(result == 0) { break; } //success

    //If we got a WSATRY_AGAIN and we have some retries left then...
    if((WSAGetLastError() == WSATRY_AGAIN) && (attemptsRemaining > 0)) {
      attemptsRemaining--;

      const DWORD QUARTER_SECOND_IN_MS = 250;
      Sleep(QUARTER_SECOND_IN_MS);

      continue;
    }

    //otherwise the request failed
    throw std::runtime_error("getaddrinfo() failed.");
  }

  //We'll copy the results into a vector and then return it.
  std::vector<HostAddress> vec;

  //last node points to nullptr as 'next'
  for(addrinfo* node = data.root; node; node = node->ai_next) {
    //set the port number
    reinterpret_cast<sockaddr_in*>(node->ai_addr)->sin_port = htons(port);
    //copy the address into the vector
    vec.emplace_back(node->ai_addr);
  }

  return vec;
}

////////////////////////////HOSTADDRESS////////////////////////////

//Initializer list here allocates the sockaddr_in and initializes it by filling it with zeroes.
SSocks::HostAddress::HostAddress(const std::string& address, uint16_t port) : buffer{0} {
  //we need to set the address family and port number
  sainp()->sin_family = AF_INET; //AF_INET is for IPv4 using the TCP stack
  sainp()->sin_port = htons(port); //convert the port to network order before storing it

  //'address' should be a dot-quad form IPv4 address. inet_pton will convert it to a network
  //address value and write it to the corerct position in the sockaddr_in.
  int result = inet_pton(AF_INET, address.c_str(), &sainp()->sin_addr);
  if(result == 0) { throw std::runtime_error("Invalid address string supplied to HostAddress."); }
  if(result == -1) { throw std::runtime_error(Utility::lastErrStr(WSAGetLastError())); }

}

//copy the pointed sockaddr_in into our newly allocated one
SSocks::HostAddress::HostAddress(const sockaddr_in* sainp) {
  std::copy(reinterpret_cast<const uint8_t*>(sainp), reinterpret_cast<const uint8_t*>(sainp + 1), buffer.begin());
}

//delegate to the sockaddr_in* constructor
SSocks::HostAddress::HostAddress(const sockaddr* sap) : HostAddress(reinterpret_cast<const sockaddr_in*>(sap)) {
  //nothing
}

std::string SSocks::HostAddress::getAddr() const {
  //An IPv4 address can be up to 15 characters long. (xxx.xxx.xxx.xxx)
  //inet_ntop() is a C-ish function, so we need an additional char for
  //the null terminator.
  const size_t BUFFER_LEN = 16;
  char str[BUFFER_LEN];

  auto addr = *sainp();

  //translate the address in sain to a C string and store it in 'str'
  inet_ntop(AF_INET, &addr.sin_addr, str, BUFFER_LEN);
  
  //return as std::string (using implicit conversion from char*)
  return str;
}

uint16_t SSocks::HostAddress::getPort() const {
  //translate from network to host byte order
  return ntohs(sainp()->sin_port);
}

void SSocks::HostAddress::setPort(uint16_t port) {
  //translate from host to network byte order
  sainp()->sin_port = htons(port);
}

SSocks::HostAddress::operator sockaddr*() {
  return reinterpret_cast<sockaddr*>(buffer.data());
}

SSocks::HostAddress::operator const sockaddr*() const {
  return reinterpret_cast<const sockaddr*>(buffer.data());
}

SSocks::HostAddress::operator sockaddr_in*() {
  return sainp();
}

SSocks::HostAddress::operator const sockaddr_in*() const {
  return sainp();
}

size_t SSocks::HostAddress::size() const {
  return buffer.size();
}

sockaddr_in* SSocks::HostAddress::sainp() {
  return reinterpret_cast<sockaddr_in*>(buffer.data());
}

const sockaddr_in* SSocks::HostAddress::sainp() const {
  return reinterpret_cast<const sockaddr_in*>(buffer.data());
}


