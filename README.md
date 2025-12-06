### Build
```
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="/usr/local/opt/llvm/bin/clang++"
cmake --build build
```