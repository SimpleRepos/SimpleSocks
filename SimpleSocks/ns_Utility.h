/** @file */
#pragma once
#include <string>

//Users should not need to make use of these classes and functions directly.

namespace SSocks {
  namespace Utility {

    /**
     * @fn std::string lastErrStr()
     * Returns the string explanation of the last WSA error.
     * Users should not need to make use of this function directly.
     */
      std::string lastErrStr(int code);

    /**
     * RAII object for Winsock itself.
     * Users should not need to make use of these class directly.
     * Winsock must be activated before its functions will work, so an instance of this object
     * is included in the Socket objects and in the HostAddress lookup function. The constructor
     * calls WSAStartup() and the destructor calls WASCleanup(). MSDN specifies that the hidden
     * resource being managed this way will refrence count, being released  when the number of
     * cleanup calls equals the number of startup calls.
     */
    struct Winsock {
      //! Start Winsock
      Winsock();

      //! Clean up Winsock
      ~Winsock();
    };

    /**
     * Tentative socket connection.
     * Users should not need to make use of these class directly.
     * This is used to create a temporary RAII socket so that the primary classes can generate a
     * socket and take initial actions on it, then take ownership of the resource only after
     * those actions have succeeded.
     */
    class TSock {
    public:
      //! Generate a socket.
      TSock(int type, int proto);

      //! Release the socket if it's still owned by this object.
      ~TSock();

      //! Forfeit ownership and return the socket resource.
      int validate();

      //! Conversion to int allows the TSock object to be passed to socket functions.
      operator int() const { return sock; }

    private:
      int sock;

    };

  }

}
