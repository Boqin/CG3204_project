/* Server for search engine
 * Module: CG3204L
 * Author: Wan Wenli, Qiu Boqin, ...
 * Last updated: 9:09PM 27 Mar 2013
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
#include <dirent.h>

#define PORT 5000
#define DB_DIR "./database/"
#define URL_DIR "./database/unquested_urls.txt"

using namespace std;

// global variables
list<string> quested_urls;
list<string> unquested_urls;
pthread_mutex_t lock;

// functions
bool is_string_in_list(list<string> l, string s);
void* clientThread(void *args);
void print_list(list<string> l);
void print_list_to_file(list<string> l, string dir);
void load_quested_urls(string dir);
void load_unquested_urls(string dir);
bool write_crawler(int sock, string url);
bool read_crawler(int sock, string fn);

int main (int argc , char **argv)
{
    // Port to listen on
    uint16_t port = PORT;

	if (argc != 2)
	{
		cerr << "Please enter a key word." << endl;
		exit(-1);
	}

	string keywd = argv[1];

    // Create a TCP socket
    int socketFD;
    if ( ( socketFD = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 )
	{
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
                sizeof ( struct sockaddr_in ) ) < 0 )
	{
        std :: cerr << "Error on bind()\n";
        close ( socketFD );
        return -1;
    }

    // Tell the OS that the server wants to listen on this socket
    //  with max number of pending TCP connections = 10
    if ( listen ( socketFD , 10 ) < 0 )
	{
        std :: cerr << "Error on listen()\n";
        close ( socketFD );
        return -1;
    }

    // Wait for connections
    cout << "Echo Server Running on Port " << port << endl;
    
	int newSocketFD;
	// get the last crawled URL
	string fn = "www.nus.edu.sg";
	cout << "Starts from URL: " << fn << endl;
	load_quested_urls(DB_DIR);
	load_unquested_urls(URL_DIR);
	// print_list(quested_urls);
	unquested_urls.push_back(fn);

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
        // Also, tell the thread the socket FD it should use for this connection
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
	while (!unquested_urls.empty())
	{
		pthread_mutex_lock(&lock);

		print_list_to_file(unquested_urls, URL_DIR);
		string url = unquested_urls.front();
		unquested_urls.pop_front();

		// write to crawler
		write_crawler(socketFD, url);
		// debug
		cout << "\nURL sent to crawler: " << url << endl;

		quested_urls.push_back(url);

		// read response from crawler
		std::replace(url.begin(), url.end(), '/', '*');
		read_crawler(socketFD, url);

		pthread_mutex_unlock(&lock);
	}
	cout << "Queue empty. Thread closing..." << endl;
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

// for debugging purpose
void print_list(list<string> l)
{
	if (l.empty())
	{
		cout << "empty list" << endl;
	}
	else
	{
		list<string>::iterator iter;
		for (iter = l.begin(); iter != l.end(); iter++)
		{
			cout << *iter << endl;
		}
	}
}

// print at most 10 URLs from unquested_urls into database
void print_list_to_file(list<string> l, string dir)
{
	ofstream f(dir.c_str());
	if(f.is_open())
	{
		list<string>::iterator iter;
		int count;
		for (iter = l.begin(); iter != l.end(); iter++)
		{
			f << *iter << endl;
			if (count == 9)
			{
				f.close();
				return;
			}
			count++;
		}
	}
	else
	{
		cerr << "Cannot open file" << dir << endl;
	}
	f.close();
	return;
}

// load unquested_urls from DB based on previous crawling
void load_unquested_urls(string dir)
{
	ifstream f(dir.c_str());
	string line;
	while(std::getline(f, line))
	{
		unquested_urls.push_back(line);
	}
	return;
}

// load quested_urls from DB based on previous crawling
void load_quested_urls(string dir)
{
	string fn;
    DIR *dp;
	struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
	{
        cout << "Error in opening " << dir << endl;
		return;
    }

    while ((dirp = readdir(dp)) != NULL)
	{
        fn = string(dirp->d_name);
		std::replace(fn.begin(), fn.end(), '*', '/');
		quested_urls.push_back(fn);
    }

    closedir(dp);
	return;
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
	char ch;
	string url;
	string url_list;
	string http_reply;
	string reply_dir;
	unsigned int t_start = 0, t_end = 0;

	/*
	 * Firstly receive the HTTP reply from crawler
	 */
	int bytes_received = 0;
	while(bytes_received <= 0)
	{
		bytes_received = recv(sock, &net_len, 4, 0);
	}
	content_len = ntohl(net_len);

	// debug
	// cout << "content_len: " << content_len << endl;

	// if length is 0, means there is no http reply
	// invalid URL
	if (content_len == 0)
		return false;
	else
	{
		// receive http_reply
		for(int i = 0; i < content_len; i++)
		{
			recv(sock, &ch, 1, 0);
			http_reply += ch;
		}

		// debug
		cout << "\nHTTP reply length: " << http_reply.length() << endl;

		// create a new file to store the HTTP reply
		reply_dir = DB_DIR + fn; // generate reply dir

		ofstream reply_file(reply_dir.c_str());
		if(reply_file.is_open())
		{
			reply_file << http_reply;
			reply_file.close();
			//debug
			cout << "HTTP reply saved in " << reply_dir << endl; } else {
			cerr << "\nUnable to open file." << endl;
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
		// receive url_list
		for(int i = 0; i < content_len; i++)
		{
			recv(sock, &ch, 1, 0);
			url_list += ch;
		}

		// debug
		// cout << "content_len: " << content_len << endl;
		// cout << "url_list: " << url_list << endl;

		t_end = url_list.find_first_of(";");

		// debug
		// cout << "\nReplied URLs:" << endl;

		while(t_end != string::npos && t_end < url_list.length())
		{
			// push a URL to the end of list
			url = url_list.substr(t_start, t_end - t_start);

			// debug
			// cout << url << endl;

			// if the url has never been crawled before
			if (is_string_in_list(quested_urls, url) == false &&
				is_string_in_list(unquested_urls, url) == false)
			{
				// insert the URL into the unquested_url
				unquested_urls.push_back(url);
			}

			t_start = t_end + 1;
			t_end = url_list.find_first_of(";", t_start);
		}
		cout << "\nUnquested urls count: " << unquested_urls.size()
			<< endl;
		cout << "Quested urls count: " << quested_urls.size()
			<< endl;
		return true;
	}
}
