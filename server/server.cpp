/* Server for search engine
 * Module: CG3204L
 * Author: Wan Wenli, Qiu Boqin, ...
 * Last updated: 3:11PM 27 Mar 2013
 */
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 5000
#define DB_DIR "/tmp/database/"

using namespace std;

void* clientThread (void *args);

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
    int newSocketFD;
    while (true)
	{
        if ( ( newSocketFD = accept ( socketFD , NULL , NULL ) ) < 0 )
		{
            std :: cerr << "Error on accept()\n";
            close ( socketFD );
            return -1;
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

    close ( socketFD );
	p_thread.exit(NULL);
    return NULL;
}
