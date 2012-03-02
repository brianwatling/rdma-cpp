#include "RDMAClientSocket.h"
#include <iostream>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    try {
        rdma::ClientSocket clientSocket(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
        while(1) {
            rdma::Buffer sendPacket = clientSocket.getWriteBuffer();
            memset(sendPacket.buffer, 'b', sendPacket.size);
            clientSocket.write(sendPacket);
            rdma::Buffer readPacket = clientSocket.read();
            std::cout << "got: " << std::string((const char*)readPacket.buffer, readPacket.size) << std::endl;
            clientSocket.returnReadBuffer(readPacket);
        }
    } catch(std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }

    return 0;
}

