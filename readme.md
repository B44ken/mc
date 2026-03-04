# minecraft
a build of `minecraftcpp-master` via emscripten, with multiplayer support!

## usage
```bash
# build
cd minecraft
emcmake cmake -S web -B ../build
cmake --build ../build -j
# then serve
python3 -m http.server 8080 --directory ../build
```

## project
`minecraft/handheld`: the original codebase with some edits
`minecraft/web`: web port including input, networking, image, etc. shims
`server`: naive websocket broadcaster