/** @file */
#pragma once
#include <vector>

namespace SSocks {

  /**
   * A time value that cane be passed to make SSocks::select() wait forever.
   */
  extern const float SELECT_FOREVER;

  /**
   * @fn template<class T> std::vector<T*> select(const std::vector<T*>& sockets, float timeoutSeconds = SELECT_FOREVER)
   * A function to determine which sockets have incoming information waiting.
   * The SSocks::select() function accepts a vector of pointers to sockets of a uniform type (that is,
   * all of them are the same type of socket, whether TCPSocket, TCPServer, or UDPSocket). It returns 
   * a vector of the same type which contains only the members of the input vector which were ready to
   * read or accept from. The use of this function is to wait on a group of sockets until one or more
   * of them has incoming information to be processed. This function blocks for the amount of time
   * specified by 'timeoutSeconds' but will return immediately as soon as one or more sockets indicates
   * that it is ready.
   * 
   * @param sockets A vector of pointers to the sockets you want to check
   * @param timeoutSeconds The maximum number of seconds to wait before giving up.
   * @return A containing only those sockets from the input vector which were ready to read or accept from.
   */
  template<class T>
  std::vector<T*> select(const std::vector<T*>& sockets, float timeoutSeconds = SELECT_FOREVER);

}
