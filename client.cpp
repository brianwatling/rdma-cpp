#include "RDMAClientSocket.h"
#include <iostream>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

uint64_t getnsecs(const struct timespec& in)
{
    return in.tv_sec * 1000000000LL + in.tv_nsec;
}

int main(int argc, char* argv[])
{
    try {
        rdma::ClientSocket clientSocket(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
        struct timespec nbegin;
        struct timespec nend;
        const int count = atoi(argv[5]);
        clock_gettime(CLOCK_REALTIME, &nbegin);
        for(int i = 0; i < count; ++i) {
            rdma::Buffer sendPacket = clientSocket.getWriteBuffer();
            memset(sendPacket.buffer, 'b', sendPacket.size);
            clientSocket.write(sendPacket);
            rdma::Buffer readPacket = clientSocket.read();
            clientSocket.returnReadBuffer(readPacket);
        }
        clock_gettime(CLOCK_REALTIME, &nend);
        const uint64_t nsecs = getnsecs(nend) - getnsecs(nbegin);
        std::cout << "wrote " << count << " packets of size " << argv[3] << " in " << nsecs << " nsecs" << std::endl;
    } catch(std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }

    return 0;
}

