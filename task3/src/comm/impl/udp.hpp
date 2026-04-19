#ifndef SRC_COMM_UDP_HPP
#define SRC_COMM_UDP_HPP

#include <atomic>
#include <mutex>
#include <thread>

#include <communication_base.hpp>
#include <iCommunication.hpp>
#include <send_scheduler.hpp>

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

  // Get the result of the last asynchronous send.
  OperationResult lastSendResult() const override;

  // Stop the periodic sending of data.
  void stopPeriodicSend();
private:

  struct SendAdapter {
    Udp& owner;
    OperationResult send(const std::uint8_t* data, std::size_t size) {
      return owner.sendImmediate(data, size);
    }
    OperationResult sendNonBlocking(const std::uint8_t* data, std::size_t size) {
      return owner.sendNonBlockingImmediate(data, size);
    }
  };

  // Send data over the communication channel without blocking.
  OperationResult sendImmediate(const std::uint8_t* data, std::size_t size);

  // Send data over the communication channel with a delay via the scheduler.
  OperationResult sendNonBlocking(const std::uint8_t* data, std::size_t size, const std::uint32_t delayMs = 0U);

  // Send data over the socket using MSG_DONTWAIT (truly non-blocking).
  OperationResult sendNonBlockingImmediate(const std::uint8_t* data, std::size_t size);

  // Map errno from ::send() to an OperationResult.
  static OperationResult mapSendError(int errnoValue);

  // Start periodic sending of data over the communication channel via the scheduler.
  OperationResult startPeriodicSend(const std::uint8_t* data,
                                    std::size_t         size,
                                    std::uint32_t       intervalInMs);

  SendAdapter m_sendAdapter;
  SendScheduler<SendAdapter> m_scheduler;
  SendScheduler<SendAdapter>::TaskId m_periodicTaskId{0};
  std::mutex m_sendMutex;
};

} // namespace comm

#endif // SRC_COMM_UDP_HPP
