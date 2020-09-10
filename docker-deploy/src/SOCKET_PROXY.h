//
// Created by shihe on 2/15/2020.
//

#ifndef HW2_SOCKET_PROXY_H
#define HW2_SOCKET_PROXY_H

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <time.h>
#include <string.h>
#include "PROXY_EXCEPTION.h"
#define SIZE_OF_DATA_CHUNK 65535

using namespace std;

std::ofstream Log ("proxy.log");

std::string badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
std::string badGateway = "HTTP/1.1 502 Bad Gateway\r\n\r\n";

void Bad_Gateway (int server_connection_fd, int client_connection_fd){
    std::cerr << "Error: 502 Bad Gateway" <<std::endl;
    //close(server_connection_fd);
    send(client_connection_fd, badGateway.c_str(), badGateway.length(), 0);
    throw errorException();
}

void Bad_Request (int server_connection_fd, int client_connection_fd) {
    std::cerr << "Error: 400 Bad Request" <<std::endl;
    //close(server_connection_fd);
    send(client_connection_fd, badRequest.c_str(), badRequest.length(), 0);
    throw errorException();
}

std::string timeToString(time_t time) {
    struct tm *timeinfo;
    timeinfo = localtime(&time);
    char *res = asctime(timeinfo);
    std::string str = std::string(res);
    str = str.substr(0, str.find("\n"));
    
    return str;
}

std::string getUTC(time_t t) {
    struct tm *ptm;
    ptm = gmtime (&t);
    time_t UTCtime = mktime(ptm);
    
    return timeToString(UTCtime);
}

int loopRecv(int start_fd, std::vector<char>* buffer) {
    int currentSize = SIZE_OF_DATA_CHUNK;
    buffer->resize(currentSize);
    int index = 0;
    int status;
    while(1){
        int currentRecv = recv(start_fd, &buffer->data()[index], SIZE_OF_DATA_CHUNK, 0);
        if (currentRecv == 0 && index == 0) {
            status = 0;
            break;
        }
        if(currentRecv < SIZE_OF_DATA_CHUNK && currentRecv >= 0) {
            index += currentRecv;
            status = index;
            break;
        } else if (currentRecv < 0) {
            status = -1;
            break;
        }
        index += currentRecv;
        currentSize += currentRecv;
        buffer->resize(currentSize);
    }
    return status;
}

class socketProxy {
public:
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname;
    const char *port;

    socketProxy() {
        status = 0;
        socket_fd = 0;
        host_info_list = nullptr;
    }
    ~socketProxy() {
        freeaddrinfo(host_info_list);
        close(socket_fd);
    }
    int initiateSocketAsServer();
    int initiateSocketAsClient();
    int bindListen();
    int acceptFromClient();
    int connectToServer();

};

int socketProxy::initiateSocketAsServer() {
  // basic guarantee
    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    port = "12345";
    hostname = NULL;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    }

    socket_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    }
    return 0;
}

int socketProxy::initiateSocketAsClient() {
  // basic guarantee
    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    }

    socket_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    }
    return 0;
}

int socketProxy::bindListen() {
  // basic guarantee
    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    } 

    status = listen(socket_fd, 100);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;

	throw errorException();
    } 
    return 0;
}

int socketProxy::acceptFromClient() {
  // basic guarantee
    cout << "Waiting for connection on port " << port << endl;
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;

	throw errorException();
    }
    return client_connection_fd;
}

int socketProxy::connectToServer() {
  // basic guarantee
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot connect to socket" << endl;
        cerr << "  (" << hostname << "," << port << ")" << endl;
       
	throw errorException();
    }
    std::cout << "Successfully connected to server" << std::endl;
    return 0;
}

#endif //HW2_SOCKET_PROXY_H
