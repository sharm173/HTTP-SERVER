#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

const char * usage =
"								\n"
"Simple iterative HTTP server					\n"
"								\n"
"To use it in one window type:					\n"
"								\n"
"   myhttpd [-f|-t|-p] [<port>]					\n"
"								\n"
"Where 1024 < port < 65536					\n"
"								\n"
"-f :Create a new process for each request			\n"
"								\n"
"-t : Create a new thread for each request			\n"
"								\n"
"-p:  Pool of threads						\n";

int QueueLength = 5;  

// Processes time request
void dispatchHTTP( int slaveSocket );
void iterativeServer (int masterSocket);
void forkServer (int masterSocket);
void createThreadForEachRequest (int masterSocket);
void poolOfThreads (int masterSocket);
void *loopthread (int masterSocket);

int
main(int argc, char **argv)
{

int port = 0;
if(argc == 2) port = atoi( argv[1] );

else if(argc == 3) port = atoi(argv[2]);

else {
    fprintf( stderr, "%s", usage );
    exit( -1 );
}
 // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress;
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);

  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }
   
  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1;
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR,
                       (char *) &optval, sizeof( int ) );
    
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
                    (struct sockaddr *)&serverIPAddress,
                    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the      
  // size of the queue of unprocessed connections  
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }
   
if(argc == 3 && argv[1][1] == 'p') {


}

else {



}





}

void iterativeServer( int masterSocket) {

	while (1) {
		 struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
		
		if (slaveSocket >= 0) {
			dispatchHTTP(slaveSocket);
		}

	//dispatch http closes slave socket

	}

}


void forkServer( int masterSocket) {
	while (1) {
                 struct sockaddr_in clientIPAddress;
                int alen = sizeof( clientIPAddress );
                int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
		
		if (slaveSocket >= 0) {
			int ret = fork();
			if (ret == 0) {
				dispatchHTTP(slaveSocket);
				exit(0);
			}
			close(slaveSocket);
		}

	}
}

void createThreadForEachRequest(int masterSocket)
{
	while (1) {
                 struct sockaddr_in clientIPAddress;
                int alen = sizeof( clientIPAddress );
                int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);

		if (slaveSocket >= 0) {
			// When the thread ends resources are recycled
			pthread_t thread;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
			pthread_create(&thread, &attr, (void * (*)(void *))dispatchHTTP, (void *) slaveSocket);

		}
	}
}


void poolOfThreads( int masterSocket ) {
	pthread_t thread[5];
	for (int i=0; i<4; i++) {
		pthread_create(&thread[i], NULL, (void * (*)(void *))loopthread,(void *)masterSocket);
	}

	loopthread (masterSocket);
}


void *loopthread (int masterSocket) {

	while (1) {
                 struct sockaddr_in clientIPAddress;
                int alen = sizeof( clientIPAddress );
                int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
		if (slaveSocket >= 0) {
			dispatchHTTP(slaveSocket);
		}
	}
}

void dispatchHTTP( int slaveSocket ) {


}
