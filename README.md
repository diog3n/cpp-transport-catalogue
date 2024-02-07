# cpp-transport-catalogue
Это финальный проект, который я выполнял для Яндекс Практикума.

# Сборка
Собрать проект можно с помощью CMake. Для этого в директории, в которой лежит CMakeLists.txt выполните:
```bash
mkdir build && cd build
cmake -DProtobuf_INCLUDE_DIRS=<protobuf/headers/path>.. && cmake --build .
```
