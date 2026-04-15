#ifndef SRC_COMMUNICATION_BUILDER_HPP
#define SRC_COMMUNICATION_BUILDER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <comm_types.hpp>
#include <iCommunication.hpp>

namespace comm{

// brief: Builder class for constructing ICommunication instances with a fluent interface.
class CommunicationBuilder
{
public:
  // constructor with default values
  CommunicationBuilder();

  // brief: Set the communication protocol (e.g., TCP, UDP).
  CommunicationBuilder& withProtocol(ProtocolType protocol);

  // brief: Set the IP address for the communication.
  CommunicationBuilder& withIp(const std::string& ip);

  // brief: Set the port number for the communication.
  CommunicationBuilder& withPort(std::uint16_t port);

  // brief: Set the timeout for the communication in milliseconds.
  CommunicationBuilder& withTimeout(std::uint32_t timeoutMs);

  // brief: Build and return the configured ICommunication instance.
  std::unique_ptr<ICommunication> build() const;

private:
  ProtocolType  m_protocol;    // protocol type (TCP, UDP, etc.)
  std::string   m_ip;          // IP address for communication
  std::uint16_t m_port;        // port number for communication
  std::uint32_t m_timeoutMs;   // timeout in milliseconds
  bool          m_hasProtocol; // flag to indicate if protocol has been set
};

} // namespace comm

#endif // SRC_COMMUNICATION_BUILDER_HPP
