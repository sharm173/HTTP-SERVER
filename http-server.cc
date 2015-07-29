#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
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
pthread_mutex_t mutex;

// Processes time request
void dispatchHTTP( int socket );
void iterativeServer (int masterSocket);
void forkServer (int masterSocket);
void createThreadForEachRequest (int masterSocket);
void poolOfThreads (int masterSocket);
void *loopthread (int masterSocket);

extern "C" void zomboy( int sig )
{
int pid = wait3(0, 0, NULL);
while(waitpid(-1, NULL, WNOHANG) > 0);
 
}

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

	struct sigaction zombie;
	zombie.sa_handler = zomboy;  
	sigemptyset(&zombie.sa_mask);
	zombie.sa_flags = SA_RESTART;
	int error2 = sigaction(SIGCHLD, &zombie, NULL );
	
	if ( error2 ) {
		perror( "sigaction" );
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
		printf("before slaveSocket\n");
		int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
		printf("after slaveSocket\n");
		if(slaveSocket < 0) {
		printf("%s",strerror(errno));
		}
		
		if (slaveSocket >= 0) {
			dispatchHTTP(slaveSocket);
		}
	//close(slaveSocket);
	//shutdown(slaveSocket,0);
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
	pthread_mutex_init(&mutex, NULL);
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
		pthread_mutex_lock(&mutex);
                int slaveSocket =accept(masterSocket,(struct sockaddr*)&clientIPAddress, (socklen_t*)&alen);
		pthread_mutex_unlock(&mutex);

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

	printf("1\n");
	while(n = read(socket, &newChar, sizeof(newChar))){
		//length++;
		if(newChar == ' '){
			
			if(gotGET==0)
				gotGET = 1;
			else {
				curr_string[length]=0;
				strcpy(docPath, curr_string);
			}
		}
		
		else if(newChar == '\n' && oldChar == '\r'){
			read(socket, &oldChar, sizeof(oldChar));
			read(socket, &newChar, sizeof(newChar));
			if(oldChar == '\r' && newChar == '\n') break;
		while(1){
			while(read(socket, &oldChar, sizeof(oldChar))) {
			if(oldChar == '\r') {
			read(socket, &newChar, sizeof(newChar));
			break;
			}
			}
			read(socket, &oldChar, sizeof(oldChar));
                        read(socket, &newChar, sizeof(newChar));
                        if(oldChar == '\r' && newChar == '\n') break;		
		}
			break;
		}
	
		else{
			oldChar = newChar;
			if(gotGET==1){
				curr_string[length] = newChar;
				length++;
			}
		}
	}


	printf("2\n");
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
			strcpy(docPath, "/index.html");
			strcat(cwd, "/http-root-dir/htdocs/index.html");
		}
		
		else {
			strcat(cwd, "/http-root-dir/htdocs");
			strcat(cwd, docPath);
		}
	}

	if(strstr(docPath,"..") != 0) {  //possible bug
		char expand[1025] = {0};
		char *real = realpath(docPath, expand);
	
		if(real != NULL && strlen(expand) >= strlen(cwd) + strlen("/http-root-dir/htdocs")) {
			strcpy(cwd,expand);	
	
		}
	
	}
	printf("3\n");


	char contentType[256] = {0};
	
	
	
	if(strstr(docPath, ".html") != NULL || strstr(docPath,".html/") != NULL) {
	strcpy(contentType,"text/html");
	}
	
	else if(strstr(docPath, ".gif") != NULL || strstr(docPath,".gif/") != NULL) {
	strcpy(contentType, "image/gif");
	}
	
	else if (strstr(docPath, ".jpg") != NULL || strstr(docPath,".jpg/") != NULL){
        strcpy(contentType, "image/jpeg");
    	}
	else if (strstr(docPath, ".xbm") != NULL || strstr(docPath,".xbm/") != NULL){
        strcpy(contentType, "image/xbm");  
        }
	
	else {
	strcpy(contentType, "text/plain");
	}

	printf("4\n");

if(strstr(cwd,"cgi-bin")!= NULL) {
                write(socket, "HTTP/1.1 200 Document follows\r\n", 31);
                write(1, "HTTP/1.1 200 Document follows\r\n", 31);
                write(socket, "Server: CS252 Lab4\r\n", 20);
                write(1, "Server: CS252 Lab4\r\n", 20);
                write(socket, "Content-type: ",14);
                write(1, "Content-type: ",14);
                write(socket,contentType, strlen(contentType));
                write(1,contentType, strlen(contentType));
                write(socket, "\r\n\r\n",4);
                write(1, "\r\n\r\n",4);
int tmpout = dup(1);
int tmpsoc = dup(socket);
dup2(socket,1);
int ret = fork();

if(fork == 0) {
//child process
char *arr[2];
arr[0] = cwd;
arr[1] = NULL;

execvp(arr[0], arr);
_exit(1);
}

dup2(tmpout,1);
close(tmpout);
dup2(tmpsoc,socket);
close(tmpsoc);
}

else {
	FILE *doc;
	//printf("%s",cwd);	
	if (strstr(contentType, "image") != 0)// || strstr(contentType, "image/jpeg") != 0 ||strstr(contentType, "image/xbm") != 0  )
       		doc = fopen(cwd, "rb");
	else
        	doc = fopen(cwd, "r");


	printf("5\n");
	if(doc > 0) {
	
		write(socket, "HTTP/1.1 200 Document follows\r\n", 31);
		write(1, "HTTP/1.1 200 Document follows\r\n", 31);
		write(socket, "Server: CS252 Lab4\r\n", 20);
		write(1, "Server: CS252 Lab4\r\n", 20);
		write(socket, "Content-type: ",14);
		write(1, "Content-type: ",14);
		write(socket,contentType, strlen(contentType));
		write(1,contentType, strlen(contentType));
		write(socket, "\r\n\r\n",4);	
		write(1, "\r\n\r\n",4);
		long count = 0;
		
		char c;
		int fd = fileno(doc);
		
		while((count = (read(fd,&c,1)))){
	
			//write(socket,&c,1);
			//write(1,&c,1);
			if((write(socket,&c,1)) != count){
			 	perror("write");
			}

		}
		printf("Write\n");		
		fclose(doc); 
		printf("close\n");
		close(fd);
	}



	else {
	//404 error
		const char *notFound = "File not Found";
		
                write(socket, "HTTP/1.1 404 File Not Found\r\n", 29);
                        
                write(socket, "Server: CS252 Lab4\r\n", 20);
        
                write(socket, "Content-type: ",14);
                 
                write(socket,contentType, strlen(contentType));
        
                write(socket, "\r\n\r\n",4);		
		
		write(socket,notFound, strlen(notFound));
	
	}

	}






	//return;
	close(socket);
//shutdown(socket,0);
}
