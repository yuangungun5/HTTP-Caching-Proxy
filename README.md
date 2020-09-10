# README

## HOW TO RUN PROGRAM IN DOCKER

1. git clone https://gitlab.oit.duke.edu/qx37/erss-hw2-qx37-sl563.git 
The repository tree should look like the structure below:  
docker-deploy  
   |—docker-compose.yml  
   |—src  
        |—Dockerfile  
        |—other code files and header files  

2. cd ECE568_HW2_sl563_qx37/ docker-deploy/
3. sudo docker-compose up
4. a user should configure the browser (Firefox is recommended) before using this web proxy
5. proxy working history is preserved in the directory /var/log/erss/proxy.log


## SOME INITIAL SET UP

A socket listen would be set up to monitoring the port. (see 'main' function in main.cpp and 'initiateSocketAsServer' and 'bindListen' function in SOCKET_PROXY.h for detailed information)
Every time a client is listened, the a socket connected to the client side would be created, and passed to a new thread with 'processing' function in main.cpp as the handler. (see 'main' function in main.cpp and 'acceptFromClient' and function in SOCKET_PROXY.h for detailed information)
Each thread will be assigned with an ID, starting from 0, and add one whenever a new thread is detached. The ID is used for log.
Whenever there's something wrong happened during the socket setting and listening, an error exception would be thrown and handled.

Inside the 'processing' function, the header of the request given by the client would be parsed, and based on the "Host" and "Port" information in the header, a socket to the server would be created. (see 'processing' function in main.cpp and 'initiateSocketAsClient' and 'connectToServer' function in SOCKET_PROXY.h for detailed information)
Based on the Method parsed, different functions would be called to handle that request.
Whenever there's something wrong happened during the request receiving and socket setting process, an error exception would be thrown and handled.


## CONNECT METHOD

If a "CONNECT" method is parsed, the processing function would call connectProcessing function to handle the request.
The connectProcessing function would send a "200 OK" message to the client and build a tunneled between the sockets connected to the client side and the server side. 
Whenever information is received from one side, it would be passed to the other side using 'tunnel' function, when no information is received from client, the tunnel would be closed by return the 'connectProcessing' Function. 
If there is an error when selecting the socket_fd or transfer the data, the tunnel would be closed and an error exception would be thrown and handled (take a look at the main.cpp for detail).
Actions would be noted down in proxy.log required by homework 2, you can take a look on proxy.log anytime after you finished some test. 
To test CONNECT method, you may consider the following test cases:
https://www.bilibili.com/
https://www.youtube.com/
https://github.com/
https://www.google.com/

## GET METHOD

If a "GET" method is parsed, the processing function would call getProcessing function to handle the request  (SEE GET_PROCESSING.h for detail).
The function would first see if what the client is looking for is in the cache by searching in the cache using the request first line as the key words.

If not, the proxy would forward the request to the server and ask for a new response.
Depending on whether the response is chunked or not, the following action would be different.
The chunked data would NOT be stored in the cache, and it would be received and sent to client chunk-by-chunk until "0\r\n" is received.
If a content-length is given in the response header, we will keep track of body length while receiving from server until the body length meets the content length.
Whether this response would be stored in cache or not depending on the following rules:
1. If the size of the response data is larger than 65kB (not crazily large though), it will NOT be stored in cache.
2. If there is NO 'Cache-Control' in the response header:
	2.1 If 'Expires' is specified, the response would be stored, and the expiration time is defined depending on the header line 'Expires',
	2.2 otherwise, the response would NOT be stored in cache.
3. If 'no-store' or 'private' is specified in 'Cache-Control', the response would NOT be stored in cache.
4. If 'no-cache', 'max-age=0', and 'must-revalidate' is specified in 'Cache-Control', the response would be stored in cache, but revalidation is required for future visit.
5. If 'max-age = #' where # > 0, the response would be stored, and the expiration time is defined depending on the max-age.
See detail in the cacheProcessing function in GET_PROCESSING.h and CACHE_PROXY.h.
****Mutex is used to protect the store_data function, which means that the storage action can only be done one by one.
****Because of memory consideration, our cache can only store 1024 pieces of response, which is not crazily large, but is enough for many usage. The cached responses are ordered by the time of last update or storage, and if the cache is full, to add a new response, the oldest response would be deleted from the cache.

If what the request is looking for is in the cache, the proxy would first check the status of the response in the cache.
Also, the proxy would also check the requirement given by the request.
The rule of defining status from request is similar to the rules discussed before, except that there will be no 'private' case in the Cache-Control given by the request.
See GET_PROCESSING.h for detail.
If the response in the cache is valid according to BOTH request and cached response, the response would be sent to client directly from the cache. 
If either request or cached response indicates that the cached response is expired or the client does not want anything from cache, a new response would be asked for from server, and whether the new response would be stored depends on the rules discussed above.
If either request or cached response requires re-validations, the revalidation process would be gotten through via the validation function:
	You can look at 'validation' function in GET_PROCESSING.h for detail.
	In short, if 'Last-Modified' or 'ETag' has been specified in the header, the request would be reconstructed by adding "If-Modified-Since" or "If-None-Match" lines into the header and sent to the server.
	If a 304 response is gotten, the revalidation is passed and the response in the cache would be directly sent to client, otherwise a new response would be asked for and whether the new response would be stored in cache has been discussed before.
Whenever there's something wrong happened during the receive or send process (e.g. nothing is received while the body length has not meet the content length), an error would be thrown, see GET_PROCESSING.h and 'badRequest' and 'badGateway' function in SOCKET_PROXY.h for detail.
Actions would be noted down in proxy.log required by homework 2, you can take a look on proxy.log anytime after you finished some test. 

To test GET method, you may consider the following test cases:
general test:
http://people.duke.edu/~bmr23/ece568/class.html

chunked data:
http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx

large-size data:
http://people.duke.edu/~bmr23/ece568/slides/ERSS_01_Servers_0.pdf

cache: 
http://wanglab.berkeley.edu/
http://gr.xjtu.edu.cn/web/guest;jsessionid=EB49ACCAAC7C2735EDA9E4B5260667D5

puppy:
http://people.duke.edu/~tkb13/images/reg-chair.jpg

****Pay attention, Firefox browser has its own cache, so when loading the same page multiple times, it would be beneficial to clear the cache and cookies of local browser each time. Otherwise, the the client may read cache from both local browser and our proxy when reading a cached website, which may lead to confusion.

## POST METHOD

If a "POST" method is parsed, the processing function would call postProcessing function to handle the request (SEE POST_PROCESSING.h for detail).
Inside the post function, the body length of the request is kept tracked of and request would be continuously received in a while loop until the Content-Length meet the body length.
If anything wrong happens in this process (e.g. no information received before body length reaches Content-Length), an error exception would be thrown and handled (see POSTPROCESSING.h and badRequest function in SOCKET_PROXY.h for detail).

After that, the request would be forwarded to the server, and a response would be received from the server.
If the response is chunked, the response would be sent to client chunk by chunk until "0\r\n" is received.
If the response is not chunked, the response would be received using a while loop until the body length meets the Content-Length, and then sent to the client.
Whenever there's something wrong happened during the receive or send process (e.g. nothing is received while the body length has not meet the content length), an error would be thrown, see GET_PROCESSING.h and 'badRequest' and 'badGateway' function in SOCKET_PROXY.h for detail.

To test POST method, you may consider the following test case:
http://httpbin.org/forms/post