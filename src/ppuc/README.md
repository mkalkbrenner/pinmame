PPUC
====

macOS
-----

Install
```shell
brew install libusb yaml-cpp
```

Compile
```shell
cp cmake/ppuc/CMakeLists_osx-x64.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run
```shell
build/Release/ppuc -c src/ppuc/examples/lw3.yml
```

Linux
-----

Install
```shell
apt install libopenal-dev libyaml-cpp-dev libusb-dev
```
Compile
```shell
cp cmake/ppuc/CMakeLists_linux.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run
```shell
sudo build/Release/ppuc -c src/ppuc/examples/lw3.yml
```