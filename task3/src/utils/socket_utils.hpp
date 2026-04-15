#ifndef SRC_UTILS_SOCKET_UTILS_HPP
#define SRC_UTILS_SOCKET_UTILS_HPP

#include <cstring>
#include <string>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <comm_types.hpp>

namespace comm {
namespace utils {

class SocketUtils {
public:
  static bool isValidPort(std::uint16_t port)
  {
    return port != 0;
  }

  static bool isValidIp(const std::string& ip)
  {
    sockaddr_in sa;
    std::memset(&sa, 0, sizeof(sa));
    return ::inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
  }

  static OperationResult closeSocket(int& fd) {
    OperationResult result {OutputType::ok, CommunicationError::none};
    if (fd < 0) {
      // "Socket already closed" is not treated as an error in this context.
    }
    else if (::shutdown(fd, SHUT_RDWR) != 0) {
      result = OperationResult{OutputType::error,
                               CommunicationError::closeFailed};
    }
    else if (::close(fd) != 0) {
      result = OperationResult{OutputType::error,
                               CommunicationError::closeFailed};
    }
    else {
      fd = -1; // Mark as closed
    }
    return result;
  }

  static bool setReceiveTimeout(int fd, int timeoutMs) {
    bool isSuccess = true;
    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
      isSuccess = false;
    }

    return isSuccess;
  }

  static bool setSendTimeout(int fd, int timeoutMs) {
    bool isSuccess = true;
    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    if (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
      isSuccess = false;
    }

    return isSuccess;
  }
};

} // namespace utils
} // namespace comm

#endif // SRC_UTILS_SOCKET_UTILS_HPP