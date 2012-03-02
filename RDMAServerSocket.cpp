#include "RDMAServerSocket.h"
#include <errno.h>
#include <string.h>
#include <stdexcept>

namespace rdma
{

ServerSocket::ServerSocket(const char* host, const char* port, uint32_t packetSize, uint32_t packetWindowSize)
: listenId(NULL), packetSize(packetSize), packetWindowSize(packetWindowSize)
{
    struct rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = RAI_PASSIVE;
    hints.ai_port_space = RDMA_PS_TCP;
    struct rdma_addrinfo* res = NULL;
    const int getAddrRet = rdma_getaddrinfo(const_cast<char*>(host), const_cast<char*>(port), &hints, &res);
    if(getAddrRet) {
        throw std::runtime_error(std::string("rdma::ServerSocket::ServerSocket() - rdma_getaddrinfo failed: ") + getLastErrorMessage());
    }

    struct ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = packetWindowSize;
    attr.cap.max_recv_wr = packetWindowSize;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_sge = 1;
    attr.cap.max_inline_data = 0;
    attr.sq_sig_all = 1;

    const int createEndpointRet = rdma_create_ep(&listenId, res, NULL, &attr);
    rdma_freeaddrinfo(res);
    if(createEndpointRet) {
        throw std::runtime_error(std::string("rdma::ServerSocket::ServerSocket() - rdma_create_ep failed: ") + getLastErrorMessage());
    }

    const int listenRet = rdma_listen(listenId, 0);
    if(listenRet) {
        rdma_destroy_ep(listenId);
        throw std::runtime_error(std::string("rdma::ServerSocket::ServerSocket() - rdma_listen failed: ") + getLastErrorMessage());
    }
}

ServerSocket::~ServerSocket()
{
    rdma_destroy_ep(listenId);
}

ClientSocket* ServerSocket::accept()
{
    struct rdma_cm_id* clientId = NULL;
    const int ret = rdma_get_request(listenId, &clientId);
    if(ret) {
        throw std::runtime_error(std::string("rdma::ServerSocket::accept() - rdma_get_request failed: ") + getLastErrorMessage());
    }

    return new ClientSocket(clientId, packetSize, packetWindowSize);
}

};

