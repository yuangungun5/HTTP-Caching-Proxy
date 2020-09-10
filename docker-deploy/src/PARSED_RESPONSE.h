//
// Created by shihe on 2/22/2020.
//


#ifndef HW2_PARSED_RESPONSE_H
#define HW2_PARSED_RESPONSE_H
#include <cstring>
#include <string>
#include <map>
#include <set>
#include <iostream>


class responseParsed {
public:
    void initialize(std::string res);
    std::map<std::string, std::string> header;
    std::set<std::string> getCacheControl();
    std::string getFirstLine();
    std::string getCode();
    std::string getResponse();
    int getMaxAge();
    int getLength();
    int getHeaderLength();
    bool bodyLengthMatch();
    int getBodyLength();
    int getContentLength();
    std::string reconstruct();
    std::string getBody();
    responseParsed(){
      maxAge = -1;
      contentLength = -1;
      bodyLength = -1;
    }
    ~responseParsed(){
        header.clear();
    }


private:
    int length;
    std::string code;
    int parseAge(std::string ageStr);
    std::set<std::string> cacheControl;
    int maxAge;
    int contentLength;
    int bodyLength;
    int headerLength;
    std::string response;
    std::string body;
    std::string firstLine;
};

int responseParsed::parseAge(std::string ageStr) {
    std::string strAge = ageStr.substr(8, ageStr.length() - 8);
    return atoi(strAge.c_str());
}

std::string responseParsed::getCode() {
    return code;
}

int responseParsed::getMaxAge(){
    return maxAge;
}

std::string responseParsed::reconstruct() {
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

std::string responseParsed::getBody(){
    return body;
}

void responseParsed::initialize(std::string res) {
    std::cout<< "start initialize response" <<std::endl;
    std::cout<< "res length: " << res.length() <<std::endl;
    length = res.length();
    response = res;
    // std::cout<<res<<std::endl;    
    int lineEnd = res.find("\r\n");
    std::cout<<"LineEnd: "<< lineEnd << std::endl;
    std::cout<<"Parsing first line."<<std::endl;
    firstLine = (std::string) res.substr(0, lineEnd);
    std::cout<<"Parsing code."<<std::endl;
    int space = firstLine.find(" ");
    code = firstLine.substr(space + 1, 3);
    std::cout<<"Parsing body."<<std::endl;
    while(1){
        int previousLineEnd = lineEnd;
        lineEnd = res.find("\r\n", previousLineEnd + 1);
        if (lineEnd <= previousLineEnd + 2) {
            break;
        }
        std::string currentLine = res.substr(previousLineEnd + 2, lineEnd - previousLineEnd - 2);
        int colon = currentLine.find(": ");
        std::string key = currentLine.substr(0, colon);
        std::string value = currentLine.substr(colon + 2, lineEnd - colon - 2);
        header[key] = value;
	    std::cout<<"Header key: "<< key << std::endl;
	    std::cout<<"Header value: "<< value << std::endl;
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
    std::cout<<"max-age read from Cache-Control: "<<maxAge<<std::endl;
    if(header.find("Content-Length") != header.end()) {
            contentLength = atoi(header["Content-Length"].c_str());
    }
    
    if (res.length() > lineEnd + 2){
        
        body = res.substr(lineEnd + 2, res.length() - lineEnd - 2);
	std::cout<< "body Length: " << body.length() << std::endl;
        bodyLength = body.length();
    }
}

std::string responseParsed::getFirstLine() {
    return firstLine;
}

std::set<std::string> responseParsed::getCacheControl(){
    std::cout<< "Cache-Control: ";
    for (std::set<std::string>::iterator it=cacheControl.begin(); it!=cacheControl.end(); ++it){
        std::cout << *it<< " " ;
    }
    std::cout << '\n';
    return cacheControl;
}

int responseParsed::getLength() {
    return length;
}

std::string responseParsed::getResponse() {
    return response;
}

int responseParsed::getHeaderLength() {
    return headerLength;
}

int responseParsed::getBodyLength() {
    return bodyLength;
}

bool responseParsed::bodyLengthMatch() {
    return bodyLength == contentLength;
}

int responseParsed::getContentLength() {
    return contentLength;
}

#endif //HW2_PARSED_RESPONSE_H
