# C++ Client Library for Mobilus Cosmo GTW

A lightweight, platform-agnostic C++ client library for interacting with the **Mobilus Cosmo GTW**.
Designed to facilitate communication, control, and event subscription of Mobilus devices connected to the Cosmo GTW.

## Overview

This library provides a clean C++ API to interface with the Mobilus Cosmo GTW device over MQTT, utilizing Google Protocol Buffers (protobuf) for message serialization.

Use this client to:

- Integrate Mobilus devices into custom C++ applications or frameworks  
- Send commands and queries to devices
- Subscribe to device events for real-time updates

## Requirements

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- Mobilus Cosmo GTW firmware version >= 0.1.7.8
- OpenSSL library
- Mosquitto library
- Google Protocol Buffers (protobuf) v2.6.1 (included)

## Limitations

- The client currently supports **single-threaded** use only.  
- Multi-threaded support and concurrency handling are planned for future releases.

## Build

This project uses **CMake** as its build system.

To build the project:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## mobcli

This library includes the **mobcli** tool — a command-line client for interacting with the Mobilus Cosmo GTW device.
See the [mobcli](tools/mobcli) for more details about the command-line client.

## Basic usage

```cpp
#include <jungi/mobilus_gtw_client/MqttMobilusGtwClient.h>
#include <jungi/mobilus_gtw_client/proto/DevicesListRequest.pb.h>
#include <jungi/mobilus_gtw_client/proto/DevicesListResponse.pb.h>

using namespace jungi::mobilus_gtw_client;

int main() {
    auto client = MqttMobilusGtwClient::with({
        "192.168.1.1", // host
        8883,          // port
        "admin",       // username
        "admin",       // password
        "/certs/mobilelabs_mobilus_ca.crt" // CA file
    });
    
     // connect to MQTT broker and authenticate with Cosmo GTW
    if (!client.connect()) {
        // handle connection failure
        return 1;
    }

    // sends the request and waits for the response
    proto::DevicesListResponse response;
    if (!client.sendRequest(proto::DevicesListRequest(), response)) {
        return 1;
    }

    // play with response

    return 0;
}
```
