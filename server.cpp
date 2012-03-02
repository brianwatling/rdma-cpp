#include "RDMAServerSocket.h"
#include <iostream>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    try {
        rdma::ServerSocket serverSocket(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));

        while(1) {
            rdma::ClientSocket* clientSocket = serverSocket.accept();
            try {
                while(1) {
                    rdma::Buffer readPacket = clientSocket->read();
                    rdma::Buffer sendPacket = clientSocket->getWriteBuffer();
                    memcpy(sendPacket.buffer, readPacket.buffer, readPacket.size);
                    clientSocket->write(sendPacket);
                    clientSocket->returnReadBuffer(readPacket);
                }
            } catch(std::exception& e) {
                std::cerr << "client exception: " << e.what() << std::endl;
            }
            delete clientSocket;
        }
    } catch(std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }

    return 0;
}

