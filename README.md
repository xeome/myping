# myping

## Table of Contents

- [myping](#myping)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
  - [Installation](#installation)
    - [Dependencies](#dependencies)
    - [Building](#building)
  - [Usage](#usage)
  - [License](#license)

## Introduction

This project is a partial reimplementation of the POSIX `ping` command in C++. It is designed primarily for educational purposes, allowing developers to understand the inner workings of network communication and debugging techniques in a Linux environment.

It uses raw sockets and builds ICMP packets from scratch including the ethernet, IP, and ICMP headers. Since raw sockets require root privileges, the program must be run as root or with the `CAP_NET_RAW` capability.

## Installation

### Dependencies

- **C++ Compiler**: A C++ compiler that supports C++11 or later.
- **CMake**: For building the project.

### Building

1. Clone the repository:

```shell
git clone https://github.com/xeome/myping
cd myping
cmake -B build
cmake --build build
```

By default, the executable will be built in the `build` directory.

## Usage

To use the reimplementation, simply run the `ping` command followed by the target host (do not forget to run the program as root or with the `CAP_NET_RAW` capability).

```shell
build/ping -t <target_host> -d <egress_interface>
```

For example, to ping `192.168.1.1`, you would run:

```shell
build/ping -t 192.168.1.1 -d eth0
```

## License

This project is licensed under the GPL License. See the [LICENSE](LICENSE) file for details.
