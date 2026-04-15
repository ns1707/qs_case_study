#ifndef COMM_TYPES_HPP
#define COMM_TYPES_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace comm {

// brief: Protocol types supported by the library.
enum class ProtocolType
{
  // tcp,
  udp,
  unspecified
};

enum class SendMode {
  blocking,
  nonBlocking,
  periodic,
  timed
};

enum class PayloadType
{
  data,
  command
};

enum class CommandType
{
  start,
  stop
};

template<typename T>
struct Payload
{
  PayloadType type;
  T           content;

  std::vector<std::uint8_t> serialize() const;
};

template<typename T>
std::vector<std::uint8_t> Payload<T>::serialize() const
{
  const auto* rawContent = reinterpret_cast<const std::uint8_t*>(&content);
  std::vector<std::uint8_t> buffer(sizeof(PayloadType) + sizeof(T));

  std::memcpy(buffer.data(), &type, sizeof(PayloadType));
  std::memcpy(buffer.data() + sizeof(PayloadType), rawContent, sizeof(T));
  return buffer;
}

template<>
inline std::vector<std::uint8_t> Payload<std::string>::serialize() const
{
  std::vector<std::uint8_t> buffer(sizeof(PayloadType) + sizeof(std::uint32_t) + content.size());
  std::memcpy(buffer.data(), &type, sizeof(PayloadType));

  std::uint32_t contentSize = static_cast<std::uint32_t>(content.size());
  std::memcpy(buffer.data() + sizeof(PayloadType), &contentSize, sizeof(std::uint32_t));
  std::memcpy(buffer.data() + sizeof(PayloadType) + sizeof(std::uint32_t), content.data(), content.size());
  return buffer;
}

// brief: High-level output types for operations.
// For detailed error information, see CommunicationError.
enum class OutputType
{
  ok,
  error,
  timeout
};

// brief: Communication error codes for detailed error reporting.
enum class CommunicationError
{
  none,
  invalidIp,
  invalidPort,
  socketCreationFailed,
  socketOptionFailed,
  bindFailed,
  connectionFailed,
  sendFailed,
  sendBufferFull,
  sendMessageTooLarge,
  sendInterrupted,
  sendNotConnected,
  partialSend,
  sendPayloadEmpty,
  receiveFailed,
  receiveBufferTooSmall,
  receivePayloadEmpty,
  closeFailed,
  notInitialized,
  alreadyInitialized,
  invalidConfiguration,
  unsupportedOperation,
  delayError,
  unknown
};

// brief:This is what the library returns for every operation.
struct OperationResult
{
  OutputType         output;
  CommunicationError error;
};

}

#endif // COMM_TYPES_HPP
