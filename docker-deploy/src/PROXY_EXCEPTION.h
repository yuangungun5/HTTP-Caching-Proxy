
#ifndef HW2_PROXY_EXCEPTION_H
#define HW2_PROXY_EXCEPTION_H

#include <iostream>
#include <string>

class errorException: public std::exception {
 public:
  virtual const char * what() const throw() {
    return "exception throws";
  }
  
};

#endif 
