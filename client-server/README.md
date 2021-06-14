# Client Server Application

## Description

This is a basic client server application that should evolve eventually to a basic chat client and server.

### Topics of investigation:

- Makefiles
- pthreads
- sockets
- signal handlers
- async I/O (`select` and friends)

## Build and run

```
make
./ncsa/server/build/server
./ncsa/client/build/client
```

## Resources

- Project structure is based on: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html
- Makefiles heavily based on: https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
- Socket programming (other than the linux manual): http://beej.us/guide/bgnet/html/

## Todo

- Investigate `poll`, `epoll`, `libevent`
- Finish the client, up till now I just used telnet for testing the server
- Make a protocol for communication and put it in a separate library
