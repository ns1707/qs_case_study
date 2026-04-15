#include <communication_builder.hpp>
#include <udp.hpp>

namespace comm {

CommunicationBuilder::CommunicationBuilder()
    : m_protocol(ProtocolType::unspecified),
      m_ip("127.0.0.1"),
      m_port(0),
      m_hasProtocol(false)
{
}

CommunicationBuilder& CommunicationBuilder::withProtocol(ProtocolType protocol)
{
  m_protocol    = protocol;
  m_hasProtocol = true;
  return *this;
}

CommunicationBuilder& CommunicationBuilder::withIp(const std::string& ip)
{
  m_ip = ip;
  return *this;
}

CommunicationBuilder& CommunicationBuilder::withPort(std::uint16_t port)
{
  m_port = port;
  return *this;
}

CommunicationBuilder& CommunicationBuilder::withTimeout(std::uint32_t timeoutMs)
{
  m_timeoutMs = timeoutMs;
  return *this;
}

std::unique_ptr<ICommunication> CommunicationBuilder::build() const
{
  std::unique_ptr<ICommunication> commPtr;
  if (m_hasProtocol)
  {
    switch (m_protocol)
    {
      case ProtocolType::udp:
        commPtr = std::make_unique<Udp>(m_ip, m_port, m_timeoutMs); // Default timeout, can be extended to be configurable
        break;
      // Extend with other protocols as needed.
      default:
        // Handle unsupported protocol case if necessary.
        break;
    }
  }
  // Extend with other protocols as needed.
  return commPtr;
}

} // namespace comm
