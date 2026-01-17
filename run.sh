./compile.sh
cmake --build build
cd "$(dirname "$0")/build/bin"
 gdb -ex run --args ./triangle