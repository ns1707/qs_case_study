#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include <communication_builder.hpp>
#include <iCommunication.hpp>
#include <comm_types.hpp>


std::string        server_address = "127.0.0.1";
std::uint16_t      server_port    = 9000U;
std::uint32_t      timeoutMs      = 2000U;
comm::ProtocolType protocol       = comm::ProtocolType::udp;

int main() {

    std::cout << "Starting communication example..." << std::endl;
    using namespace comm;

    std::unique_ptr<ICommunication> comm =
        CommunicationBuilder()
            .withProtocol(protocol)
            .withIp(server_address)
            .withPort(server_port)
            .withTimeout(timeoutMs)
            .build();

    OperationResult result = comm->initialize();
    if (result.error != CommunicationError::none) {
        std::cout << "initialize failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "initialize succeeded" << std::endl;
    }

    result = comm->connect();
    if (result.error != CommunicationError::none) {
        std::cout << "connect failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "connect succeeded" << std::endl;
    }

    // Send Blocking: serialize data into a uint8_t buffer
    const Payload<std::string> payload {PayloadType::data, "client says hello \n"};

    const auto buffer = payload.serialize();
    result = comm->send(buffer.data(), buffer.size(), comm::SendMode::blocking);
    if (result.error != CommunicationError::none) {
        std::cout << "send failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "send succeeded" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // send non-blocking with delay
    const Payload<std::string> payload2 {PayloadType::data, "Non Blocking Hello \n"};

    const auto buffer2 = payload2.serialize();
    // Send: serialize data into a uint8_t buffer
    result = comm->send(buffer2.data(), buffer2.size(), comm::SendMode::nonBlocking, 2000U);
    if (result.error != CommunicationError::none) {
        std::cout << "send non-blocking failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "send non-blocking succeeded" << std::endl;
    }

    // send periodic with delay
    const Payload<std::string> payload3 {PayloadType::data, "Periodic Hello \n"};

    const auto buffer3 = payload3.serialize();
    // Send: serialize data into a uint8_t buffer
    result = comm->send(buffer3.data(), buffer3.size(), comm::SendMode::periodic, 2000U);
    if (result.error != CommunicationError::none) {
        std::cout << "send periodic failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "send periodic succeeded" << std::endl;
    }



    result = comm->close();
    if (result.error != CommunicationError::none) {
        std::cout << "close failed: " << static_cast<int>(result.error) << std::endl;
        std::cout << "output = " << static_cast<int>(result.output) << std::endl;
        return 1;
    }
    else
    {
        std::cout << "close succeeded" << std::endl;
    }


    // --- Test: non-blocking send should fail on unconnected socket ---
    std::cout << "\n=== Test: non-blocking send on unconnected socket ===" << std::endl;
    std::unique_ptr<ICommunication> commFail =
        CommunicationBuilder()
            .withProtocol(protocol)
            .withIp(server_address)
            .withPort(server_port)
            .withTimeout(timeoutMs)
            .build();

    result = commFail->initialize();
    if (result.error != CommunicationError::none) {
        std::cout << "initialize failed: " << static_cast<int>(result.error) << std::endl;
        return 1;
    }

    // Skip connect() — send will fail with EDESTADDRREQ
    const Payload<std::string> failPayload {PayloadType::data, "this should fail"};
    const auto failBuffer = failPayload.serialize();
    result = commFail->send(failBuffer.data(), failBuffer.size(), comm::SendMode::nonBlocking);
    if (result.error != CommunicationError::none) {
        std::cout << "PASS: non-blocking send failed as expected, error="
                  << static_cast<int>(result.error) << std::endl;
    }
    else
    {
        std::cout << "FAIL: non-blocking send should have failed" << std::endl;
    }
    commFail->close();
    return 0;
}
