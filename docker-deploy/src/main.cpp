#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/time.h>
#include "GET_PROCESSING.h"
#include "POST_PROCESSING.h"
#include <algorithm>
#include <bits/stdc++.h>
#include <vector>

#define SIZE_OF_HEADER 8192
#define SIZE_OF_DATA_CHUNCK 65535

int tunnel(int start_fd, int end_fd){
    // strong guarantee
    std::vector<char> data;

    int status=loopRecv(start_fd, &data);
    if(status > 0) {
        send(end_fd, data.data(), status, 0);
    } else if (status < 0){
        std::cerr << "Error: cannot receive data" << std::endl;
	throw errorException();
    }
    data.clear();
    return status;
}

void connectProcessing(int client_connection_fd, int server_connection_fd, int threadID) {
    // strong guarantee
    const char *message = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_connection_fd, message, strlen(message), 0);
    Log << threadID << ": Responding \"HTTP/1.1 200 OK\"" <<std::endl;
    
    fd_set set;
    while(1) {
        FD_ZERO(&set);
        FD_SET(client_connection_fd, &set);
        FD_SET(server_connection_fd, &set);
        int status = select(std::max(client_connection_fd, server_connection_fd) + 1, &set, NULL, NULL, 0);
	    if (status <= 0){
	        std::cerr << "Error: select" <<std::endl;
		close(server_connection_fd);
	        throw errorException();

	    }
        
        if(FD_ISSET(client_connection_fd, &set)) {
            try{
	      if(tunnel(client_connection_fd, server_connection_fd) == 0){
		return;
	      }
            } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
                close(server_connection_fd);
                return;
            }
        } else {
            try{
                tunnel(server_connection_fd, client_connection_fd);
	    } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
                close(server_connection_fd);
                return;
            }
        }	
    }
    Log << threadID << ":  Tunnel closed" << std::endl;    
}

std::string getIP(int client_connection_fd) {
    struct sockaddr_in client_sock_addr;
    socklen_t len = sizeof(client_sock_addr);
    getpeername(client_connection_fd, (struct sockaddr *) &client_sock_addr, &len);
    std::string ip = inet_ntoa((&client_sock_addr)->sin_addr);
    return ip;
}

void processing(int client_connection_fd, cache_proxy *cacheProxy, int threadID) {
    std::vector<char> buffer;
    int req = loopRecv(client_connection_fd, &buffer);

    if (req <= 0){
      std::cerr<< "Error: invalid request" << std::endl;
      send(client_connection_fd, badRequest.c_str(), badRequest.length(), 0);
      close(client_connection_fd);
      return;
    }

    std::cout<< buffer.data() <<std::endl;
    std::string reqString = buffer.data();
    std::cout<< "request string: "<< reqString <<std::endl;
    requestParsed *reqParsed = new requestParsed();
    reqParsed->initialize(reqString);
    std::string method = reqParsed->getMethod();
    socketProxy socketToServer;
    std::string hostname = reqParsed->getHostname();
    std::string port = reqParsed->getPort();
    std::string firstLine = reqParsed->getFirstLine();

    Log << threadID << ": \"" << firstLine << "\" from " << getIP(client_connection_fd) << " @ " << getUTC(time(0)) << std::endl;
    
    socketToServer.hostname = hostname.c_str();
    std::cout<< "host name: " << socketToServer.hostname << std::endl;
    socketToServer.port = port.c_str();
    try{
        socketToServer.initiateSocketAsClient();
        socketToServer.connectToServer();
    } catch (errorException & e) {
        std::cout << e.what() <<std::endl;
        send(client_connection_fd, badRequest.c_str(), badRequest.length(), 0);
        close(client_connection_fd);
        return;
    }

    if (method == "CONNECT") {
        try{
	    connectProcessing(client_connection_fd, socketToServer.socket_fd, threadID);
        } catch (errorException & e) {
            std::cout << e.what() <<std::endl;
            close(client_connection_fd);
            return;
        }
    }
    else if (method == "GET") {
        try{
	  getProcessing(client_connection_fd, socketToServer.socket_fd, reqParsed, cacheProxy, threadID);
        } catch (errorException & e) {
            std::cout << e.what() <<std::endl;
            close(client_connection_fd);
            return;
        }
    } else if (method == "POST") {
        try{
	  postProcessing(client_connection_fd, socketToServer.socket_fd, reqParsed, &buffer, req, threadID);
        } catch (errorException & e) {
            std::cout << e.what() <<std::endl;
            close(client_connection_fd);
            return;
        }
    }
    close(socketToServer.socket_fd);
    close(client_connection_fd);
}

int main() {
  
    // SOCKET_PROXY listen;
    socketProxy listenProxy;
    try{
      listenProxy.initiateSocketAsServer();
      listenProxy.bindListen();
    }
    catch(errorException & e) {
      std::cout << e.what() <<std::endl;
    }
    cache_proxy cacheProxy;
    int threadID = 0;
      
    while(1) {
          int client_connection_fd;
	  try{
	    client_connection_fd = listenProxy.acceptFromClient();
	    std::thread newThread(processing, client_connection_fd, &cacheProxy, threadID);
	    std::cout<< "Thread created!" <<std::endl;
	    newThread.detach();
	  }
    
	  catch(errorException & e) {
	    std::cout << e.what() <<std::endl;
	  }
	  
	  threadID++;
      }
    
    return EXIT_SUCCESS;
}
