PPUC
====

macOS
-----

Install
```
brew install libusb yaml-cpp
```

Compile
```
cp cmake/libpinmame/CMakeLists_osx-x64.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run
```
build/Release/ppuc -c src/ppuc/examples/lw3.yml
```

Linux
-----

Install _libusb_ and _yaml-cpp_ dev packages.

Compile
```
cp cmake/libpinmame/CMakeLists_linux.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run
```
build/Release/ppuc -c src/ppuc/examples/lw3.yml
```