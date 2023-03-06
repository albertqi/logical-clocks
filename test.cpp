#include <iostream>
#include <string>

#define PADDED_LENGTH 50

bool allTestsPassed = true;

void test(bool b, std::string msg)
{
    int i = msg.size();
    msg += b ? "PASSED" : "FAILED";
    msg.insert(i, PADDED_LENGTH - msg.size(), ' ');
    allTestsPassed = allTestsPassed && b;
    std::cerr << msg << std::endl;
}

int main()
{
    std::cerr << "\nRUNNING TESTS..." << std::endl;

    std::string msg = allTestsPassed ? "\nALL TESTS PASSED :)\n" :
                                       "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}