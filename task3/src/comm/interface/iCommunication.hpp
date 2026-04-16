#ifndef SRC_COMM_ICOMM_HPP
#define SRC_COMM_ICOMM_HPP

#include <comm_types.hpp>
#include <cstdint>

namespace comm {
// brief: Interface for communication operations. Defines the contract for all communication types (e.g., TCP, UDP).
class ICommunication
{
public:
  // Destructor is virtual to allow proper cleanup of derived classes.
  virtual ~ICommunication() {}

  // Initialize the communication channel (e.g., create socket, bind, etc.).
  virtual OperationResult initialize() = 0;

  // Establish a connection if applicable (e.g., for TCP/UDP).
  virtual OperationResult connect() = 0;

  // Send data over the communication channel.
  virtual OperationResult send(const std::uint8_t* data, 
                               std::size_t size,
                               const comm::SendMode mode = comm::SendMode::blocking,
                               const std::uint32_t delayMs = 0U) = 0;

  // Receive data from the communication channel.
  virtual OperationResult receive(std::uint8_t* buffer, std::size_t bufferSize, std::size_t& bytesReceived) = 0;

  // Close the communication channel.
  virtual OperationResult close() = 0;

  // Get the result of the last asynchronous send (non-blocking/periodic).
  virtual OperationResult lastSendResult() const = 0;

  // Get the timeout value of the communication channel.
  virtual std::uint32_t timeout() const = 0;
};

} // namespace comm

#endif // SRC_COMM_ICOMM_HPP
