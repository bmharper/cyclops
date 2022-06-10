## Ubuntu 22.04 dependencies

sudo apt install -y libssl-dev libcurl4-openssl-dev libunwind-dev build-essential clang

To build live555 library:

```
cd cpp/third_party/live555
./genMakefiles linux
make -j
```
