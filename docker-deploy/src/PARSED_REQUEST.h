// Created by shihe on 2/15/2020.
//

#ifndef HW2_PARSED_REQUEST_H
#define HW2_PARSED_REQUEST_H
#include <cstring>
#include <string>
#include <map>
#include <set>
#include <iostream>

class requestParsed {
public:
    void initialize(std::string req);
    std::string getMethod();
    std::string getUrlAddr();
    std::string getProtocal();
    std::string getHostname();
    std::string getPort();
    std::string getBody();
    int getLength();
    std::string reconstruct();
    std::map<std::string, std::string> header;
    std::set<std::string> getCacheControl();
    std::string getFirstLine();
    std::string getRequest();
    int getMaxAge();
    requestParsed(){
        maxAge = -1;
        bodyLength = -1;
        contentLength = -1;
    }
    ~requestParsed(){
        header.clear();
    }
    bool bodyLengthMatch();
    int getHeaderLength();
    int getBodyLength();
    int getContentLength();

private:
    int length;
    int parseAge(std::string ageStr);
    std::string request;
    std::string firstLine;
    std::string body;
    std::string method;
    std::string urlAddr;
    std::string protocal;
    std::string hostname;
    std::string port;
    std::set<std::string> cacheControl;
    int maxAge;
    int contentLength;
    int bodyLength;
    int headerLength;
};

int requestParsed::parseAge(std::string ageStr) {
    std::string strAge = ageStr.substr(8, ageStr.length() - 8);
    return atoi(strAge.c_str());
}

std::string requestParsed::reconstruct() {
    std::string reconstructHeader = firstLine;
    std::map<std::string, std::string>::iterator it;
    for ( it = header.begin(); it != header.end(); it++ )
    {
        reconstructHeader.append(it->first);
        reconstructHeader.append(": ");
        reconstructHeader.append(it->second);
        reconstructHeader.append("\r\n");
    }
    reconstructHeader.append("\r\n");
    return reconstructHeader;
}

void requestParsed::initialize(std::string req) {
    length = req.length();
    request = req;
    int lineEnd = req.find("\r\n");
    firstLine = req.substr(0, lineEnd);
    int space1 = req.find(" ");
    method = firstLine.substr(0, space1);
    int space2 = req.find(" ", space1 + 1);
    urlAddr = firstLine.substr(space1 + 1, space2 - space1 - 1);
    protocal = firstLine.substr(space2 + 1,lineEnd - space2 - 1);
    while(1){
        int previousLineEnd = lineEnd;
        lineEnd = req.find("\r\n", previousLineEnd + 1);
        if (lineEnd <= previousLineEnd + 2) {
            break;
        }
        std::string currentLine = req.substr(previousLineEnd + 2, lineEnd - previousLineEnd - 2);
        int colon = currentLine.find(": ");
        std::string key = currentLine.substr(0, colon);
        std::string value = currentLine.substr(colon + 2, lineEnd - colon - 2);
        header[key] = value;
    }
    headerLength = lineEnd + 2;
    if(header.count("Cache-Control") != 0) {
        std::string controlLine = header["Cache-Control"];
        while(1) {
            int comma = controlLine.find(", ");
            if(comma == -1) {
                if(controlLine.substr(0, 8) == "max-age=") {
                    maxAge = parseAge(controlLine);
                }
                cacheControl.insert(controlLine);
                break;
            } else {
                std::string value = controlLine.substr(0, comma);
                if(value.substr(0, 8) == "max-age=") {
                    maxAge = parseAge(value);
                }
                cacheControl.insert(value);
                controlLine = controlLine.substr(comma + 2, controlLine.length() - comma - 2);
            }
        }
    }
    
    if (method == "POST"){
        if(header.find("Content-Length") != header.end()) {
            contentLength = atoi(header["Content-Length"].c_str());
        }
        if(lineEnd + 2 <= req.length() - 1) {
            body = req.substr(lineEnd + 2, req.length() - lineEnd - 2);
            bodyLength = body.length();
        }
        else {
            body = "";
            bodyLength = 0;
        }
    }
    
    // Exception needed
    std::string host_info = header["Host"];
    int colon = host_info.find(":");
    std::cout<< "Colon: " <<colon<< "; Done!" <<std::endl;
    hostname = host_info.substr(0, colon);
    if(colon == std::string::npos) {
        hostname = host_info;
        if(protocal.substr(0, 5) == "HTTPS") {
            port = "433";
        } else if (protocal.substr(0, 4) == "HTTP") {
            port = "80";
        } else {
            // raise error
        }
    } else {
        hostname = host_info.substr(0, colon);
        port = host_info.substr(colon + 1, host_info.length() - 1 - colon);
    }
}

int requestParsed::getHeaderLength() {
    return headerLength;
}

int requestParsed::getBodyLength() {
    return bodyLength;
}

int requestParsed::getContentLength() {
    return contentLength;
}

bool requestParsed::bodyLengthMatch() {
    return bodyLength == contentLength;
}

std::string requestParsed::getMethod() {
    std::cout<< "Method: " <<method<< "; Done!" <<std::endl;
    return method;
}

std::string requestParsed::getUrlAddr() {
    return urlAddr;
}

std::string requestParsed::getFirstLine() {
    return firstLine;
}

std::string requestParsed::getProtocal() {
    return protocal;
}

std::string requestParsed::getHostname() {
    std::cout<< "Host: " <<hostname<< "; Done!" <<std::endl;
    return hostname;
}

std::string requestParsed::getPort() {
    std::cout<< "Port: " <<port<< "; Done!" <<std::endl;
    return port;
}

std::string requestParsed::getBody() {
    return body;
}


std::set<std::string> requestParsed::getCacheControl(){
    std::cout<< "Cache-Control: ";
    for (std::set<std::string>::iterator it=cacheControl.begin(); it!=cacheControl.end(); ++it){
        std::cout << *it<< " " ;
    }
    std::cout << '\n';
    return cacheControl;
}

int requestParsed::getLength() {
    return length;
}

int requestParsed::getMaxAge(){
    return maxAge;
}

std::string  requestParsed::getRequest() {
    return request;
}

#endif //HW2_PARSED_REQUEST_H
