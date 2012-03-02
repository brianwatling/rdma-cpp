#pragma once

#include "RDMAClientSocket.h"
#include <boost/noncopyable.hpp>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <stdint.h>

namespace rdma
{

class ServerSocket : boost::noncopyable
{
public:
    ServerSocket(const char* host, const char* port, uint32_t packetSize, uint32_t packetWindowSize);

    ~ServerSocket();

    ClientSocket* accept();

private:
    struct rdma_cm_id* listenId;
    uint32_t packetSize;
    uint32_t packetWindowSize;
};

};

