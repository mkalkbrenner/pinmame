PPUC
====

Compile
```
cp cmake/libpinmame/CMakeLists_linux.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run
```
build/Release/ppuc
```