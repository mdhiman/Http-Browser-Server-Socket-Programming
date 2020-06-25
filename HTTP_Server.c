#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#define BUFSIZE 2000
#define MAXHEADERSIZE 1200

pthread_mutex_t mutexsum;//it is a variable used to track record of maximum number of use
struct stat info;
FILE *flog;
char * createAndSendHeader(char filetype[],int newsockfd,char *buf)
{
	char header[MAXHEADERSIZE];

	time_t current_time,modified_time;
	current_time=time(NULL);
	modified_time=info.st_mtime;
	int fileSize=info.st_size;
	if(!strstr(filetype,"image"))
	{
	sprintf(header,"HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\nDate: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n",
                   ctime(&current_time),ctime(&modified_time),fileSize,filetype);
	}
	else
	{
	sprintf(header,"HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n",
                   fileSize,filetype);
	}
	buf=(char*)realloc(buf,strlen(header));
	memcpy(buf,header,strlen(header));
	buf[strlen(header)]='\0';
	//send(newsockfd,header,strlen(header),0);
	return buf;			
}
void processGet(char temp[],int newsockfd)
{
		 		 char *buf=NULL;		
					if(strstr(temp,"pdf"))
				{
					buf=createAndSendHeader("application/pdf",newsockfd,buf);
				}
				else if(strstr(temp,"html"))
				{
				buf=createAndSendHeader("text/html",newsockfd,buf);
				}
				else if(strstr(temp,"jpeg")||strstr(temp,"jpg"))
				{
				buf=createAndSendHeader("image/jpeg",newsockfd,buf);
				
				}
				else if(strstr(temp,"png"))
				{
				buf=createAndSendHeader("image/png",newsockfd,buf);				}
				else
				{
				buf=createAndSendHeader("text/html",newsockfd,buf);
				}
				printf("after header....\n");
				FILE *source=fopen(temp,"r");
				size_t headerSize=strlen(buf);
				size_t size=info.st_size+headerSize;
   				 buf=( char*)realloc(buf,size);
    			fread(buf+headerSize, 1, size- headerSize, source);
				send(newsockfd, buf, size, 0);

			/* We again initialize the buffer, and receive a 
			   message from the client. 
			*/
			/*for(i=0; i < 100; i++) buf[i] = '\0';
			recv(newsockfd, buf, 100, 0);*/
			//printf("%s\n", buf);
			 fclose(source);
			
}
void processPut(FILE *newfile,int newsockfd)
{
				
			printf("processig put....\n" );
				unsigned char buf[255];
    			size_t size;
    			while(size=recv(newsockfd,buf,255,0))
           		 {
             	 fwrite(buf, 1, size, newfile);
           		 }
          	  fclose(newfile);
          	  fprintf(flog,"file putted ...successfully.....\n" );
}
void *handleRequest(void *data)
{
	/* We initialize the buffer, copy the message to it,
			   and send the message to the client. 
			*/
			   int newsockfd=(int)data;
			   char buff[BUFSIZE];
			   flog=fopen("MyHTTPLog.txt","w");
			   for (int i = 0; i < BUFSIZE; ++i)
			   {
			   	buff[i]='\0';
			   }
			int headerSize=recv(newsockfd,buff,BUFSIZE,0);
			fprintf(flog,buff );
			int count=0;
			char temp[200];
			char findComment[4];
			int charCount=0;
			//printf("%s\n",buff );
			for (int i = 0; i < strlen(buff); ++i)
			{
					if(i<3)
					findComment[i]=buff[i];
				
			}
			findComment[3]='\0';
			fprintf(flog,"%s....\n",findComment);
			fprintf(flog,"%s....\n",buff );
			if(!strstr(buff,"http://"))
			{
				printf("if block.....\n" );
				for (int i = 0; i < strlen(buff); ++i)
			{
				if(buff[i]==' ')
				{
					count++;
				}
				else if(count==1)
				{
					if(buff[i]!='/')
					temp[charCount++]=buff[i];
				}
				if(count==2)
				{
					temp[charCount]='\0';
					//printf("hi....from server...\n" );
					break;
				}
			}

			}
			else
			{
				fprintf(flog,"new block....\n" );
				char header[300];
				memcpy(header,buff,strstr(buff,"\n")-buff);
				printf("else block....\n" );
				printf("%s\n",header );
				header[strstr(buff,"\n")-buff]='/0';
				fprintf(flog,"%s\n",header );
				char *abc=strtok(header,"//");
				fprintf(flog,"%s\n",abc );
				char *new =strtok(NULL,"/");
				fprintf(flog,"%s\n",new );
				char *filename=strtok(NULL,"/");
				fprintf(flog,"%s\n",filename );
				filename=strtok(filename," ");
				fprintf(flog,"%s\n",filename );
				memcpy(temp,filename,strlen(filename));
			}
			if(strcmp(findComment,"GET")==0)
			{
				if(stat(temp,&info)==-1)
				{
					fprintf(flog,"error in opening file..file not found or ..permission denied");
					pthread_exit(NULL);
				}
			}
			if(strcmp(findComment,"GET")==0)
			{	printf("i am in get....from server...\n" );
				processGet(temp,newsockfd);
			}
			if(strcmp(findComment,"PUT")==0)
			{
				char finalFinalname[]="server/";
				strcat(finalFinalname, temp);
				printf("%s\n",finalFinalname );
			FILE *newfile=fopen(finalFinalname,"w");
			if(newfile==NULL)
			{
				printf("file does not exist\n" );
			}
			char *startHeader=strstr(buff,"\r\n\r\n");
			fwrite(startHeader+4,1,headerSize-(startHeader- buff+4),newfile);

			processPut(newfile,newsockfd);
			}
			close(newsockfd);
			fclose(flog);
			pthread_exit(NULL);

}

int main()
{
	int			sockfd ; /* Socket descriptors */
	struct sockaddr_in	serv_addr;

	int i;
	char buf[100];		/* We will use this buffer for communication */
	pthread_mutex_init(&mutexsum,NULL);
	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	   flog=fopen("MyHTTPLog.txt","w");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		fprintf(flog,"Cannot create socket\n");
		exit(0);
	}
	/* The structure "sockaddr_in" is defined in <netinet/in.h> for the
	   internet family of protocols. This has three main fields. The
 	   field "sin_family" specifies the family and is therefore AF_INET
	   for the internet family. The field "sin_addr" specifies the
	   internet address of the server. This field is set to INADDR_ANY
	   for machines having a single IP address. The field "sin_port"
	   specifies the port number of the server.
	*/
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(50000);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		fprintf(flog,"Unable to bind local address\n");
		exit(0);
	}
	listen(sockfd,30);
	fclose(flog);//closing log file
	while (1) 
	{

		/* The accept() system call accepts a client connection.
		   It blocks the server until a client request comes.

		   The accept() system call fills up the client's details
		   in a struct sockaddr which is passed as a parameter.
		   The length of the structure is noted in clilen. Note
		   that the new socket descriptor returned by the accept()
		   system call is stored in "newsockfd".
		*/
		   struct sockaddr_in cli_addr;
		int clilen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;
		flog=fopen("MyHTTPLog.txt","w");
		if (newsockfd < 0) 
		{
			fprintf(flog,"Accept error\n");
			exit(0);
		}
		pthread_t newClient;
		fclose(flog);
		int retid=pthread_create(&newClient,NULL,handleRequest,(void*)newsockfd);
		if(retid)
		{
			perror("thread creation failed with error code..");
		}
	}
	
	return 0;

}