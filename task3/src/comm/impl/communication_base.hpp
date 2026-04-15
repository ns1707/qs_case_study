#ifndef SRC_COMM_COMM_BASE_HPP
#define SRC_COMM_COMM_BASE_HPP

#include <comm_types.hpp>
#include <iCommunication.hpp>
#include <socket_utils.hpp>

namespace comm {

// brief: Base class for common communication functionalities. Implements shared logic for TCP, UDP, etc.
class CommunicationBase : public ICommunication {
public:
    // Constructor with IP, port, and timeout parameters.
    CommunicationBase(const std::string& ip,
                      std::uint16_t      port,
                      std::uint32_t      timeoutMs);

    // Get the timeout value of the communication channel.
    std::uint32_t timeout() const override;

    // Close the communication channel.
    OperationResult close() override;

protected:
    // Apply a delay in milliseconds.
    void applyDelay(const uint32_t delayMs) const;

    // Get a reference to the socket file descriptor.
    int& socketFdRef();

    // Check if the communication channel is initialized.
    bool isInitialized() const;

    // Mark the communication channel as initialized or not.
    void markInitialized(bool value);

    // Create a sockaddr_in structure for the communication channel.
    sockaddr_in makeAddress() const;

    // Validate if the given port number is within the valid range.
    static bool isValidPort(std::uint16_t port);

    // Validate the current configuration (IP, port, etc.) before initialization.
    OperationResult validateConfiguration() const;

    // Minimal validation for example purposes.
    // Replace with inet_pton-based validation in production.
    static bool isValidIp(const std::string& ip);

private:
    std::string   m_ip;          // IP address for communication
    std::uint16_t m_port;        // port number for communication
    bool          m_initialized; // flag to indicate if the communication channel is initialized
    int           m_socketFd;    // socket file descriptor, -1 if not initialized
    uint32_t      m_timeoutMs;   // timeout in milliseconds for send/receive operations
};
}

#endif // SRC_COMM_COMM_BASE_HPP
