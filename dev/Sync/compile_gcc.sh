g++ -pedantic -Wall -fprofile-arcs -ftest-coverage -std=gnu++17 serializer_test.cpp -o test_app
./test_app
geninfo . -b . -o ./coverage_info
genhtml coverage_info -o ./temp

