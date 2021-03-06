# Socket

## Abstract

Written in modern C++, this lightweight library provides an object overlay for using POSIX sockets. The aim is to create a simple layer over the built-in socket component so as to facilitate its use, and provide common utility functions.

## Features
 - create a socket, with a maximum number of concurrent connections
 - manage the TCP/IP address and the port number
 - create/accept/close a connection
 - send/receive data
 - broadcast data

## Integration

Simply include the file to your project with the line `#include "socketObject.hpp"`