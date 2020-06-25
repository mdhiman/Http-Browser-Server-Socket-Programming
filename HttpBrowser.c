#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#define COMMANDARRAYSIZE 300
#define BUFSIZE 2000
/*
This enum specifies the different file types browser can receive
*/
enum
{
  html,
  pdf,
  image_jpeg,
  image_png,
  other
};
FILE *fp;//file pointer for log file
int sockfd;//socket descriptor
static int fileCount;//it is variable which is used for naming of file
struct stat info;

char * createAndSendHeader(char filetype[],int newsockfd,char *buf,char *partHeader)
{
  char header[1000];
  //printf("part header is....%s\n",partHeader );
  time_t current_time,modified_time;
  current_time=time(NULL);
  modified_time=info.st_mtime;
  int fileSize=info.st_size;
  if(!strstr(filetype,"image"))
  {
  sprintf(header,"%s HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\nDate: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n",
                   partHeader,ctime(&current_time),ctime(&modified_time),fileSize,filetype);

  }
  else
  {
  sprintf(header,"%s HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: Closed\r\n\r\n",
                   partHeader,fileSize,filetype);
  }
  buf=(char*)malloc(strlen(header));
  memcpy(buf,header,strlen(header));
  buf[strlen(header)]='\0';
  printf("header size is....%d\n",strlen(header) );
  printf("buf size is.....%d\n",strlen(buf) );
  return buf;     
}

/*
@def:this function finally call the respected application with respect to the file type,
as for example if the filetype is pdf then it opens the document in evince
@param1:type char array,it is basically filename which is to be opened
@param2:it takes a application_type and it is int value,and bounded to above enum type
@return:it does not return any value
*/
void FinalShowingOutput(char filename[],int application_type)
{
   if(application_type==pdf)
{
execlp("evince","evince",filename,NULL);
}
else if(application_type==html)
{
execlp("firefox","firefox",filename,NULL);
}
else if(application_type==other)
{
execlp("gedit","gedit",filename,NULL);
}
else if (application_type==image_jpeg||application_type==image_png)
{
  execlp("eog","eog",filename,NULL);
}

}
/*
@def:it processes the GET command ,it add http header,send request to server,receives data from server and parse
the header of received packet ,after parsing it decides file type and create the output file and call a finalShowingOutput function
and open the file with respecting application and close the socket and file

@param1:it takes a char* parameter as input ,the pointer is basically pointer to the address given by the user

@return:it does not return anything
*/
void ProcessGet(char *command)
{
            char buf[BUFSIZE],*totaldata=NULL;//buf is for storing command which is input param 
            char buffer[BUFSIZE];//this buffer is for receiving data
            strcpy(buf,command);
            buf[strlen(command)]='\0';
            int size=0;
            fprintf(fp,"%s\n",buf);
           // printf("%s\n",buf );
            //adding http header
              strcat(buf," HTTP/1.0\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\nAccept: */*\r\nCache-Control: no-cache\r\nAccept-Encoding:gzip, deflate\r\nfrom: dhiman\r\n\n");
            
            send(sockfd, buf, strlen(buf) + 1, 0);
            //printf("after sending header..in get\n" );
            int tempsize=0,totalsize=0,tempdatacount=0;
            //receiving data from server...
            while(tempsize=recv(sockfd,buffer,BUFSIZE,0))
            {
              totalsize=totalsize+tempsize;
              totaldata=(char*)realloc(totaldata,totalsize);
              memcpy(totaldata+tempdatacount,buffer,tempsize);
              tempdatacount=totalsize;
            }
            printf("after receiving\n" );
            //finding address of the first byte of data excluding http header which is separated by /r/n/r/n
            char *startdataAddress=strstr(totaldata,"\r\n\r\n");
            //finding  length of header data
            int headerdataLength=startdataAddress- totaldata;
            //finding content type of the file e.g:pdf/jpeg/html
            char *filetypePtr=strstr(totaldata,"Content-Type: ");
            char filetype[60];
            //copying file type name to filetype array
            printf("before filetype..\n" );
             sscanf(filetypePtr,"Content-Type: %s\r\n",filetype);
            printf("File-Type = %s\n", filetype);
            char filename[100];
            int application_type;
            /*@if-block
              creating file corresponding to the file type 
            */
            if(strstr(filetype,"text/html")!=NULL)
            {  
              sprintf(filename,"file%d.html",fileCount);
              application_type=html;
            }
            else if(strstr(filetype,"application/pdf")!=NULL)
             {
              sprintf(filename,"file%d.pdf",fileCount);
              application_type=pdf;
             }
             else if(strstr(filetype,"image/jpeg")!=NULL)
             {
              sprintf(filename,"file%d.jpeg",fileCount);
              application_type=image_jpeg;
            }
             else if(strstr(filetype,"image/png")!=NULL)
             {
              sprintf(filename,"file%d.png",fileCount);
              application_type=image_png;
             }
             else 
             {
              sprintf(filename,"file%d",fileCount);
              application_type=other;
             }
              fileCount++;
              FILE *outputfilePtr = fopen(filename,"w");
              //writing received data without header to the file..
              fwrite(totaldata + headerdataLength + 4,1,totalsize- headerdataLength-4,outputfilePtr);
              FILE *raw = fopen("rawfile","w");
              //write whole data to the file rawfile
               fwrite(totaldata,1,totalsize,raw);
               fclose(raw);//closing file
               fclose(outputfilePtr);//closing raw file with header
               free(totaldata);
               //fork a process for system call..because it does not block the current process on that call
               if(fork()==0)
               {
              FinalShowingOutput(filename,application_type);
                }
            close(sockfd); //closing socket
}
/*
@def:
@param1:
@param2:
@return:
*/
void ProcessPut(char *command)
{
      char *tempCommand=(char*)malloc(strlen(command));
      memcpy(tempCommand,command,strlen(command));
      tempCommand[strlen(command)]='\0';
      printf("%s\n",tempCommand );
      char *abc=strtok(command,"//");
        printf("%s\n",abc );
        char *new =strtok(NULL,"/");
        printf("%s\n",new );
        char *filename=strtok(NULL,"/");
        printf("%s\n",filename );
        if(stat(filename,&info)==-1)
        {
          printf("file does not exist..put exit...\n" );
          return;
        }

         char *buf=NULL;    
          if(strstr(filename,"pdf"))
        {
          buf=createAndSendHeader("application/pdf",sockfd,buf,tempCommand);
        }
        else if(strstr(filename,"html"))
        {
        buf=createAndSendHeader("text/html",sockfd,buf,tempCommand);
        }
        else if(strstr(filename,"jpeg")||strstr(filename,"jpg"))
        {
        buf=createAndSendHeader("image/jpeg",sockfd,buf,tempCommand);
        
        }
        else if(strstr(filename,"png"))
        {
        buf=createAndSendHeader("image/png",sockfd,buf,tempCommand);       }
        else
        {
        buf=createAndSendHeader("text/html",sockfd,buf,tempCommand);
        }
        printf("after header....\n");
        FILE *source=fopen(filename,"r");
        printf("header size is....%d\n",strlen(buf) );
        size_t headerSize=strlen(buf);
        size_t size=info.st_size+headerSize;
           buf=( char*)realloc(buf,size);
          fread(buf+headerSize, 1, size- headerSize, source);
        send(sockfd, buf, size, 0);

fclose(source);
close(sockfd);
return;
}
/*
@def:this function process the command and also find ip address from given 
host by gethostbyname() and create and connect the socket  
@param1:string which is basically a string enter by the user with get/put and url
@param2:it is the length of the parameter 1
@return:it does not returns anything..
*/
void ProcessCommand(char *command,int size)
{
  char *token;
  char ip[200];
  char buf[4];
  command[strlen(command)-1]='\0';
  char temp[300];
  strncpy(temp,command,strlen(command));
  temp[strlen(command)]='\0';
token =strtok(temp,"//");//tokenize the command with space
printf("%s\n",token );
strncpy(buf,token,3);
buf[3]='\0';
token=strtok(NULL,"//");//
token[strlen(token)]='\0';
char ipbyname[50];
char port_number[10];
if(strstr(token,":"))
{
 // printf("i am in if....\n" );
  char *delimaddress=strstr(token,":");
  memcpy(ipbyname,token,delimaddress-token);
  ipbyname[delimaddress-token]='\0';
  memcpy(port_number,delimaddress+1,token+strlen(token)-delimaddress);
 // printf("port number is...%s\n",port_number );
 // printf("%s\n",ipbyname );
}
printf("%s\n",token );
convertHostNameToIp(ipbyname,ip);//call function for changing name to ip address..
//creating socket....
if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                 fprintf(fp,"cannot create socket error" );
                 exit(1);
            }
struct sockaddr_in server;
//assigning host address
//strcpy(ip,"10.3.100.207");
server.sin_family = AF_INET;
server.sin_addr.s_addr = inet_addr(ip);
server.sin_port  = htons(atoi(port_number)); 
            //connect call
            if ((connect(sockfd, (struct sockaddr *) &server,sizeof(server))) < 0) 
            {
              perror("unable to connect...");
                fprintf(fp,"Unable to connect to server...connect error\n");
                exit(0);
            }
            printf("%s\n",buf);
if(!strcmp(buf,"GET"))
{
  printf("i am in GET.....\n" );
  ProcessGet(command);
}
if(!strcmp(buf,"PUT"))
{
  ProcessPut(command);
}
return;
}

/*
@def:this function take hostname as parameter and return the corresponding IP by DNS call
@param1:this parameter is basically the host name ...as for example cse.iitkgp.ac.in 
@param2:this parameter is to store the ip of the given hostname 
@return:return 1 on success
*/
int convertHostNameToIp(char *hostname,char *ip)
{
  struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
  
        strcpy(ip , inet_ntoa(*addr_list[0]) );//copying ip address from sockenet structure ..we only copy the first ip address
        printf("%s\n",ip );

  //printf("%s\n",ip);
     
    return 1;
}

int main()
{
  char command[COMMANDARRAYSIZE];//array for command
  char quit[5]="quit\n";
  fp=fopen("MyBrowserLog.txt","a");//log file for some error
  fileCount=0;//initialize fileCount variable which is global and static
  while(1)
  { fflush(stdin);//clearing the stdin
    printf("MyBrowser>");
    fgets(command,COMMANDARRAYSIZE,stdin);
    if(strncmp(command,quit,5)==0)
    {
    break;
    }
    if(strncmp(command,"PUT",3)==0||strncmp(command,"GET",3)==0)
    {
    ProcessCommand(command,COMMANDARRAYSIZE);
    }
    else
    {
    printf("invalid input..try again\n");
    } 
    
  }

  fclose(fp);
return 0;
}
