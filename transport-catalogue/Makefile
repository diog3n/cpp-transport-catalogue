default:
	g++ *.cpp -g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -fsanitize=address -std=c++17 -o main -Wfatal-errors -Wall 
debug:
	g++ *.cpp -g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -fsanitize=address -std=c++17 -o main -Wfatal-errors -Wall
debug-run:
	g++ *.cpp -g -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_DEBUG=1 -fsanitize=address -std=c++17 -o main -Wfatal-errors -Wall
	../executables/debug/main
release:
	g++ *.cpp -O1 -std=c++17 -o main -Wfatal-errors -Wall

release-run:
	g++ *.cpp -O1 -std=c++17 -o main -Wfatal-errors -Wall
