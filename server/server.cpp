/* Server for search engine
 * Module: CG3204L
 * Author: Wan Wenli, Qiu Boqin, ...
 * Last updated: 3:11PM 27 Mar 2013
 */
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <list>
#include <cstdlib>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <algorithm>

#define PORT 5000
#define DB_DIR "/tmp/database/"

using namespace std;

// global variables
list<string> quested_urls;
list<string> unquested_urls;
pthread_mutex_t lock;

// functions
bool is_string_in_list(list<string> l, string s);
bool write_crawler(int sock, string url);
bool read_crawler(int sock, string fn);
void* clientThread(void *args);

int main (int argc , char **argv)
{
    // Port to listen on
    uint16_t port = PORT;

    // Create a TCP socket
    int socketFD;
    if ( ( socketFD = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 ) {
        std :: cerr << "Error creating Server socket\n";
        return -1;
    }

    // Bind to all IP Addresses of this machine
    struct sockaddr_in serverAddress;
    memset ( &serverAddress , 0 , sizeof ( struct sockaddr_in ) );
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons ( port );
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    if ( bind ( socketFD , (const struct sockaddr*) &serverAddress ,
                sizeof ( struct sockaddr_in ) ) < 0 ) {
        std :: cerr << "Error on bind()\n";
        close ( socketFD );
        return -1;
    }

    // Tell the OS that the server wants to listen on this socket
    //  with max number of pending TCP connections = 10
    if ( listen ( socketFD , 10 ) < 0 ) {
        std :: cerr << "Error on listen()\n";
        close ( socketFD );
        return -1;
    }

    // Wait for connections
    cout << "Echo Server Running on Port " << port
                << std :: endl;
	read_crawler(1, "www.google.com.sg");
    int newSocketFD;
    while (true)
	{
        if ( ( newSocketFD = accept ( socketFD , NULL , NULL ) ) < 0 )
		{
            std :: cerr << "Error on accept()\n";
            close ( socketFD );
            return -1;
        }
		else
		{
			cout << "New connection established." << endl;
		}

        // On a new connection, create a new thread
        //  Also, tell the thread the socket FD it should use for this connection
        pthread_t threadID;
        int *clientSocketFD = new int;
        *clientSocketFD = newSocketFD;
        if ( clientSocketFD == NULL )
		{
            std :: cerr << "Out of heap memory\n";
            close ( socketFD );
            return -1;
        }
        if ( pthread_create ( &threadID , NULL , clientThread , clientSocketFD ) != 0 )
		{
            std :: cerr << "Error on pthread_create()\n";
            close ( socketFD );
            return -1;
        }
    }
    return 0;
}

// Thread handling one particular client
void* clientThread (void *args)
{
    // Get the socketFD this thread has been assigned to
    int *clientSocketFD = (int*) args;
    int socketFD = *clientSocketFD;
    delete clientSocketFD;

	// get the URL from queue to crawl
	pthread_mutex_lock(&lock);
	string url = unquested_urls.front();
	unquested_urls.pop_front();
	quested_urls.push_back(url);
	pthread_mutex_unlock(&lock);

	// write to crawler
	write_crawler(socketFD, url);

	//
	std::replace(url.begin(), url.end(), '/', '*');
	read_crawler(socketFD, url);

    close (socketFD);
	pthread_exit(NULL);
    return NULL;
}

bool is_string_in_list(list<string> l, string s)
{
    if (l.empty())
		return false;
    else
    {
        list<string>::iterator iter;
        for (iter = l.begin(); iter != l.end(); iter++)
        {
            if(s == *iter)
                return true;
        }
        return false;
	}
}

bool write_crawler(int sock, string url)
{
	int content_len = url.length();
	int net_len = htonl(content_len);

	send(sock, (const char*)&net_len, 4, 0);
	send(sock, url.c_str(), content_len, 0);
	return true;
}

bool read_crawler(int sock, string fn)
{
	int net_len;
	int content_len;
	string url;
	string url_list;
	string http_reply;
	string reply_dir;
	unsigned int t_start = 0, t_end = 0;

	/*
	 * Firstly receive the HTTP reply from crawler
	 */
	recv(sock, &net_len, 4, 0);
	content_len = ntohl(net_len);

	// if length is 0, means there is no http reply
	// invalid URL
	if (content_len == 0)
		return false;
	else
	{
		recv(sock, &http_reply, content_len, 0);
		// create a new file to store the HTTP reply
		reply_dir = fn + DB_DIR;
		ofstream reply_file(reply_dir.c_str());
		if(reply_file.is_open())
		{
			reply_file << http_reply;
			reply_file.close();
		}
		else
		{
			cerr << "Unable to open file." << endl;
		}
	}

	/*
	 * Next receive the list of URL
	 */
	recv(sock, &net_len, 4, 0);
	content_len = ntohl(net_len);

	if (content_len == 0)
		return false;
	else
	{
		// recv(sock, &url_list, content_len, 0);
		t_end = url_list.find_first_of(";");
		while(t_end != string::npos && t_end < url_list.length())
		{
			// push a URL to the end of list
			pthread_mutex_lock(&lock);
			url = url_list.substr(t_start, t_end - t_start);
			pthread_mutex_unlock(&lock);

			cout << url << endl;
			unquested_urls.push_back(url);
			// insert the URL into the unquested_url
			t_start = t_end + 1;
			t_end = url_list.find_first_of(";", t_start);
		}
	}
}
