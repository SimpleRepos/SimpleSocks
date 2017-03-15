/** @file */
#pragma once
#include <vector>
#include <string>
#include <array>

//Forward declarations so we don't have to leak the Winsock header into the user space
struct sockaddr;
struct sockaddr_in;

namespace SSocks {
  //forward declaration
  class HostAddress;

  /**
   * @fn std::vector<HostAddress> nsLookup(const std::string& hostName, uint16_t port = 0)
   * Look up a host via DNS and return a vector of HostAddress objects pointing to that host.
   * @param hostName Canonical name of host to look up, such as "google.com".
   * @param port Desired port to connect to later.
   * @return A vector of HostAddress objects matching the indeicated host.
   */
  std::vector<HostAddress> nsLookup(const std::string& hostName, uint16_t port = 0);

  /**
   * Class used to represent an IPv4 internet address and port number.
   * It has conversion functions that allow it to conver to and from sockaddr
   * and sockaddr_in. Note that SimpleSocks is presently IPv4 only, so this
   * class - especially - would need a rewrite to support IPv6.
   */
  class HostAddress {
  public:
    /**
     * Construct from user provided address string and port number.
     * @see nsLookup()
     * @param address A dot-quadded address string.\n
     * THIS CONSTRUCTOR WILL NOT LOOK UP A HOST.\n
     * The address string should be in the format "127.0.0.1", NOT "google.com".
     * @param port A port number.
     */
    HostAddress(const std::string& address, uint16_t port);

    //! convert from a sockaddr_in*
    HostAddress(const sockaddr_in* sainp);

    //! convert from a sockaddr*
    HostAddress(const sockaddr* sap);

    //! copy constructor
    HostAddress(const HostAddress&) = default;

    //! copy-assign
    HostAddress& operator=(const HostAddress&) = default;

    //! move constructor
    HostAddress(HostAddress&&) = default;

    //! move-assign
    HostAddress& operator=(HostAddress&&) = default;

    //! Return a string containing the dot-quad formatted address, such as "127.0.0.1".
    std::string getAddr() const;

    //! Return the currently stored port number
    uint16_t getPort() const;

    //! Change the stored port number
    void setPort(uint16_t port);

    //! Conversion to sockaddr* HostAddress can be used in its place.
    operator sockaddr*();
    //! Conversion to const sockaddr* HostAddress can be used in its place.
    operator const sockaddr*() const;
    //! Conversion to sockaddr_in* HostAddress can be used in its place.
    operator sockaddr_in*();
    //! Conversion to const sockaddr_in* HostAddress can be used in its place.
    operator const sockaddr_in*() const;

    //! Return size of address data.
    size_t size() const;

  private:
    static const size_t SIZEOF_SOCKADDR_IN = 16;
    std::array<uint8_t, SIZEOF_SOCKADDR_IN> buffer;

    sockaddr_in* sainp();
    const sockaddr_in* sainp() const;

  };

}

