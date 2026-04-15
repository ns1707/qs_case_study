#ifndef SRC_COMM_UDP_HPP
#define SRC_COMM_UDP_HPP

#include <atomic>
#include <thread>

#include <communication_base.hpp>
#include <iCommunication.hpp>

namespace comm {

// brief: UDP communication implementation. Inherits from CommunicationBase and implements ICommunication interface.
class Udp : public CommunicationBase 
{
public:
  // Constructor with IP, port, and timeout parameters.
  explicit Udp(const std::string& ip,
               std::uint16_t      port,
               std::uint32_t      timeoutMs);

  // Destructor to clean up resources.
  ~Udp();

  // Initialize the UDP communication channel (e.g., create socket, bind, etc.).
  OperationResult initialize() override;

  // Establish a connection if applicable (e.g., for TCP/UDP).
  OperationResult connect() override;

  // Send data over the communication channel.
  OperationResult send(const std::uint8_t* data,
                       std::size_t size,
                       const comm::SendMode mode = comm::SendMode::blocking,
                       const std::uint32_t delayMs = 0U) override;

  // Receive data from the communication channel.
  OperationResult receive(std::uint8_t* buffer, std::size_t bufferSize, std::size_t& bytesReceived) override;

  // Stop the periodic sending of data.
  void stopPeriodicSend();
private:

  // setting a constant max count for periodic send to prevent infinite sending in this example. In production, this could be made configurable or use a different mechanism to control the sending loop.
  static constexpr std::uint32_t MAX_PERIODIC_SEND_COUNT = 20U;

  // Send data over the communication channel without blocking.
  OperationResult sendImmediate(const std::uint8_t* data, std::size_t size);

  // Send data over the communication channel without blocking.
  OperationResult sendNonBlocking(const std::uint8_t* data, std::size_t size, const std::uint32_t delayMs = 0U);

  // Map errno from ::send() to an OperationResult.
  static OperationResult mapSendError(int errnoValue);

  // Start periodic sending of data over the communication channel.
  OperationResult startPeriodicSend(const std::uint8_t* data,
                                    std::size_t         size,
                                    std::uint32_t       intervalSeconds);

  std::atomic<bool> m_stopRequested{false}; // Flag to signal the periodic send thread to stop
  std::thread       m_periodicThread;       // periodic send thread
  bool              m_isConfigValidated{false};  // Flag to indicate if the configuration has been validated
  OperationResult   m_lastPeriodicSendResult{OutputType::ok, CommunicationError::none}; // Store the last result of periodic send attempts
};

} // namespace comm

#endif // SRC_COMM_UDP_HPP