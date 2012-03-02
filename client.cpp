#include "RDMAClientSocket.h"
#include <iostream>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    try {
        rdma::ClientSocket clientSocket(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
    } catch(std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }

    return 0;
}

