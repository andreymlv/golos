# Golos

Speak with your friends.

## Quick start

On client with flag `-f` you can send music to the server.
Without this flag capture mode will be started.

```
git clone --recursive https://github.com/andreymlv/golos.git
cd golos
mkdir build
cd build
cmake ..
make -j8
./server -p <port>
./client -a <ip> -p <port> [-f <mp3 or wav or flac file>]
```
