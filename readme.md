# minecraft wasm
a build of `minecraftcpp-master` via emscripten

## usage
```bash
# build
cd minecraft
emcmake cmake -S web -B ../build
cmake --build ../build -j

# then serve
python3 -m http.server 8080 --directory ../build
```

## notes
- networking is compiled out (`NO_NETWORK`) for browser compatibility