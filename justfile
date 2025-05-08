build:
  cmake -S . -B build
  cmake --build build

run target:
  ./build/{{target}}

