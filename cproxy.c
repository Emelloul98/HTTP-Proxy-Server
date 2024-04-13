#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#define CHUNK_SIZE 3000
#define PATH_SYSTEM_MAX 4096
void writeToSocket(char*,int);
char* addHttpFormat(char*,char*,int);
void readFromSocket(int,char*);
int fileExists(const char*);
void createDirectories(const char*);
void writeToFile(FILE* file,char*,int);
void printFile(const char*);
void openWithFirefox(char*);
int countFile(const char*);

int main(int argc, char *argv[]) {
    struct hostent* server_info=NULL;
    in_port_t port=80;
    int sock_fd,url_length,portLen,isFlag=0,isPath=1,indexFlag=0;
    struct sockaddr_in sock_info;
    char *nameStart,*portStart,*nameEnd, *pathWIthSlash;
    //input from the cmd:
    if(argc<2 || argc>3)
    {
        printf("Usage: cproxy <URL> [-s]\n");
        exit(EXIT_FAILURE);
    }
    if(argc==3)
    {
        int flagLen= (int)strlen(argv[2]);
        char flag[flagLen];
        flag[0]='\0';
        strcpy(flag,argv[2]);
        if(strcmp(flag,"")!=0 && strcmp(flag,"-s")!=0)
        {
            printf("Usage: cproxy <URL> [-s]\n");
            return 1;
        }
        else if(strcmp(flag,"-s")==0)
        {
            isFlag=1;
        }
    }
    int size=(int)strlen(argv[1]);
    char full_url[size+1];
    strncpy(full_url, argv[1], size);
    full_url[size]='\0';
//    error detection:
    if((strncmp(full_url,"http://",7)!=0)) // Usage error if the url is missing a http start
    {
        printf("Usage: cproxy <URL> [-s]\n");
        exit(EXIT_FAILURE);
    }
    // Parse the URl:
    nameStart=strstr(full_url,"://")+3;// A pointer to the first w of the url(www.google.com)
    portStart=strchr(nameStart, ':');// A pointer to the port number start(optional)
    pathWIthSlash= strchr(nameStart, '/');// A pointer to the path start include the '/' sign
    if(portStart!=NULL)
    {
        nameEnd=portStart-1;
    }
    else if(pathWIthSlash != NULL)
    {
        nameEnd= pathWIthSlash - 1;
    }
    else // when there is no ":" and no "/" in the url
    {
        nameEnd=nameStart+(int)strlen(nameStart)-1;
    }
    url_length= (int)(strlen(nameStart)-strlen(nameEnd)+1);
    // Only if there is a port number:
    if(portStart != NULL)
    {
        if(pathWIthSlash != NULL)
        {
            portLen= (int)(strlen(portStart) - strlen(pathWIthSlash) - 1); // How many digits the port number is
        }
        else portLen= (int)(strlen(portStart)-1);
        char port_string[portLen+1];
        strncpy(port_string,portStart+1,portLen);
        port_string[portLen]='\0';
        port=atoi(port_string);//insert the new port number to the port variable
        pathWIthSlash= (portStart + portLen + 1);
    }
    char hostName[url_length + 1];
    strncpy(hostName, nameStart, url_length);
    hostName[url_length]='\0';
    int full_path_size;
    if(pathWIthSlash == NULL || strlen(pathWIthSlash)==0)//if there is no path:
    {
        pathWIthSlash="/index.html";
        isPath=0;
    }
    else if(pathWIthSlash[(int)strlen(pathWIthSlash)-1]=='/')
    {
        indexFlag=1;
    }
    full_path_size= (int)(strlen(hostName)+ strlen(pathWIthSlash))+1;
    char* fullPath=(char*)malloc(full_path_size+11);
    if(fullPath==NULL)//malloc error
    {
        fprintf(stderr, "malloc\n");
        exit(EXIT_FAILURE);
    }
    memset(fullPath,0,full_path_size+1);
    snprintf(fullPath,full_path_size,"%s%s",hostName,pathWIthSlash);
    if(indexFlag==1)
    {
        strcat(fullPath,"index.html");
        full_path_size+=(int) strlen("index.html");
    }
    fullPath[full_path_size]='\0';
    //checks if the path already exist:
    if(fileExists(fullPath) == 1)
    {
        char* headers=(char*) malloc(CHUNK_SIZE);
        if(headers==NULL)
        {
            free(fullPath);
            fprintf(stderr, "malloc\n");
            exit(EXIT_FAILURE);
        }
        memset(headers,0,CHUNK_SIZE);
        int contentLength= countFile(fullPath);//counting the content of the file
        //local file system print:
        snprintf(headers,(size_t)CHUNK_SIZE,"HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n",contentLength);
        printf("File is given from local filesystem\n%s",headers);
        printFile(fullPath);
        int totalBytes=contentLength+(int)strlen(headers);
        printf("\n Total response bytes: %d\n",totalBytes);
        if(isFlag==1)// Flag is up when there was '-s' in the url input0
        {
            openWithFirefox(fullPath);
        }
        free(fullPath);
        free(headers);
        return 0;
    }
    // creating a http request:
    char* fullRequest;
    fullRequest=addHttpFormat(pathWIthSlash,hostName,isPath);//calls a function that converts the url to a http request
    int reqLen=(int)strlen(fullRequest);
    printf("HTTP request =\n%s\nLEN = %d\n",fullRequest,reqLen);
    // Opening a new socket:
    if((sock_fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==-1)
    {
        perror("socket failed");
        free(fullRequest);
        free(fullPath);
        exit(EXIT_FAILURE);
    }
    // building the hostent struct that will have the URL info of the server:
    char* getHostName;
    if(strncmp(hostName,"www.",4)==0)
    {
        getHostName=hostName+4;
    }
    else getHostName=hostName;
    server_info=gethostbyname(getHostName);
    if(!server_info)
    {
        herror("gethostbyname\n");
        free(fullRequest);
        free(fullPath);
        exit(EXIT_FAILURE);
    }
    // sock_info initialization:
    memset(&sock_info,0,sizeof(struct sockaddr_in));
    sock_info.sin_family=AF_INET;
    sock_info.sin_port=htons(port);
    sock_info.sin_addr.s_addr=((struct in_addr*)server_info->h_addr)->s_addr;
    //connecting to the server:
    if(connect(sock_fd,(struct sockaddr*)&sock_info,sizeof(struct sockaddr_in))==-1)
    {
        perror("connect failed");
        free(fullRequest);
        free(fullPath);
        exit(EXIT_FAILURE);
    }
    // The proxy needs the server, so he Creates an HTTP request:
    writeToSocket(fullRequest,sock_fd);
    readFromSocket(sock_fd,fullPath);
    if(isFlag==1)
    {
        openWithFirefox(fullPath);
    }
    free(fullPath);
    close(sock_fd);
}
void openWithFirefox(char* fullPath)
{
    int len=(int)strlen(fullPath)+10;// 9 for the xdg-open + '\0' sign
    char* command=(char*)malloc(sizeof(char)*len);
    if(command==NULL)
    {
        fprintf(stderr, "malloc\n");
        exit(EXIT_FAILURE);
    }
    snprintf(command, len,"xdg-open %s",fullPath);//calls the system function to open in firefox
    system(command);//sends the full command to the system function
    free(command);
}
// A function that write a request to the socket that is connected to the server
void writeToSocket(char* fullRequest,int sd)
{
    int alreadyWrite=0;
    int reqLen=(int)strlen(fullRequest);
    while(alreadyWrite < reqLen)// We will continue writing to the server until he takes our request
    {
        ssize_t temp;
        //write to socket:
        temp=write(sd, fullRequest + alreadyWrite, reqLen - alreadyWrite);
        if(temp<0)// If the writing failed
        {
            perror("Write ERR");
            close(sd);
            free(fullRequest);
            exit(EXIT_FAILURE);
        }
        alreadyWrite+=(int)temp;// counting the total amount of chars that was writen
    }
    free(fullRequest);
}
// A function called by write to socket that helps the proxy server to convert url to a http request
char* addHttpFormat(char* path,char* hostName,int flag)
{
    int toFree=0;
    // Calculate the total length needed for the formatted string
    if(path==NULL || strlen(path)==0 ||flag==0)
    {
        path=(char*)malloc(2*sizeof (char));
        if(path==NULL)
        {
            fprintf(stderr, "malloc\n");
            exit(EXIT_FAILURE);
        }
        // adds a '/' sign to the path(only hi is missing one)
        path[0]='/';
        path[1]='\0';
        toFree=1;
    }
    //counting the request length
    size_t totalLength = strlen("GET ") + strlen(path) + strlen(" HTTP/1.0\r\n Host:") + strlen(hostName) + 2*strlen("\r\n") + 1;
    // Allocate memory for the formatted string
    char* httpRequest = (char*)malloc(totalLength);
    if (httpRequest == NULL)
    {
        free(path);
        fprintf(stderr, "malloc\n");
        exit(EXIT_FAILURE);
    }
    // Construct the formatted HTTP request string
    snprintf(httpRequest, totalLength, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, hostName);
    if(toFree==1)
    {
        free(path);
    }
    return httpRequest;
}
//Assigning to 'unsigned char *' from 'char *' converts between pointers to integer types where one is of the unique plain 'char' type and the other is not
void readFromSocket(int sd,char* path)
{
    FILE *file;
    int flag=0,headers_count,body_count;
    unsigned char headers[CHUNK_SIZE];
    memset(headers,0,CHUNK_SIZE);
    unsigned char* body;
    ssize_t totalResponseByte=0;
    unsigned char* buffer;
    ssize_t bytesRead;
    do {
        bytesRead=0;
        buffer=(unsigned char*)malloc(CHUNK_SIZE);
        if(buffer==NULL)
        {
            fprintf(stderr, "malloc\n");
            free(path);
            close(sd);
            exit(EXIT_FAILURE);
        }
        memset(buffer,0,CHUNK_SIZE);//resets the buffer with zeros
        bytesRead = read(sd, buffer, CHUNK_SIZE);
        if (bytesRead < 0)
        {
            perror("Read ERR");
            free(buffer);
            free(path);
            close(sd);
            exit(EXIT_FAILURE);
        }
        if(bytesRead==0)
        {
            free(buffer);
            break;
        }
        totalResponseByte+=bytesRead;//counting the total amount of chars that was read
        if(flag==0) // when this is the first chuck that we read
        {
            char* temp=strstr((char*)buffer,"200 OK\r\n");
            if(temp!=NULL) // there was a good answer from the server
            {
                body=(unsigned char*)strstr((char*)buffer, "\r\n\r\n");
                body=(unsigned char*)(((char*)body)+4);//skips the "\r\n\r\n" sign
                headers_count= (int)(body - buffer);
                memcpy(headers, buffer, headers_count);
                headers[headers_count+1]='\0';
//                printf("my path is: %s",path);
                createDirectories(path);//calls a function that opens a new directory
                file=fopen(path, "wb");//opening a file
                if(file==NULL)
                {
                    printf("Error opening the file.\n");
                    free(buffer);
                    free(path);
                    close(sd);
                    exit(EXIT_FAILURE);
                }
                body_count=((int)(bytesRead))-headers_count;
                printf("%s\r\n\r\n",(char*)headers);//print the headers
                fwrite(body, 1, body_count, stdout);
                writeToFile(file,(char*)body,body_count);//write the body data to the file
                body=NULL;
            }
            else // there was a bed respond from the server
            {
                fwrite(buffer, 1, bytesRead, stdout);
                free(buffer);
                free(path);
                close(sd);
                exit(0);
            }
            flag=1;
        }
        else // when it's not the first chunk that we read, and we already start writing to the file
        {
            fwrite(buffer, 1, bytesRead, stdout);
            writeToFile(file,(char*)buffer,(int)bytesRead);
        }
        free(buffer);
        buffer=NULL;
    }  while (bytesRead!=0);
    printf("\n Total response bytes %d\n",(int)totalResponseByte);
    fclose(file);//close the file after all the socket was read
}
int fileExists(const char *filename)
{
// if the file exists and is readable:
    if (access(filename, F_OK) != -1 && access(filename, R_OK) != -1) return 1;
    return 0;  // else-file does not exist or is not readable
}
// a function that counts the file size:
int countFile(const char *filename)
{
    size_t totalFile=0;
    FILE *file = fopen(filename, "rb");//open the file for read only
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    unsigned char *buffer = (unsigned char *)malloc(CHUNK_SIZE);
    if(buffer==NULL)
    {
        fprintf(stderr, "malloc\n");
        exit(EXIT_FAILURE);
    }
    size_t bytesRead;
    // Read and print the contents of the file
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        totalFile+=bytesRead;
    }
    fclose(file);
    free(buffer);
    return (int)totalFile; //return the int casting of the total amount
}
void createDirectories(const char *path)
{
    int lenPath=(int) strlen(path);
    char *token;
    char *pathCopy=strdup(path);
    // Get the current working directory
    char currentDir[PATH_SYSTEM_MAX];
    getcwd(currentDir, sizeof(currentDir));
    // Iterate through the path and create directories, excluding the last token
    token = strtok(pathCopy, "/");
    int i=0,len;
    char * prevToken;
    char *isExists=(char*)malloc(lenPath+1);
    memset(isExists,0, lenPath+1);
    if(isExists==NULL)
    {
        fprintf(stderr, "malloc\n");
        exit(EXIT_FAILURE);
    }
    while (token != NULL)
    {
        prevToken=(char*)malloc((int)strlen(token)+1);
        if(prevToken==NULL)
        {
            free(isExists);
            fprintf(stderr, "malloc\n");
            exit(EXIT_FAILURE);
        }
        memset(prevToken,0, strlen(token)+1);
        if(i!=0)
        {
            len= (int)strlen(isExists);
            isExists[len]='/';
            isExists[len+1]='\0';
        }
        // Save the previous token
        strcpy(prevToken, token);
        prevToken[(int)strlen(token)]='\0';
        // Create directories only for intermediate tokens (not the last one)
        token=strtok(NULL, "/");
        strcat(isExists,prevToken);
        if(fileExists(isExists)==1)
        {
            i++;
            free(prevToken);
            chdir(isExists);
            continue;
        }
        if (token!= NULL)
        {
            mkdir(prevToken, 0755);  // Create a directory with read, write, and execute permissions for the owner
            chdir(prevToken);        // Change the working directory to the newly created directory
        }
        i++;
        free(prevToken);
    }
    // Change the current working directory back to the original
    if (prevToken != NULL) {
        chdir(currentDir);
    }
    free(isExists);
    free(pathCopy);
}
void writeToFile(FILE* file,char* body,int size)
{
    size_t temp;
    temp = fwrite(body, sizeof(char), size, file);
    if (temp != size) {
        perror("Write ERR");
        fclose(file);
        exit(EXIT_FAILURE);
    }
}
//A function that gets a file name and prints his content
void printFile(const char *filename)
{
    FILE *file = fopen(filename, "rb");//opens the file for read only
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    size_t bytesRead;
    unsigned char *buffer = (unsigned char *)malloc(CHUNK_SIZE);
    // Read and print the contents of the file
    do {
        bytesRead = fread(buffer, 1, CHUNK_SIZE, file);
        if (bytesRead > 0)
        {
            fwrite(buffer, 1, bytesRead, stdout);
        }
    } while (bytesRead > 0);
    free(buffer);
    fclose(file);//close the file
}



