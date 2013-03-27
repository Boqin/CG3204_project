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

void* clientThread ( void *args );

int main ( int argc , char **argv ) {

    // Port to listen on
    uint16_t port = PORT;

    // Create a TCP socket
    int socketFD;
    if ( ( socketFD = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 ) {
        std :: cerr << "Error creating Server socket\n";
        return -1;
    }

    // Bind to all IP Addresses of this machine, on port 5000 
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
    cout << "Echo Server Running on port " << port << endl;
    int newSocketFD;
    while ( true ) {

        if ( ( newSocketFD = accept ( socketFD , NULL , NULL ) ) < 0 ) {
            std :: cerr << "Error on accept()\n";
            close ( socketFD );
            return -1;
        }

        // On a new connection, create a new thread
        //  Also, tell the thread the socket FD it should use for this connection
        pthread_t threadID;
        int *clientSocketFD = new int;
        *clientSocketFD = newSocketFD;
        if ( clientSocketFD == NULL ) {
            std :: cerr << "Out of heap memory\n";
            close ( socketFD );
            return -1;
        }
        if ( pthread_create ( &threadID , NULL , clientThread , clientSocketFD ) != 0 ) {
            std :: cerr << "Error on pthread_create()\n";
            close ( socketFD );
            return -1;
        }
    }

    return 0;
}

// Thread handling one particular client
void* clientThread ( void *args ) {

    // Get the socketFD this thread has been assigned to
    int *clientSocketFD = (int*) args;
    int socketFD = *clientSocketFD;
    delete clientSocketFD;

    // Receive client input until the user types 'BYE'
    std :: string welcomePrompt = "Type BYE to exit...\r\n";
    if ( send ( socketFD , welcomePrompt.c_str() ,
                welcomePrompt.length() , 0 ) < 0 ) {
        std :: cerr << "Error on send()\n";
        close ( socketFD );
        return NULL;
    }
    
    // Here we read 1 byte at a time. Buffering can be
    //  used to achieve better I/O efficiency.
    signed char ch;
    std :: string echoLine = "";
    while ( recv ( socketFD , &ch , 1 , 0 ) == 1 ) {

        // Ignore \r
        if ( ch == '\r' )
            continue;

        // On End of line, send an echo back to the client
        else if ( ch == '\n' ) {
            if ( echoLine == "BYE" )
                break;
            echoLine = "Echo: " + echoLine + "\r\n";
            if ( send ( socketFD , echoLine.c_str() ,
                        echoLine.length() , 0 ) < 0 ) {
                std :: cerr << "Error on send()\n";
                break;
            }
            echoLine = "";
        }

        // Otherwise, store the character
        else 
            echoLine += ch;
    }

    // Client has closed connection or there was error
    //  Close the socket and exit the thread
    close ( socketFD );
    return NULL;
}
