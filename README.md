# Case Study - Quantum Systems

## task 1 — Fix dangling reference bug in threads
A cpp file where multiple threads are spawned, the thread cb functions
are given as lambdas, which blanket captures all references in its current stack.

Also, there is a logical bug with seconds and millisecond mismatch.

### Project Structure
```
task1/
├── task_thread.hpp

```

## task 2  — Matrix 90 deg rotation issue.
A python script executing an inplace matrix rotation.
The bug was multiple rotations of already rotated elements.
The issue is fixed with a layered 4-way swap technique.

### Project Structure
```
task2/
├── matrix_rotate_90.py

```

## task 3 — C++ Communication Library
A class for UDP communication, supporting blocking, non-blocking, and periodic send modes.

### Prerequisites
**Linux only** — The library is developed for a linux system. Due to the usage of POSIX sockets (`sys/socket.h`, `arpa/inet.h`)
and it is not portable to windows.

### Project Structure

```
task3/
├── src/
│   ├── main.cpp                          # Example usage and test cases
│   ├── comm/
│   │   ├── interface/
│   │   │   ├── comm_types.hpp            # Enums, Payload<T>, OperationResult
│   │   │   ├── iCommunication.hpp        # ICommunication interface
│   │   │   └── communication_builder.hpp # Builder API
│   │   └── impl/
│   │       ├── communication_base.cpp/hpp  # Shared base (socket, config, validation)
│   │       ├── communication_builder.cpp   # Builder implementation
│   │       ├── send_scheduler.hpp          # Generic send scheduler (delayed & periodic)
│   │       └── udp.cpp/hpp                 # UDP implementation
│   ├── utils/
│   │   └── socket_utils.hpp              # Socket helpers (close, timeout, validation)
│   └── external/                         # Reserved for third-party dependencies
├── doc/
│   └── communication_builder.puml        # PlantUML class diagram

```

### Building

Requires CMake 3.20+ and a C++17 compiler.

```bash
cd task3/
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

This produces the `comm_task3` executable.

### Running

Start a UDP listener in one terminal, then run the application in another:

```bash
# Terminal 1 — listener
nc -u -l -p 9000

# Terminal 2 — application
./build/comm_task3
```

### Design
The class diagram is available at [`doc/communication_builder.puml`](doc/communication_builder.puml).
