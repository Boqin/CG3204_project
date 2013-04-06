#include <iostream>
#include <string>
#include <boost/regex.hpp>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <deque>
#include <fstream>
#include <pthread.h>

using namespace std;

string gethostaddr(string url);
string getHostUrl(string url);
string getLocationUrl(string url);
bool findValid(string toplevel);
void* spider_thread ( void *args );

deque <string> all_links;
string hostAddress;
string locationAddress;
int serverFD;

pthread_mutex_t lock;

int main ()
{
	int client_sock;
	struct sockaddr_in server_addr;
	struct hostent *server_host;
	string IP_Address;
	
	cout << "Please type server's IP Address: ";
	cin >> IP_Address;
	
    /// Create new TCP socket.
    if ( ( client_sock = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 ) 
    {
        cerr << "Error creating Server socket\n";
        return -1;
    }
	
	server_host = gethostbyname(IP_Address.c_str());
	
	memset( &server_addr , 0 , sizeof ( struct sockaddr_in ) );
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons ( 5000 );
    server_addr.sin_addr = *((struct in_addr *) server_host->h_addr);
    bzero(&(server_addr.sin_zero),8);
	
	/// Connect to server at port 5000
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) 
    {
        cerr << "Error connecting socket";
        return -1;
    }
	
	int net_len = 1000;
	serverFD = client_sock;
	
	cout << "START" << endl;
	
	while (recv( client_sock ,&net_len, 4, 0 )) 
	{
		int length;
		length = ntohl(net_len);
		
		if (length != 0)
		{
			cout << endl;
			cout << "Start Crawler" << endl;
			
			/// receive url string from server
			signed char ch;
			string url_from_server;
			
			for (int i = 0;  i < length; ++i) 
			{
				recv( client_sock , &ch, 1, 0 );
				url_from_server += ch;
			}
			
			// cout << url_from_server << endl;
			// cout << length << endl;
			
			string temp = gethostaddr(url_from_server);
			// cout << temp << endl;
			
			cout << endl;
			
			if (gethostbyname(getHostUrl(temp).c_str()) != NULL)
			{		
				/// divide url string into host and location url
				hostAddress = getHostUrl(temp);
				locationAddress = getLocationUrl(temp);
				
				pthread_t threadID;
				int sock;
				int *sockFD = new int;
				
				/// Create new TCP socket. 
				if ( ( sock = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 ) 
				{
					cerr << "Error creating Server socket\n";
					return -1;
				}
				
				*sockFD = sock;
				
				fflush(stdout);
				
				// /// Bind to IP Address of this machine on port 80.
				// struct sockaddr_in server;
				// memset ( &server , 0 , sizeof ( struct sockaddr_in ) );
				// server.sin_family = AF_INET;
				// server.sin_port = htons ( 80 );
				// server.sin_addr = *((struct in_addr *) HOST->h_addr);
				// bzero(&(server.sin_zero),8);
				
				// if (connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
				// {
					// cerr << "Error connecting port 80\n";
					// close ( sock );
				// }
				
				// /// HTTP request message
				// string send_data = "HEAD " + locationAddress + " HTTP/1.1\r\nHost: " + hostAddress + "\r\n\r\n";
				// cout << send_data << endl;
				
				// if (send ( sock , send_data.c_str() , send_data.length() , 0 ) < 0)
				// {
					// cerr << "Error on send HEAD packet\n";
					// close ( sock );
				// }
				
				// signed char ch;
				// string echo = "";
				
				// /// HTTP reply message
				// while ( recv( sock ,&ch, sizeof(ch), 0 ) == 1 ) 
				// {
					// echo += ch;
				// }
					
				// cout << echo << endl;
				
				if (pthread_create(&threadID, NULL, spider_thread, sockFD) != 0)
				{
					cerr << "Error on pthread_create()\n";
					close ( client_sock );
					return -1;
				}
				
			}
			else
			{
				/// Invalid url
			}
		}
		sleep(3);
	}
	
	while (1);
	
	close (client_sock);
    
	return 0;
}

string gethostaddr(string full_addr)
{
	string link = "";
    string http( full_addr.begin(), full_addr.begin()+7 );
	string https( full_addr.begin(), full_addr.begin()+8 );
	
    if (!http.compare("http://"))
    {
		/// there is http
        string host( full_addr.begin()+7, full_addr.end() );
        link = host;
    }
    else
    {
		/// there is no http
		link = full_addr;
	}
    
    return link;
}

/* This function takes in a URL address string and returns the host address URL. */
string getHostUrl(string full_addr)
{
    string link = "";
    size_t sp = full_addr.find_first_of( '/'); 
  
    if ( sp != string::npos ) 
    {
		string host( full_addr.begin(), full_addr.begin()+sp ); 
		string location( full_addr.begin()+sp, full_addr.end() );
        link = host;
    }
	else
	{
		link = full_addr;
	}
    
    return link;
}

/* This function takes in a URL address string and returns the location address URL. */
string getLocationUrl(string full_addr)
{
	string link = "";
    size_t sp = full_addr.find_first_of( '/');
	
	if ( sp != string::npos ) 
    {
		string location( full_addr.begin()+sp, full_addr.end() );
        link = location;
    }
	else
	{
		link = "/";
	}
	
	return link;
}

bool findValid(string temp)
{
	if (temp.compare("js") == 0)
	{
		return false;
	}
	
	if (temp.compare("dtd") == 0)
	{
		return false;
	}
	
	if (temp.compare("gif") == 0)
	{
		return false;
	}
	
	if (temp.compare("pdf") == 0)
	{
		return false;
	}
	
	if (temp.compare("jpg") == 0)
	{
		return false;
	}
	
	if (temp.compare("css") == 0)
	{
		return false;
	}
	
	if (temp.compare("ico") == 0)
	{
		return false;
	}
	
	return true;
	
}

/* Thread process */
void* spider_thread( void *args )
{
    struct sockaddr_in server;
    struct hostent *host_addr;
	
	int *clientSocketFD = (int*) args;
    int sockFD = *clientSocketFD;
    delete clientSocketFD;
	
    /// Bind to IP Address of this machine on port 80.
    host_addr = gethostbyname(hostAddress.c_str());
    
    memset ( &server , 0 , sizeof ( struct sockaddr_in ) );
    server.sin_family = AF_INET;
    server.sin_port = htons ( 80 );
    server.sin_addr = *((struct in_addr *) host_addr->h_addr);
    bzero(&(server.sin_zero),8);
	
	int conn = connect(sockFD, (struct sockaddr *)&server, sizeof(struct sockaddr));
	
	cout << "Connect: " << conn << endl;
	
    if (conn == -1) 
    {
		cerr << "Error connecting\n";
        close ( sockFD );
        pthread_exit(NULL);
    }
    
    /// HTTP request message
    string send_data = "GET " + locationAddress + " HTTP/1.1\r\nHost: " + hostAddress + "\r\n\r\n";
    cout << send_data << endl;
    
    if (send ( sockFD , send_data.c_str() , send_data.length() , 0 ) < 0)
    {
        cerr << "Error on send()\n";
        close ( sockFD );
        pthread_exit(NULL);
    }
	
	cout << "send" << endl;
	
    signed char ch;
    string echo = "";
    
    /// HTTP reply message
    while ( recv( sockFD ,&ch, sizeof(ch), 0 ) == 1 ) 
    {
        echo += ch;
    }
	
	cout << "Received HTTP reply message" << endl;
	
	/// HTTP html body
	unsigned pos = echo.find("\r\n\r\n");
	string httpbody = echo.substr (pos+4);
	
	cout << "Editing" << endl;
	
	unsigned newpos = httpbody.find_first_of("\r\n");
	string body = httpbody.substr(newpos+2);
	
	cout << "Edited" << endl;
	
	//cout << body << endl;
	
	deque <string> temp_links;
	
    /// Extract all URLs from the HTTP response 
    string regex_pattern = "(http://)([a-zA-Z0-9]+\.[a-zA-Z0-9\-]+|[a-zA-Z0-9\-]+)\.[a-zA-Z\.]{2,25}(/[a-zA-Z0-9\.\?=/#%&\+-]+|/|)";
    boost::cmatch match;
    boost::regex rgx (regex_pattern);
    
    while (boost::regex_search (echo.c_str(),match,rgx))
    {
        string host_url = getHostUrl( match.str() );
		string location_url = getLocationUrl( match.str() );
       
		string url = gethostaddr(host_url + location_url);
		
		unsigned found_1 = url.find_last_of("/\\");
		unsigned found_2 = (url.substr(found_1 + 1)).find_last_of(".");
		
		string tld = (url.substr(found_1 + 1)).substr(found_2 + 1);
		
		if (!findValid(tld))
		{
			echo = match.suffix().str();		
			continue;
		}
		
        pthread_mutex_lock(&lock);
        
		bool exist = false;
        for (deque<string>::iterator it = all_links.begin(); it!=all_links.end(); ++it)
        {
			if (*it == url)
            {
				exist = true;
            }
        }
		
		if (!exist)
        {
            all_links.push_back(url);
			temp_links.push_back(url);
        }
		
        pthread_mutex_unlock(&lock);
        
        echo = match.suffix().str();
    }
	
	/// Send data to server
	// cout << "ALL LINKS" << endl;
    
    // for (deque<string>::iterator it = all_links.begin(); it!=all_links.end(); ++it)
    // {
        // std::cout << *it << endl;
    // }
	
	/// HTTP html page length
	int body_length = htonl(body.length());
	if (send(serverFD, (const char*)&body_length, 4, 0) < 0) 
	{
		cerr << "Error on send html page length\n";
    }
	
	/// HTTP html page
	if (send(serverFD, body.c_str(), body.length(), 0) < 0) 
	{
		cerr << "Error on send html page content\n";
    }
	
	string list = "";
	for (deque<string>::iterator it = temp_links.begin(); it != temp_links.end(); ++it)
	{
		string item = *it;
		list += (item + ";");
	}
	
	/// List of URLs size
	int quantity = htonl(list.length());
	if (send(serverFD, (const char*)&quantity, 4, 0) < 0) 
	{
		cerr << "Error on send urls list size\n";
    }
	
	/// List of URLs
	if (send(serverFD, list.c_str(), list.length(), 0) < 0) 
	{

		cerr << "Error on send urls list in string content\n";
    }
	
	// cout << body_length << endl;
	// cout << quantity << endl;
	// cout << list << endl;
	
	close (sockFD);  
	
    pthread_exit(NULL);
}
///192.168.43.180