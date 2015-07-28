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
void dispatchHTTP( int socket );
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
   
if(argc == 3 ) {
if(argv[1][1] == 'f') forkServer(masterSocket);
else if (argv[1][1] == 't') createThreadForEachRequest(masterSocket);
else if (argv[1][1] == 'p') poolOfThreads(masterSocket);

else {

}

}

else {
//run iterative server
iterativeServer(masterSocket);
}





}

void iterativeServer( int masterSocket) {

	while (1) {
		 struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
	//		if(slaveSocket == -1) continue;		
		if (slaveSocket >= 0) {
			dispatchHTTP(slaveSocket);
		}
//	close(slaveSocket);
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

void dispatchHTTP( int socket ) {

char curr_string[1025];
int n;
unsigned char newChar;
unsigned char oldChar = 0;
int gotGET = 0;
int length = 0;
char docPath[1025] = {0};


	while(n = read(socket, &newChar, sizeof(newChar))){
		length++;
		if(newChar == ' '){
			
			if(gotGET==0)
				int gotGet = 1;
			else 
				curr_string[length]=0;
				strcpy(docPath, curr_string);
		}
		
		else if(newChar == '\n' && oldChar == '\r'){
			break;
		}
	
		else{
			oldChar = newChar;
			if(gotGET==0){
				curr_string[length] = newChar;
				length++;
			}
		}
	}

	char cwd[256] = {0};
	getcwd(cwd,sizeof(cwd)); 

	if (strncmp(docPath, "/icons", strlen("/icons")) == 0) {
		strcat(cwd, "/http-root-dir/");	
		strcat(cwd, docPath);
	}
	
	else if (strncmp(docPath, "/htdocs", strlen("/htdocs")) == 0) {
		strcat(cwd, "/http-root-dir/");
		strcat(cwd, docPath);
	}

	else if (strncmp(docPath, "/cgi-bin", strlen("/cgi-bin")) == 0) {
		strcat(cwd, "/http-root-dir/");
		strcat(cwd, docPath);
	}
	
	else {
	
		if(strcmp(docPath,"/") == 0) {
			strcat(cwd, "/http-root-dir/htdocs/index.html");
		}
		
		else {
			strcat(cwd, "/http-root-dir/htdocs");
			strcat(cwd, docPath);
		}
		
	
	}

	if(strstr(docPath,"..") != 0) {
		char expand[1025] = {0};
		char *real = realpath(docPath, expand);
	
		if(real != NULL && strlen(expand) >= strlen(cwd) + strlen("/http-root-dir")) {
			strcpy(cwd,expand);	
	
		}
	
	}
	
	char contentType[1025] = {0};
	
	
	
	if(strstr(docPath, ".html") != NULL || strstr(docPath,".html/") != NULL) {
	strcpy(contentType,"text/html");
	}
	
	if(strstr(docPath, ".gif") != NULL || strstr(docPath,".gif/") != NULL) {
	strcpy(contentType, "image/gif");
	}

	else {
	strcpy(contentType, "text/plain");
	}
	
}
