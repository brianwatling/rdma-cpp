#pragma once

#include <boost/noncopyable.hpp>
#include <string>
#include <stdint.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

namespace rdma
{

std::string getLastErrorMessage();

class ClientSocket : boost::noncopyable
{
public:
    ClientSocket(const char* host, const char* port, uint32_t packetSize, uint32_t packetWindowSize);

    ClientSocket(struct rdma_cm_id* clientId, uint32_t packetSize, uint32_t packetWindowSize);

    ~ClientSocket();

private:
    void setupBuffers();

    struct rdma_cm_id* clientId;
    struct ibv_mr* memoryRegion;
    uint32_t packetSize;
    uint32_t packetWindowSize;
    char* buffer;
};

};

