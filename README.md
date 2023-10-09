# Golos

Speak with your friends.

## Quick start

```
git clone --recursive https://github.com/andreymlv/golos.git
cd golos
mkdir build
cd build
cmake ..
make -j8
./server -p [port]
# File mode
./client -a <ip> -p [port] -f [mp3 or wav or flac file]
# Capture mode
./client -a <ip> -p [port]
```
