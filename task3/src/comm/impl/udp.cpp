#include <chrono>
#include <cerrno>
#include <cstring>
#include <vector>

#include <udp.hpp>
#include <comm_types.hpp>
#include <socket_utils.hpp>

namespace comm {

Udp::Udp(const std::string& ip,
         std::uint16_t      port,
         std::uint32_t      timeoutMs)
    : CommunicationBase(ip, port, timeoutMs),
      m_sendAdapter{*this},
      m_scheduler(m_sendAdapter)
{
}

OperationResult Udp::initialize()
{

  OperationResult result = OperationResult{OutputType::ok, CommunicationError::none};
  if (isInitialized())
  {
    result = OperationResult{OutputType::error,
                             CommunicationError::alreadyInitialized};
  }
  else
  {
    result = validateConfiguration();
    if (result.error == CommunicationError::none)
    {
      int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
      {
        result = OperationResult{OutputType::error,
                                 CommunicationError::socketCreationFailed};
      }
      else
      {
        if (!comm::utils::SocketUtils::setReceiveTimeout(fd, timeout()))
        {
          result = OperationResult{OutputType::error,
                                   CommunicationError::socketOptionFailed};
        }
        else if (!comm::utils::SocketUtils::setSendTimeout(fd, timeout()))
        {
          result = OperationResult{OutputType::error,
                                   CommunicationError::socketOptionFailed};
        }
        else
        {
          socketFdRef() = fd;
          markInitialized(true);
          m_scheduler.start();
        }
      }
    }
  }
  return result;
}

OperationResult Udp::connect()
{
  // UDP is connectionless in the general case.
  // We expose it for API symmetry.
  OperationResult result {OutputType::ok, CommunicationError::none};
  if (!isInitialized())
  {
    result = OperationResult{OutputType::error,
                             CommunicationError::notInitialized};
  }
  else
  {
    sockaddr_in addr = makeAddress();
    if (::connect(socketFdRef(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
      result = OperationResult{OutputType::error,
                               CommunicationError::connectionFailed};
    }
  }

  return result;
}

OperationResult Udp::send(const std::uint8_t* data, std::size_t size, const comm::SendMode mode, const std::uint32_t delayMs)
{
  OperationResult result {OutputType::ok, CommunicationError::none};
  if (!isInitialized())
  {
     result = OperationResult{OutputType::error,
                              CommunicationError::notInitialized};
  }
  else if (data == nullptr || size == 0U)
  {
    result = OperationResult{OutputType::error,
                             CommunicationError::sendPayloadEmpty};
  }
  else
  {
    switch (mode)
    {
      case comm::SendMode::blocking:
        result = sendImmediate(data, size);
        break;
      case comm::SendMode::nonBlocking:
        if ((delayMs < 1000U) || (delayMs > 255000U)) // Boundary check for delayMs
        {
          result = OperationResult{OutputType::error,
                                   CommunicationError::delayError};
        }
        else
        {
          result = sendNonBlocking(data, size, delayMs);
        }
        break;
      case comm::SendMode::periodic:
        if ((delayMs < 1000U) || (delayMs > 255000U)) // Boundary check for delayMs
        {
          result = OperationResult{OutputType::error,
                                   CommunicationError::delayError};
        }
        else
        {
          result = startPeriodicSend(data, size, delayMs);
        }
        break;
      case comm::SendMode::timed:
        result = sendNonBlocking(data, size, delayMs);
        break;
      default:
        result = OperationResult{OutputType::error,
                                 CommunicationError::unsupportedOperation};
        break;
    }
  }
  
  return result;
}

OperationResult Udp::receive(std::uint8_t* buffer, std::size_t bufferSize, std::size_t& bytesReceived)
{
  static_cast<void>(buffer);        // buffer is currently unused, but may be used in future enhancements for storing received data
  static_cast<void>(bufferSize);    // bufferSize is currently unused, but may be used in future enhancements for buffer overflow protection
  static_cast<void>(bytesReceived); // bytesReceived is currently unused, but may be used in future enhancements for tracking received data

  // TODO: Implement receive logic. For now, we return a default result.
  OperationResult result {OutputType::ok, CommunicationError::none};
  return result;
}

Udp::~Udp()
{
  stopPeriodicSend();
  m_scheduler.stop();
}

comm::OperationResult Udp::lastSendResult() const
{
  return m_scheduler.lastResult();
}

comm::OperationResult Udp::sendImmediate(const std::uint8_t* data, std::size_t size)
{
  std::lock_guard<std::mutex> lock(m_sendMutex);
  OperationResult result {OutputType::ok, CommunicationError::none};

  const ssize_t bytesSent = ::send(socketFdRef(), data, size, 0);
  if (bytesSent < 0)
  {
    result = mapSendError(errno);
  }
  else if (bytesSent != static_cast<ssize_t>(size))
  {
    // Partial send is treated as failure in this context.
    result = OperationResult{OutputType::error,
                              CommunicationError::partialSend};
  }

  return result;
}

comm::OperationResult Udp::sendNonBlocking(const std::uint8_t* data, std::size_t size, const std::uint32_t delayMs)
{
  if (data == nullptr || size == 0U)
  {
    return OperationResult{OutputType::error,
                           CommunicationError::sendPayloadEmpty};
  }
  return m_scheduler.sendWithDelay(data, size, delayMs);
}

OperationResult Udp::startPeriodicSend(const std::uint8_t* data,
                                       std::size_t         size,
                                       std::uint32_t       intervalInMs)
{
  if (data == nullptr || size == 0U)
  {
    return OperationResult{OutputType::error,
                           CommunicationError::sendPayloadEmpty};
  }
  if (intervalInMs == 0U)
  {
    return OperationResult{OutputType::error,
                           CommunicationError::invalidArgumentPeriodic};
  }
  return m_scheduler.sendPeriodically(data, size, 0, intervalInMs, m_periodicTaskId);
}

void Udp::stopPeriodicSend()
{
  if (m_periodicTaskId != 0)
  {
    m_scheduler.cancel(m_periodicTaskId);
    m_periodicTaskId = 0;
  }
}

OperationResult Udp::mapSendError(int errnoValue)
{
  OperationResult result {OutputType::error, CommunicationError::unknown};
  switch (errnoValue)
  {
    case EAGAIN:        // send buffer full (also covers EWOULDBLOCK)
      result = {OutputType::error, CommunicationError::sendBufferFull};
      break;
    case EMSGSIZE:      // datagram exceeds max size
      result = {OutputType::error, CommunicationError::sendMessageTooLarge};
      break;
    case EINTR:         // interrupted by signal
      result = {OutputType::error, CommunicationError::sendInterrupted};
      break;
    case ENOTCONN:      // socket not connected
      result = {OutputType::error, CommunicationError::sendNotConnected};
      break;
    case ECONNREFUSED:  // destination unreachable (ICMP port unreachable)
      result = {OutputType::error, CommunicationError::connectionFailed};
      break;
    default:
      result = {OutputType::error, CommunicationError::sendFailed};
      break;
  }
  return result;
}

} // namespace comm
