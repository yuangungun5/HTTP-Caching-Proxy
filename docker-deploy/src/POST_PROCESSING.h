//
// Created by shihe on 2/23/2020.
//

#ifndef HW2_POST_PROCESSING_H
#define HW2_POST_PROCESSING_H
#include "CACHE_PROXY.h"
#include "SOCKET_PROXY.h"
#include "PARSED_REQUEST.h"
#include "PARSED_RESPONSE.h"
#define SIZE_OF_DATA_CHUNK 65535

int postID = -1;

void postProcessing(int client_connection_fd, int server_connection_fd, requestParsed *reqParsed, std::vector<char> *buffer, int index, int threadID) {

  int bodyLength = index - reqParsed->getHeaderLength();
  std::cout<<"current bodylength: "<<bodyLength <<std::endl;
  std::cout<<"content length: "<<reqParsed->getContentLength()<<std::endl;

  postID = threadID;

  // receive full message
  while (bodyLength < reqParsed->getContentLength()){
    buffer->resize(SIZE_OF_DATA_CHUNK + index);
    int currentReq = recv(client_connection_fd, &buffer->data()[index], SIZE_OF_DATA_CHUNK, 0);
    if (currentReq <= 0){
        Bad_Request (server_connection_fd, client_connection_fd);     // basic guarantee
    }
    std::cout<<"currentReq: "<<currentReq<<std::endl;
    index += currentReq;
  }

  int status = send(server_connection_fd, &buffer->data()[0], index, 0);
  if (status <= 0) {
    Bad_Request (server_connection_fd, client_connection_fd);     // basic guarantee
  }

  std::vector<char> serverBuffer;
  serverBuffer.resize(SIZE_OF_DATA_CHUNK);
  int currentRecv = loopRecv(server_connection_fd, &serverBuffer);
  if (currentRecv <= 0) {
      Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
  }
  std::cout<< "currentRev: "<<currentRecv<<std::endl;
  Log << postID << ": POST request sent" << std::endl;
  int indexRes = currentRecv;
  responseParsed newRes;
  std:string newResString = serverBuffer.data();
  newRes.initialize(newResString);
  if(newRes.header["Transfer-Encoding"] == "chunked") {
    if(send(client_connection_fd, &serverBuffer.data()[0], indexRes, 0) <= 0) {
        Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
    }
    while(1) {
      std::vector<char> newResBuffer;
      newResBuffer.resize(SIZE_OF_DATA_CHUNK);
      currentRecv = recv(server_connection_fd, &newResBuffer.data()[0], SIZE_OF_DATA_CHUNK, 0);
      if (currentRecv <= 0) {
  	  Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
      }
      if(send(client_connection_fd, &newResBuffer.data()[0], currentRecv, 0) <= 0 ) {
 	  Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
      }
      if(currentRecv < 5 || (newResBuffer.data()[0] == '0' && newResBuffer.data()[1] == '\r' && newResBuffer.data()[2] == '\n')) {
        std::cout<<"Break because of Transfer-Encoding"<<std::endl;
        break;
      }
    }
  } else {
    while (indexRes < newRes.getHeaderLength() + newRes.getContentLength()){
      serverBuffer.resize(indexRes + SIZE_OF_DATA_CHUNK);
      currentRecv = recv(server_connection_fd, &serverBuffer.data()[indexRes], SIZE_OF_DATA_CHUNK, 0);
      if (currentRecv <= 0){
	  Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
      }
      std::cout<<"currentRecv: "<<currentRecv<<std::endl;
      indexRes += currentRecv;
    }
    status = send(client_connection_fd, &serverBuffer.data()[0], indexRes, 0);
    if (status <= 0) {
        Bad_Gateway (server_connection_fd, client_connection_fd);     // basic guarantee
    }
  }

  Log << postID << ": POST response received." <<std::endl;
}


#endif //HW2_POST_PROCESSING_H
