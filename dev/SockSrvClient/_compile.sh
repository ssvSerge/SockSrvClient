# g++ -std=c++17 -fprofile-arcs -ftest-coverage -static-libstdc++ -static-libasan -lgcov -O -g -fsanitize=address -fno-omit-frame-pointer -Wall -I ../common/include -o srv_cli main.cpp ../common/src/SockServerClient.cpp ../common/src/SockServerClientOsTools.cpp

g++ -std=c++17 -lpthread -fprofile-arcs -ftest-coverage -O0 -g -Wall -I ../common/include -c main.cpp  -o part1.o 
g++ -std=c++17 -lpthread -fprofile-arcs -ftest-coverage -O0 -g -Wall -I ../common/include -c ../common/src/SockServerClient.cpp -o part2.o 
g++ -std=c++17 -lpthread -fprofile-arcs -ftest-coverage -O0 -g -Wall -I ../common/include -c ../common/src/SockServerClientOsTools.cpp -o part3.o 
g++ -lpthread  -lgcov --coverage part1.o part2.o part3.o -o srv_cli 

./srv_cli
geninfo . -b . -o ./coverage_info
genhtml coverage_info -o ./temp
