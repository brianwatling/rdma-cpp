#include "RDMAClientSocket.h"
#include <unistd.h>
#include <stdexcept>
#include <malloc.h>
#include <string.h>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

namespace rdma
{

std::string getLastErrorMessage()
{
    char buffer[256];
    const char* message = strerror_r(errno, buffer, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = 0;
    return boost::lexical_cast<std::string>(errno) + " " + std::string(message);
}

ClientSocket::ClientSocket(const char* host, const char* port, uint32_t packetSize, uint32_t packetWindowSize)
: clientId(NULL), memoryRegion(NULL), packetSize(packetSize), packetWindowSize(packetWindowSize), buffer(NULL), writeBuffers(packetWindowSize)
{
    struct rdma_addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    struct rdma_addrinfo* res = NULL;
    const int getAddrRet = rdma_getaddrinfo(const_cast<char*>(host), const_cast<char*>(port), &hints, &res);
    if(getAddrRet) {
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_getaddrinfo failed: ") + getLastErrorMessage());
    }

    struct ibv_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = packetWindowSize;
    attr.cap.max_recv_wr = packetWindowSize;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_sge = 1;
    attr.cap.max_inline_data = 0;
    attr.sq_sig_all = 1;

    const int createEndpointRet = rdma_create_ep(&clientId, res, NULL, &attr);
    rdma_freeaddrinfo(res);
    if(createEndpointRet) {
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_create_ep failed: ") + getLastErrorMessage());
    }

    setupBuffers();

    const int connectRet = rdma_connect(clientId, NULL);
    if(connectRet) {
        rdma_destroy_ep(clientId);
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_create_ep failed: ") + getLastErrorMessage());
    }
}

ClientSocket::ClientSocket(struct rdma_cm_id* clientId, uint32_t packetSize, uint32_t packetWindowSize)
: clientId(clientId), packetSize(packetSize), packetWindowSize(packetWindowSize), buffer(NULL), writeBuffers(packetWindowSize)
{
    setupBuffers();

    const int acceptRet = rdma_accept(clientId, NULL);
    if(acceptRet) {
        rdma_dereg_mr(memoryRegion);
        free(buffer);
        rdma_destroy_ep(clientId);
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_accept failed: ") + getLastErrorMessage());
    }
}

void ClientSocket::setupBuffers()
{
    const size_t bufferSize = packetSize * packetWindowSize * 2;
    buffer = reinterpret_cast<char*>(memalign(getpagesize(), bufferSize));
    if(!buffer) {
        rdma_destroy_ep(clientId);
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - memalign failed: ") + getLastErrorMessage());
    }

    memoryRegion = rdma_reg_msgs(clientId, buffer, bufferSize);
    if(!memoryRegion) {
        free(buffer);
        rdma_destroy_ep(clientId);
        throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_reg_msgs failed: ") + getLastErrorMessage());
    }

    for(uint32_t i = 0; i < packetWindowSize; ++i) {
        void* const location = buffer + i * packetSize;
        const int ret = rdma_post_recv(clientId, location, location, packetSize, memoryRegion);
        if(ret) {
            rdma_dereg_mr(memoryRegion);
            free(buffer);
            rdma_destroy_ep(clientId);
            throw std::runtime_error(std::string("rdma::ClientSocket::ClientSocket() - rdma_post_recv failed: ") + getLastErrorMessage());
        }
    }

    char* writeDataBegin = buffer + packetSize * packetWindowSize;
    for(uint32_t i = 0; i < packetWindowSize; ++i) {
        writeBuffers.push_back(Buffer(writeDataBegin + i * packetSize, packetSize));
    }
}

ClientSocket::~ClientSocket()
{
    rdma_disconnect(clientId);
    rdma_dereg_mr(memoryRegion);
    rdma_destroy_ep(clientId);
    free(buffer);
}

Buffer ClientSocket::getWriteBuffer()
{
    if(writeBuffers.empty()) {
        struct ibv_wc wc;
        do {
            const int ret = rdma_get_send_comp(clientId, &wc);
            if(ret <= 0) {
                throw std::runtime_error(std::string("rdma::ClientSocket::getWriteBuffer() - rdma_get_send_comp failed: ") + getLastErrorMessage());
            }
        } while(!wc.wr_id);
        return Buffer(reinterpret_cast<void*>(wc.wr_id), packetSize);
    }
    Buffer ret = writeBuffers.front();
    writeBuffers.pop_front();
    return ret;
}

void ClientSocket::write(const Buffer& buffer)
{
    const int ret = rdma_post_send(clientId, buffer.buffer, buffer.buffer, buffer.size, memoryRegion, 0);
    if(ret) {
        writeBuffers.push_front(buffer);
        throw std::runtime_error(std::string("rdma::ClientSocket::write() - rdma_post_send failed: ") + getLastErrorMessage());
    }
}

void ClientSocket::write(void* buffer, size_t len)
{
    const int ret = rdma_post_send(clientId, NULL, buffer, len, 0, IBV_SEND_INLINE);
    if(ret) {
        throw std::runtime_error(std::string("rdma::ClientSocket::write() - rdma_post_send failed: ") + getLastErrorMessage());
    }
}

Buffer ClientSocket::read()
{
    struct ibv_wc wc;
    const int ret = rdma_get_recv_comp(clientId, &wc);
    if(ret <= 0) {
        throw std::runtime_error(std::string("rdma::ClientSocket::read() - rdma_get_recv_comp failed: ") + getLastErrorMessage());
    }

    return Buffer(reinterpret_cast<void*>(wc.wr_id), wc.byte_len);
}

void ClientSocket::returnReadBuffer(const Buffer& buffer)
{
    const int ret = rdma_post_recv(clientId, buffer.buffer, buffer.buffer, packetSize, memoryRegion);
    if(ret) {
        throw std::runtime_error(std::string("rdma::ClientSocket::returnReadBuffer() - rdma_post_recv failed: ") + getLastErrorMessage());
    }
}

};

