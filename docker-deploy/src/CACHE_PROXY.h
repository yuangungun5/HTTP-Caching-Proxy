#ifndef HW2_CACHE_PROXY_H
#define HW2_CACHE_PROXY_H

#include <vector>
#include <ctime>
#include <map>
#include <cstring>
#include <string>
#include "PARSED_RESPONSE.h"
#include "SOCKET_PROXY.h"
#include <thread>         // std::thread
#include <mutex>          // std::mutex
#define SIZE_OF_CACHE 1024

std::mutex mtx;           // mutex for critical section

class node {
 public:
  responseParsed * value;
  std::string key;
  time_t dateOfBirth; // UTC time
  std::vector<char> response;
  int index;
  node *prev;
  node *next;
  node() {
    prev = NULL;
    next = NULL;
  }
 node(std::string k, responseParsed * v, std::vector<char> r, int i): key(k), value(v), response(r), index(i), prev(NULL), next(NULL), dateOfBirth(){}
 node(std::string k, responseParsed * v, std::vector<char> r, int i,  node *p, node *n, time_t currTime): key(k), value(v), response(r), index(i), prev(p), next(n), dateOfBirth(currTime) {}
  ~node(){
    delete value;
  }

  std::vector<char> getResponse() {
      return response;
  }

  int getIndex() {
      return index;
  }


  time_t getExpirationTime() {
    if(value->getMaxAge() != -1) {
      return dateOfBirth + value->getMaxAge();
    } else {
      return dateOfBirth;
    }
  }
  
  bool isExpired() {
    int age = time(0) - dateOfBirth;
    if(value->getMaxAge() == -1 || value->getMaxAge() < age) {
      return true;
    } else {
      return false;
    }
  }
};


class cache_proxy{
 private:
  int capacity;
  
 public:
    node *head;
    node *tail;
    std::map<std::string, node *> proxyMap; // change node to node *

  cache_proxy() {
    head = NULL;
    tail = NULL;
    capacity = SIZE_OF_CACHE;
    proxyMap.clear();
    std::cout<<"Cache Created!"<<std::endl;
  }
  
  ~cache_proxy() {
    while (head != NULL) {
      node *temp = head;
      head = head->next;
      delete temp;
    }
    proxyMap.clear();
  }

  void insertHead(std::string key, responseParsed * value, std::vector<char> r, int i);
  void removeTail();
  void moveToHead(std::string key);

  std::string get_data(std::string key);
  void store_data(std::string key, responseParsed * value, std::vector<char> * r, int i);
  
};

void cache_proxy::insertHead(std::string key, responseParsed * value, std::vector<char> r, int i) {
  std::cout<<"Trying to insert head to cache"<<std::endl;
  head = new node(key, value, r, i, NULL, head, time(0));
  if (tail == NULL) {
    tail = head;
  }
  else {
    head->next->prev = head;
  }
  proxyMap[key] = head;

  //std::cout<<"DATE OF BIRTH: "<<timeToString(head->dateOfBirth)<<std::endl; 
  std::cout<<"Head inserted!"<<std::endl;
}

void cache_proxy::removeTail() {
  node *temp = tail;
  proxyMap.erase(tail->key);
    delete tail->value;
  if (head == tail) {
    head = NULL;
    tail = NULL;
  }
  else {
    tail = tail->prev;
    tail->next = NULL;
  }
  delete temp;
}

void cache_proxy::moveToHead(std::string key) {
  node *currNode = proxyMap[key];
  if (currNode == head) {
      time_t t = time(0);
      head->dateOfBirth = t;
      return;
  }
  else if (currNode == tail) {
    tail = tail->prev;
    tail->next = NULL;
  }
  else {
    node *prevNode = currNode->prev;
    node *nextNode = currNode->next;
    prevNode->next = nextNode;
    nextNode->prev = prevNode;
  }

  currNode->next = head;
  head->prev = currNode;
  currNode->prev = NULL;
  head = currNode;
  time_t t = time(0);
  head->dateOfBirth = t;
}

std::string cache_proxy::get_data(std::string key) {
  std::cout<<"Trying to get data from cache"<<std::endl;
  if (proxyMap.count(key) == 0) {
    return NULL;
  }
  // in cache, change the priory and return data
  std::cout<<"Data get from cache"<<std::endl;
  return proxyMap[key]->value->getResponse();
}

void cache_proxy::store_data(std::string key, responseParsed * value, std::vector<char> * r, int i) {
  mtx.lock();
  std::cout<<"Trying to store a response in cache"<<std::endl;
  if (proxyMap.size() == capacity) {
    // cache is full
    removeTail();
    insertHead(key, value, *r, i);
    std::cout<<"A response is stored in cache"<<std::endl;
  }
  else {
    if (proxyMap.count(key) != 0) {
      // key already exists
      proxyMap[key]->value = value;
      proxyMap[key]->response = *r;
      proxyMap[key]->index = i;
      moveToHead(key);
      std::cout<<"A response is stored in cache"<<std::endl;
    }
    else {
      // key does not exist
      insertHead(key, value, *r, i);
      std::cout<<"A response is stored in cache"<<std::endl;
      std::cout<<"check the response just stored: "<<proxyMap[key]->value->getMaxAge()<<std::endl;
    }
  }
  mtx.unlock();
 
}

#endif //HW2_CACHE_PROXY_H
