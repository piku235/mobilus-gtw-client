# 📙 C++ Client Library for Mobilus Cosmo GTW

[![CI](https://github.com/piku235/mobilus-gtw-client/actions/workflows/continuous-integration.yml/badge.svg)](https://github.com/piku235/mobilus-gtw-client/actions/workflows/continuous-integration.yml)

A lightweight, platform-agnostic C++ client library for interacting with the **Mobilus Cosmo GTW**.
Designed to facilitate communication, control, and event subscription of Mobilus devices connected to the Cosmo GTW.

## Overview

This library provides a clean C++ API to interface with the Mobilus Cosmo GTW device over MQTT, utilizing Google Protocol Buffers (protobuf) for message serialization.

Use this client to:

- Integrate Mobilus devices into custom C++ applications or frameworks  
- Send commands and queries to devices
- Subscribe to device events for real-time updates

## Requirements

- C++17 compatible compiler (GCC 8+, Clang 7+)
- Mobilus Cosmo GTW firmware version ≥ 0.1.7.8
- OpenSSL v3
- Mosquitto v2.x (v1.x optionally supported)
- Google Protocol Buffers (protobuf) v2.6.1 (included)

## Build

This project uses **CMake** as its build system.

To build the project:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## mobcli

A command-line client for interacting with the Mobilus Cosmo GTW and it's also a good usage example of this library.
See the [mobcli repository](https://github.com/piku235/mobcli) for more details.

## Basic usage

```cpp
#include <jungi/mobilus_gtw_client/MqttMobilusGtwClient.h>
#include <jungi/mobilus_gtw_client/proto/DevicesListRequest.pb.h>
#include <jungi/mobilus_gtw_client/proto/DevicesListResponse.pb.h>

using namespace jungi::mobilus_gtw_client;

int main() {
    auto client = MqttMobilusGtwClient::builder()
        .dsn(MqttDsn::from("mqtts://192.168.1.1:8883?cacert=ca.cert").value())
        .login({ "admin", "admin" })
        .build();

    // connect to MQTT broker and authenticate with Cosmo GTW
    if (auto e = client.connect(); !e) {
        // print, log or handle error
        return 1;
    }

    // sends the request and waits for the response
    proto::DevicesListResponse response;
    if (auto e = client.sendRequest(proto::DevicesListRequest(), response); !e) {
        // print, log or handle error
        return 1;
    }

    // play with response

    return 0;
}
```
