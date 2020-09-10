//
// Created by shihe on 2/23/2020.
//

#ifndef HW2_GET_PROCESSING_H
#define HW2_GET_PROCESSING_H

#include "CACHE_PROXY.h"
#include "SOCKET_PROXY.h"
#include "PARSED_REQUEST.h"
#include "PARSED_RESPONSE.h"
#define SIZE_OF_DATA_CHUNK 65535

int thisID = -1;

void cacheProcess(cache_proxy *cacheProxy, std::string key, int client_connection_fd, requestParsed *reqParsed, responseParsed *newRes, std::vector<char> *newResponse, int index) {
    std::set<std::string> reqCacheControl = reqParsed->getCacheControl();
    std::set<std::string> resCacheControl = newRes->getCacheControl();
    int status = -1;
    if (resCacheControl.size() == 0 && newRes->header.count("Expires") == 0) {
        // Not cachable
        std::cout<<"Not cachable because server didn't specify cache-control in response"<<std::endl;
	Log << thisID << ": not cacheable because server did not specify cache-control in response." << std::endl;
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
	
    } else if(index > SIZE_OF_DATA_CHUNK) {
        //Not cachable
        std::cout<<"Not cachable because the size of the response is larger than 65kB"<<std::endl;
	Log << thisID << ": not cacheable because the size of the response is larger than 65kB." << std::endl;
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
	
    } else if (resCacheControl.find("no-store") != resCacheControl.end()) {
        // Not cachable

        Log << thisID << ": not cacheable because server specifies 'no-store' in cache-control." << std::endl;

        std::cout<<"Not cachable because of server specifies 'no-store' as cache-control in response"<<std::endl;
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
    } else if (resCacheControl.find("private") != resCacheControl.end()) {
        // Not cachable

      	Log << thisID << ": not cacheable because server specifies 'private' in cache-control." << std::endl;

        std::cout<<"Not cachable because of server specifies 'prvivate' as cache-control in response"<<std::endl;
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);

    } else if (resCacheControl.find("no-cache") != resCacheControl.end()
               || newRes->getMaxAge() == 0
	       || resCacheControl.find("must-revalidate") != resCacheControl.end()) {

      	Log << thisID << ": cached, but requires re-validation." << std::endl;
        std::cout<<" Cached, but requires re-validation"<<std::endl;
        cacheProxy->store_data(key, newRes, newResponse, index);
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
    } else if (newRes->getMaxAge() != -1 ) {
        time_t expiresAt = time(0) + newRes->getMaxAge();
        Log << thisID << ": cached, but expires at " << getUTC(expiresAt) << std::endl;
        std::cout<< "Cached, but expires at "<< getUTC(expiresAt) << std::endl;
        cacheProxy->store_data(key, newRes, newResponse, index);
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
    } else if (newRes->header.count("Expires") != 0) {
        Log << thisID << ": cached, but expires at " << newRes->header["Exipres"] << std::endl;
        std::cout<< "Cached, but expires at "<< newRes->header["Expires"] << std::endl;
        cacheProxy->store_data(key, newRes, newResponse, index);
        send(client_connection_fd, &newResponse->data()[0], index, 0);
    } else if (resCacheControl.size() == 0) {
        // Not cachable
        Log << thisID << ": not cacheable because server did not specify cache-control. " << std::endl;
        std::cout<<"Not cachable because the server didn't specifies cache-control in response"<<std::endl;
        status = send(client_connection_fd, &newResponse->data()[0], index, 0);
    }

    if(status <= 0) {
      // basic guarantee
        Log << thisID << "ERROR" <<std::endl;	
        std::cerr << "Error: 400 Bad Request" <<std::endl;
	throw errorException();
    }

    Log << thisID << ": Responding \"" << newRes->getFirstLine() << std::endl;    
}

void validate(cache_proxy *cacheProxy, std::string key, int server_connection_fd, int client_connection_fd, requestParsed *reqParsed) {
    responseParsed *cachedRes = cacheProxy->proxyMap[key]->value;
    std::vector<char> cachedResponse = cacheProxy->proxyMap[key]->getResponse();
    
    if(cachedRes->header.count("Last-Modified") == 0 && cachedRes->header.count("ETag") == 0 ) {
        std::cout<<"lack of 'Last-Modified' AND 'ETag' header, a new response would be get"<< std::endl;
	// directly forward to server
	if(send(server_connection_fd, reqParsed->getRequest().c_str(), reqParsed->getRequest().length(), 0) <= 0) {
            Bad_Request (server_connection_fd, client_connection_fd);
	    }
        std::vector<char> newResponse;
        newResponse.resize(SIZE_OF_DATA_CHUNK);
        int currentSize = SIZE_OF_DATA_CHUNK;
        // Get a new response
        int currentRecv = recv(server_connection_fd, &newResponse.data()[0], SIZE_OF_DATA_CHUNK, 0);
        int index = currentRecv;
        if(index <= 0) {
            Bad_Gateway(server_connection_fd, client_connection_fd);
        }
	
	Log << thisID << ": Requesting \"" << reqParsed->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;
	
        std::cout<<"Response get."<<std::endl;
        std::string resString = newResponse.data();
        responseParsed *newRes = new responseParsed();
        newRes->initialize(resString);
	
	Log << thisID << ": Received \"" << newRes->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;

        if(newRes->header["Transfer-Encoding"] == "chunked") {
            if(send(client_connection_fd, &newResponse.data()[0],currentRecv, 0) <= 0) {
                Bad_Gateway(server_connection_fd, client_connection_fd);
            }
            while(1) {
                std::vector<char> buffer;
                buffer.resize(SIZE_OF_DATA_CHUNK);
                int currentRes = recv(server_connection_fd, &buffer.data()[0], SIZE_OF_DATA_CHUNK, 0);
                if (currentRes <= 0) {
                    Bad_Gateway(server_connection_fd, client_connection_fd);
                }
                if(send(client_connection_fd, &buffer.data()[0], currentRes, 0) <= 0) {
                    Bad_Gateway(server_connection_fd, client_connection_fd);
                }
                if(currentRes < 5 || (buffer.data()[0] == '0' && buffer.data()[1] == '\r' && buffer.data()[2] == '\n')) {
                    break;
                }
            }
        } else {
            while (index - newRes->getHeaderLength() < newRes->getContentLength()) {
                newResponse.resize(index + SIZE_OF_DATA_CHUNK);
                currentRecv = recv(server_connection_fd, &newResponse.data()[index], SIZE_OF_DATA_CHUNK, 0);
                if (currentRecv <= 0) {
                    Bad_Gateway(server_connection_fd, client_connection_fd);
                }
                index += currentRecv;
            }
	        std::cout<< "get code: " <<newRes->getCode() <<std::endl;
            if (newRes->getCode() != "200"){
                std::cout<<"The website has not been modified and can be directly send to client"<<std::endl;
                if(send(client_connection_fd, &cachedResponse.data()[0], cacheProxy->proxyMap[key]->getIndex(), 0) < cacheProxy->proxyMap[key]->getIndex()) {
                    std::cerr << "Error: error when send from cache to client" <<std::endl;
                    close(server_connection_fd);
                    throw errorException();
                }
            } else {
	        // 200 - update
                std::cout<< "Cache has been modified needs to be updated"<<std::endl;
                try {
                    cacheProcess(cacheProxy, key, client_connection_fd, reqParsed, newRes, &newResponse, index);
                } catch (errorException & e) {
                    std::cout << e.what() <<std::endl;
		    close(server_connection_fd);
                    return;
                }
                //cacheProcess(cacheProxy, key, client_connection_fd, reqParsed, &newRes);
            }
        }
    } else {
        // Has 'Last-Modified' AND 'ETag' header, send request to revalidate
        if(cachedRes->header.count("Last-Modified") != 0){
	        std::cout<< "cache previous response [Last-Modified]: " << cachedRes->header["Last-Modified"] <<std::endl;
                std::string ifModified = cachedRes->header["Last-Modified"];
	        reqParsed->header["If-Modified-Since"] = ifModified;
        }
	    else{
            std::cout<< "cache previous response [ETag]: " << cachedRes->header["ETag"] <<std::endl;
            std::string ifNoneMatch = cachedRes->header["ETag"];
            reqParsed->header["If-None-Match"] = ifNoneMatch;
	    }
	    //int len = reqParsed->reconstruct().length();
        if(send(server_connection_fd, reqParsed->getRequest().c_str(), reqParsed->getRequest().length(), 0) <= 0) {
            Bad_Request (server_connection_fd, client_connection_fd);
        }
	std::cout<<"new response"<<std::endl;
        std::vector<char> newResponse;
        newResponse.resize(SIZE_OF_DATA_CHUNK);
        int currentSize = SIZE_OF_DATA_CHUNK;
        // Get a new response
        int currentRecv = recv(server_connection_fd, &newResponse.data()[0], SIZE_OF_DATA_CHUNK, 0);
        int index = currentRecv;
        if (currentRecv <= 0) {
            Bad_Gateway (server_connection_fd, client_connection_fd);
        }
	
	Log << thisID << ": Requesting \"" << reqParsed->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;
	
        std::cout<<"Response get."<<std::endl;
        std::cout<<"parse new response"<<std::endl;
        std::string resString = newResponse.data();
        responseParsed *newRes = new responseParsed();
        std::cout<<"trying to initialize newRes."<<std::endl;
        newRes->initialize(resString);

	Log << thisID << ": Received \"" << newRes->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;
	
	std::cout<< "After reconstructing header, get code: " << newRes->getCode() <<std::endl;
        if(newRes->getCode() == "200") {
            std::cout<< "Cache has been modified needs to be updated"<<std::endl;
            if(newRes->header["Transfer-Encoding"] == "chunked") {
                if(send(client_connection_fd, &newResponse.data()[0],currentRecv, 0) <= 0) {
                    Bad_Gateway (server_connection_fd, client_connection_fd);
                }
                while(1) {
                    std::vector<char> buffer;
                    buffer.resize(SIZE_OF_DATA_CHUNK);
                    int currentRes = recv(server_connection_fd, &buffer.data()[0], SIZE_OF_DATA_CHUNK, 0);
                    if (currentRes <= 0) {
                        Bad_Gateway (server_connection_fd, client_connection_fd);
                    }
                    if(send(client_connection_fd, &buffer.data()[0], currentRes, 0) <= 0) {
                        Bad_Gateway (server_connection_fd, client_connection_fd);
                    }
                    if(currentRes < 5 || (buffer.data()[0] == '0' && buffer.data()[1] == '\r' && buffer.data()[2] == '\n')) {
                        std::cout<<"Break because of Transfer-Encoding"<<std::endl;
                        break;
                    }
                }
            } else {
                while (index - newRes->getHeaderLength() < newRes->getContentLength()) {
                    newResponse.resize(index + SIZE_OF_DATA_CHUNK);
                    currentRecv = recv(server_connection_fd, &newResponse.data()[index], SIZE_OF_DATA_CHUNK, 0) ;
                    if (currentRecv <= 0) {
                        Bad_Gateway (server_connection_fd, client_connection_fd);
                    }
                    index += currentRecv;
                }
                try{
                    cacheProcess(cacheProxy, key, client_connection_fd, reqParsed, newRes, &newResponse, index);
                } catch (errorException & e) {
                    std::cout << e.what() <<std::endl;
		    close(server_connection_fd);
		    return;
                }
            }
        }
        else if(newRes->getCode() == "304") {
            std::cout<<"The website has not been modified and can be directly send to client"<<std::endl;
            if(send(client_connection_fd, &cachedResponse.data()[0], cacheProxy->proxyMap[key]->getIndex(), 0) <= 0) {
                Bad_Gateway (server_connection_fd, client_connection_fd);
            }
            //send(client_connection_fd, cachedRes->getResponse().c_str(), cachedRes->getLength(), 0);
        } else {
            std::cerr<< "Http Code " << newRes->getCode() << std::endl;
            throw errorException();
        }
            //cacheProcess(cacheProxy, keyFirstLine, client_connection_fd, reqParsed, &newRes, &newResponse, index);
            //send(client_connection_fd, &newResponse.data()[0], index, 0);
    }
}


time_t strToTime(std::string str){
  struct tm tm;
  memset(&tm, 0, sizeof(tm));

  strptime(str.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm);
  time_t t = mktime(&tm);
  
  return t;
}

int checkStatus(requestParsed *reqParsed, node *responseNode) {
  //if (responseNode->value->getMaxAge() > 0 && reqParsed->getMaxAge() > 0 ) {

    time_t t = time(0);
    if (responseNode->value->getMaxAge() > 0 && responseNode->isExpired()) {
      return 1;
    } else if (reqParsed->getMaxAge() > 0 && responseNode->dateOfBirth + reqParsed->getMaxAge() < t) {
      return 2;
    }
    else if (responseNode->value->header.count("Expires") != 0 && strToTime(responseNode->value->header["Expires"]) < t){
      return 4;
    }
    else {
      return 0;
    }
    
    return 3;
}

void getProcessing(int client_connection_fd, int server_connection_fd, requestParsed *reqParsed, cache_proxy *cacheProxy, int threadID) {
    std::string keyFirstLine = reqParsed->getFirstLine();
    thisID = threadID;
    std::set<std::string> reqCacheControl = reqParsed->getCacheControl();
    if (reqParsed->getMaxAge() > 0){
        Log << thisID << ": NOTE max-age=" << reqParsed->getMaxAge() << " in request" <<std::endl;
    }

    if(cacheProxy->proxyMap.count(keyFirstLine) != 0) {
        std::cout<< "What the client is looking for is in the cache."<<std::endl;

	node * responseNode = cacheProxy->proxyMap[keyFirstLine];
        // needs validation or expire
	    std::cout<< "check cache size: " << cacheProxy->proxyMap.size() <<std::endl;
	
        int status = checkStatus(reqParsed, responseNode);
	    std::cout << "status: " << status <<std::endl;
        if (status == 0) {
            // valid
	  
  	    Log << thisID << ": in cache, valid" << std::endl;

	    if(send(client_connection_fd, &responseNode->getResponse().data()[0], responseNode->getIndex(), 0) < 0 ) {
	        close(server_connection_fd);
                throw errorException();
            }
	    Log << thisID << ": Responding \"" << keyFirstLine << std::endl;
	    
        } else if (status == 1) {
            // Expired because of response
            try{
                validate(cacheProxy, keyFirstLine, server_connection_fd, client_connection_fd, reqParsed);
            } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
            }
	    time_t expiredAt = responseNode->dateOfBirth + responseNode->value->getMaxAge();
  	    Log << thisID << ": in cache, but expired at " << getUTC(expiredAt) << " because response max-age=" << responseNode->value->getMaxAge() << std::endl;

            // ID: in cache, but expired at responseNode->dateOfBirth + responseNode->value->getMaxAge();
        } else if (status == 2) {
            // Expired because of request
            try{
                validate(cacheProxy, keyFirstLine, server_connection_fd, client_connection_fd, reqParsed);
            } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
            }
 	    time_t expiredAt = responseNode->dateOfBirth + reqParsed->getMaxAge();
  	    Log << thisID << ": in cache, but expired at " << getUTC(expiredAt) << "because request max-age=" << reqParsed->getMaxAge() << std::endl;

            // ID: in cache, but expired at responseNode->dateOfBirth + requestParsed->getMaxAge();
        } else if (status == 3) {
            // request revalidation because of request
            try{
                validate(cacheProxy, keyFirstLine, server_connection_fd, client_connection_fd, reqParsed);
            } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
            }
  	    Log << thisID << ": in cache, but requires revalidation" << std::endl;

            // ID: in cache, but requires revalidation
        } else if (status == 4){
     	    // Expire because of "Expires"
	    try{
	        validate(cacheProxy, keyFirstLine, server_connection_fd, client_connection_fd, reqParsed);
	        } catch (errorException & e) {
	        std::cout << e.what() <<std::endl;
            }
  
	    Log << thisID << ": in cache, but expired at " << responseNode->value->header["Expires"] << std::endl;

            // ID: in cache, but expired at "Expires"
	    }
    } else {
        // What the client is looking for is NOT in the cache.
        std::cout<<"What the client is looking for is NOT in the cache."<<std::endl;
	Log << thisID << ": not in cache" << std::endl;
        if(send(server_connection_fd, reqParsed->getRequest().c_str(), reqParsed->getLength(), 0) <= 0) {
            Bad_Request (server_connection_fd, client_connection_fd);
        }
        std::cout<<"Request sent."<<std::endl;
        std::vector<char> newResponse;
	newResponse.resize(SIZE_OF_DATA_CHUNK);
	int currentSize = SIZE_OF_DATA_CHUNK;
        // Get a new response
        int currentRecv = recv(server_connection_fd, &newResponse.data()[0], SIZE_OF_DATA_CHUNK, 0);
        int index = currentRecv;
        if (index <= 0) {
            Bad_Gateway (server_connection_fd, client_connection_fd);
        }
	
	Log << thisID << ": Requesting \"" << reqParsed->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;
	
        std::cout<<"Response get."<<std::endl;
        std::string resString = newResponse.data();
	//std::cout<<"\nresponseString:"<<std::endl;
	//std::cout<<resString<<std::endl;
        responseParsed *newRes = new responseParsed();
	std::cout<<"trying to initialize newRes."<<std::endl;
	newRes->initialize(resString);

	    Log << thisID << ": Received \"" << newRes->getFirstLine() <<"\" from " << reqParsed->getHostname() << std::endl;
	    if ( newRes->header.count("Expires") != 0) {
	      Log << thisID << ": NOTE Expires: " << newRes->header["Expires"] << std::endl;
	    }
	    if (newRes->getMaxAge() > 0){
	      Log << thisID << ": NOTE max-age=" << newRes->getMaxAge() << std::endl;
	    }
	    if (newRes->header.count("ETag") != 0){
	      Log << thisID << ": NOTE ETag: " << newRes->header["ETag"] << std::endl;
	    }
	    if (newRes->header.count("Last-Modified") != 0){
	      Log << thisID << ": NOTE Last-Modified: " << newRes->header["Last-Modified"] << std::endl;
	    }
	    
	    if(newRes->header["Transfer-Encoding"] == "chunked") {
            if(send(client_connection_fd, &newResponse.data()[0],currentRecv, 0) <= 0) {
                Bad_Gateway (server_connection_fd, client_connection_fd);
            }
	        while(1) {
                std::vector<char> buffer;
                buffer.resize(SIZE_OF_DATA_CHUNK);
                int currentRes = recv(server_connection_fd, &buffer.data()[0], SIZE_OF_DATA_CHUNK, 0);
                if (currentRes <= 0) {
                    Bad_Gateway (server_connection_fd, client_connection_fd);
                }
                if(send(client_connection_fd, &buffer.data()[0], currentRes, 0) <= 0) {
                    Bad_Gateway (server_connection_fd, client_connection_fd);
                }
                if(currentRes < 5 || (buffer.data()[0] == '0' && buffer.data()[1] == '\r' && buffer.data()[2] == '\n')) {
                    break;
                }
	        }
	    } else {
	    //not chunked
            while ( index - newRes->getHeaderLength() < newRes->getContentLength()) {
                newResponse.resize(index + SIZE_OF_DATA_CHUNK);
                currentRecv = recv(server_connection_fd, &newResponse.data()[index], SIZE_OF_DATA_CHUNK, 0);
                if (currentRecv <= 0) {
  		            std::cerr << "Error: cannot receive data" << std::endl;
			    close(server_connection_fd);
                    throw errorException();
                }
                index += currentRecv;
            }
            try{
                cacheProcess(cacheProxy, keyFirstLine, client_connection_fd, reqParsed, newRes, &newResponse, index);
            } catch (errorException & e) {
                std::cout << e.what() <<std::endl;
            }
	    }
    }
}
#endif //HW2_GET_PROCESSING_H
