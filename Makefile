
all: server client

LIBS += -lrdmacm -libverbs

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

server: server.o RDMAServerSocket.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

client: client.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
