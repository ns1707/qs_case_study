#include <thread>
#include <chrono>

#include <communication_base.hpp>
#include <comm_types.hpp>
#include <socket_utils.hpp>

comm::CommunicationBase::CommunicationBase(const std::string& ip,
                                           std::uint16_t      port,
                                           std::uint32_t      timeoutMs)
    : m_ip(ip),
      m_port(port),
      m_initialized(false),
      m_socketFd(-1),
      m_timeoutMs(timeoutMs)
{
}

std::uint32_t comm::CommunicationBase::timeout() const
{
  return m_timeoutMs;
}

comm::OperationResult comm::CommunicationBase::close()
{
  return comm::utils::SocketUtils::closeSocket(m_socketFd);
}

void comm::CommunicationBase::applyDelay(const uint32_t delayMs) const
{
  std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
}

bool comm::CommunicationBase::isInitialized() const
{
  return m_initialized;
}

int& comm::CommunicationBase::socketFdRef()
{
  return m_socketFd;
}

void comm::CommunicationBase::markInitialized(bool value)
{
  m_initialized = value;
}

sockaddr_in comm::CommunicationBase::makeAddress() const
{
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  ::inet_pton(AF_INET, m_ip.c_str(), &addr.sin_addr);
  return addr;
}

comm::OperationResult comm::CommunicationBase::validateConfiguration() const
{
  comm::OperationResult result {comm::OutputType::ok, comm::CommunicationError::none};
  if (!comm::utils::SocketUtils::isValidIp(m_ip))
  {
    result = comm::OperationResult{comm::OutputType::error,
                                   comm::CommunicationError::invalidIp};
  }
  else if (!comm::utils::SocketUtils::isValidPort(m_port))
  {
    result = comm::OperationResult{comm::OutputType::error,
                                   comm::CommunicationError::invalidPort};
  }

  return result;
}

bool comm::CommunicationBase::isValidPort(std::uint16_t port)
{
  return port != 0;
}

// Minimal validation for example purposes.
// Replace with inet_pton-based validation in production.
bool comm::CommunicationBase::isValidIp(const std::string& ip)
{
  return !ip.empty();
}
