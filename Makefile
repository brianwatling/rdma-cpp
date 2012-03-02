
all: server client

CXXFLAGS += -O3

LIBS += -lrdmacm -libverbs -lrt

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

server: server.o RDMAServerSocket.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

client: client.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

.PHONY:
clean:
	rm -f *.o server client
