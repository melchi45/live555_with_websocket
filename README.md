# live5555
Live555 based RTSP with WebSocket transport 

This source need websocketpp and boost library.

```
sudo apt update
sudo apt install libwebsocketpp-dev
sudo apt install libboost-all-dev
sudo apt install libspdlog-dev
```
### build command
```
cmake . -B linux -DLIVE555_ENABLE_OPENSSL=OFF
cmake --build linux
```
