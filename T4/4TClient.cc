#include <thread>
#include "4T.h"

int main(int argc, char **argv)
{
    T4Client ec(argv[1], argv[2], argv[3]);

    std::thread net_thread([&ec](){ ec.net_thread(); });

    ec.login();

    ec.input_thread();
}

