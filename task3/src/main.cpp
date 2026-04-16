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
        std::cout << "send periodic requested successfully" << std::endl;
    }

    // Let periodic sends run; poll for failures
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        OperationResult periodicResult = comm->lastSendResult();
        if (periodicResult.error != CommunicationError::none)
        {
            std::cout << "periodic send failed mid-flight: error="
                      << static_cast<int>(periodicResult.error) << std::endl;
            break;
        }
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


    // =========================================================================
    // Edge-case tests
    // =========================================================================
    int passed = 0;
    int failed = 0;

    auto check = [&](const char* name, bool condition) {
        if (condition) { ++passed; std::cout << "  PASS: " << name << std::endl; }
        else           { ++failed; std::cout << "  FAIL: " << name << std::endl; }
    };

    auto makeComm = [&](){
        return CommunicationBuilder()
            .withProtocol(protocol)
            .withIp(server_address)
            .withPort(server_port)
            .withTimeout(timeoutMs)
            .build();
    };

    const Payload<std::string> testPayload {PayloadType::data, "edge-case test\n"};
    const auto testBuf = testPayload.serialize();

    // --- 1. Blocking send edge cases ---
    std::cout << "\n=== Blocking send edge cases ===" << std::endl;
    {
        // 1a. send on uninitialized socket
        auto c = makeComm();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::blocking);
        check("blocking on uninitialized → notInitialized",
              result.error == CommunicationError::notInitialized);
    }
    {
        // 1b. send on initialized but unconnected socket (blocking)
        auto c = makeComm(); c->initialize();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::blocking);
        check("blocking on unconnected → send error",
              result.error != CommunicationError::none);
        c->close();
    }

    // --- 2. Non-blocking send edge cases ---
    {
        // 2a. delayMs = 999 (just below min) → delayError
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::nonBlocking, 999U);
        check("nonBlocking delayMs=999 → delayError",
              result.error == CommunicationError::delayError);
        c->close();
    }
    {
        // 2b. delayMs = 1000 (lower boundary) → ok
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::nonBlocking, 1000U);
        check("nonBlocking delayMs=1000 → ok",
              result.error == CommunicationError::none);
        c->close();
    }
    {
        // 2c. delayMs = 255000 (upper boundary) → ok
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::nonBlocking, 255000U);
        check("nonBlocking delayMs=255000 → ok",
              result.error == CommunicationError::none);
        c->close();
    }
    {
        // 2d. delayMs = 255001 (above max) → delayError
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::nonBlocking, 255001U);
        check("nonBlocking delayMs=255001 → delayError",
              result.error == CommunicationError::delayError);
        c->close();
    }
    {
        // 2e. nullptr payload
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(nullptr, 10, SendMode::nonBlocking, 2000U);
        check("nonBlocking nullptr payload → sendPayloadEmpty",
              result.error == CommunicationError::sendPayloadEmpty);
        c->close();
    }

    // --- 3. Periodic send edge cases ---
    std::cout << "\n=== Periodic send edge cases ===" << std::endl;
    {
        // 3a. delayMs = 999 (just below min) → delayError
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::periodic, 999U);
        check("periodic delayMs=999 → delayError",
              result.error == CommunicationError::delayError);
        c->close();
    }
    {
        // 3b. delayMs = 1000 (lower boundary) → ok
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::periodic, 1000U);
        check("periodic delayMs=1000 → ok",
              result.error == CommunicationError::none);
        c->close();
    }
    {
        // 3c. delayMs = 255000 (upper boundary) → ok
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::periodic, 255000U);
        check("periodic delayMs=255000 → ok",
              result.error == CommunicationError::none);
        c->close();
    }
    {
        // 3d. delayMs = 255001 (above max) → delayError
        auto c = makeComm(); c->initialize(); c->connect();
        result = c->send(testBuf.data(), testBuf.size(), SendMode::periodic, 255001U);
        check("periodic delayMs=255001 → delayError",
              result.error == CommunicationError::delayError);
        c->close();
    }

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===" << std::endl;

    return 0;
}
