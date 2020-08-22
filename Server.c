#include<windows.h>
#include<stdio.h>
#include<malloc.h>

typedef struct cookie
{
const char *key;
char *value;
int maxAge;
}COOKIE;

typedef struct socket
{
int portNumber;
char *ipAddress;
int descriptor;
struct sockaddr_in address;
}TMSOCKET;

typedef struct webServer
{
TMSOCKET* socket;
void * (*processor)(const char *);
}SERVER_SOCKET;

typedef struct request
{
char buffer[10000];
char queryString[1000];
char *folderName;
char *fileName;
char *path;
char  *cookies;
int isGet;
int isPost;
}REQUEST;

typedef struct response
{
char buffer[10000];
char *header;
char *status;
int errorCode;
char *mimeType;
char  *cookies;
TMSOCKET* clientDescriptor;
}RESPONSE;

COOKIE * createCookie(char *key,char *value)
{
COOKIE *cookie=(COOKIE *)malloc(sizeof(COOKIE));
cookie->key=key;
cookie->value=value;
}
void setMaxAge(COOKIE *cookie,int maxAge)
{
cookie->maxAge=maxAge;
}
void addCookie(RESPONSE *response,COOKIE *cookie)
{
if(response->cookies==NULL)
{
response->cookies=(char *)malloc(sizeof(char)*10000);
response->cookies[0]='\0';
}
char cookieString[251];
sprintf(cookieString,"Set-Cookie: %s=%s; Max-Age=%d\n",cookie->key,cookie->value,cookie->maxAge);
strcat(response->cookies,cookieString);
}
char * getCookieValue(REQUEST *request,const char * key)
{
char *searchString=(char *)malloc(strlen(key)*sizeof(char)+2);
strcpy(searchString,key);
searchString[strlen(key)]='=';
searchString[strlen(key)+1]='\0';
char *r=strstr(request->cookies,searchString);
if(r==NULL)
{
free(searchString);
return NULL;
}
int i=strlen(searchString);
char *result=(char *)malloc(sizeof(1000));
int x=0;
printf("hello1");
while(r[i]!=';' && r[i]!='\0' && r[i]!='\n')
{
result[x++]=r[i++];
}
result[x]='\0';
return result;
}

char * getString(REQUEST* request,char *query)
{
char* q=(char *)malloc((strlen(query)+2)*(sizeof(char)));
strcpy(q,query);
strcat(q,"=");
q[strlen(query)+1]='\0';
char *p=strstr(request->queryString,q);
if(!p) return NULL;
int l=strlen(q);
int i=l;
while(p[i]!='&' && p[i]!='\0') i++;
char *result=(char *)malloc((i-l+1)*sizeof(char));
int x=0;
while(l<i) result[x++]=p[l++];
result[x]='\0';
free(p);
return result;
}

int getInt(REQUEST* request,char* query)
{
return atoi(getString(request,query));
}


void setMimeType(RESPONSE *response,const char *mimeType)
{
free(response->mimeType);
response->mimeType=(char *)malloc(strlen(mimeType)*sizeof(char));
strcpy(response->mimeType,mimeType);
}


void processFile(REQUEST* request,RESPONSE* response)
{
FILE* file;
file=fopen(request->path,"r");
if(file==NULL)
{
response->errorCode=404;
response->status="PAGE NOT FOUND";
return;
}
char data[100];
int x;
fseek(file,0,SEEK_SET);
while (fgets(data,100,file)!=NULL)
{
strcat(response->buffer,data);
}
fclose(file);
}


void tcpRead(TMSOCKET* clientSocket,REQUEST* request)
{
request->buffer[0]='\0';
int bytesReceived=recv(clientSocket->descriptor,request->buffer,sizeof(request->buffer),0);
if(bytesReceived<0)
{
printf("Unable to extract request data..\n");
closesocket(clientSocket->descriptor);
return;
}
request->buffer[bytesReceived]='\0';
}

void parseRequest(REQUEST* request,RESPONSE *response)
{
char *requestBuffer=request->buffer;
request->cookies=(char *)malloc(2000*sizeof(char));
if(requestBuffer[0]=='G')
{
request->isGet=1;
request->isPost=0;
}
else
{
request->isPost=1;
request->isGet=0;
}
printf("%s*************************",request->buffer);
char filePath[1000];
char fileName[100];
char folderName[100];
char *str=strchr(requestBuffer,'/');
int index=strcspn(str," ");
for(int i=1;i<index;i++) filePath[i-1]=str[i];
filePath[index-1]='\0';
request->path=filePath;
if(strlen(filePath)==0)
{
response->errorCode=400;
response->status="BAD REQUEST";
return;
}
index=strcspn(filePath,"/");
if(index>=strlen(filePath))
{
strcpy(folderName,filePath);
const char * s="index.html";
strcpy(fileName,s);
sprintf(filePath,"%s/%s",folderName,fileName);
request->folderName=(char *)malloc(1000*(sizeof(char)));
strcpy(request->folderName,folderName);
request->fileName=(char *)malloc(1000*(sizeof(char)));
strcpy(request->fileName,fileName);
request->path=(char *)malloc(1000*(sizeof(char)));
strcpy(request->path,filePath);
request->cookies=strstr(requestBuffer,"Cookie: ");
return;
}
else
{
for(int i=0;i<index;i++) folderName[i]=filePath[i];
folderName[index]='\0';
}
request->folderName=folderName;
if(strlen(folderName)==0)
{
response->errorCode=400;
response->status="BAD REQUEST";
return;
}

str=strchr(filePath,'/');
if(str[0]=='/' && strlen(str)==1)
{
const char * s="index.html";
strcpy(fileName,s);
}
else
{
for(int i=0;i<strlen(str);i++) fileName[i]=str[i+1];
fileName[strlen(str)]='\0';
}
request->fileName=fileName;
if(strlen(fileName)==0)
{
const char *s="index.html";
strcpy(fileName,s);
}
request->folderName=(char *)malloc(1000*(sizeof(char)));
strcpy(request->folderName,folderName);
request->fileName=(char *)malloc(1000*(sizeof(char)));
strcpy(request->fileName,fileName);
request->path=(char *)malloc(1000*(sizeof(char)));
strcpy(request->path,filePath);
request->cookies=strstr(requestBuffer,"Cookie: ");
return;
}

void tcpClose(TMSOCKET *clientSocket)
{
closesocket(clientSocket->descriptor);
free(clientSocket->ipAddress);
free(clientSocket);
}

void tcpEnd(TMSOCKET *serverSocket)
{
closesocket(serverSocket->descriptor);
free(serverSocket->ipAddress);
free(serverSocket);
WSACleanup();
}

void freeRequest(REQUEST *request)
{
free(request->folderName);
free(request->fileName);
free(request->path);
free(request->buffer);
free(response->cookies);
free(request);
}

void freeResponse(RESPONSE *response)
{
free(response->buffer);
free(response->mimeType);
free(response->status);
free(response->header);
free(response->cookies);
free(response);
}

void destroyCookie(COOKIE *cookie)
{
free(cookie->value);
free(cookie);
}

void tcpWrite(RESPONSE* response)
{
int bytesSent=0;
if(response->header==NULL)
{
response->header=(char *)malloc(10000*sizeof(char));
if(response->cookies==NULL)
{
sprintf(response->header,"HTTP/1.1 %d %s\nContent-Type: %s\r\n\n",response->errorCode,response->status,response->mimeType);
}
else
{
sprintf(response->header,"HTTP/1.1 %d %s\nContent-Type: %s\nCookies: %s\r\n\n",response->errorCode,response->status,response->mimeType,response->cookies);
}
bytesSent+=send(response->clientDescriptor->descriptor,response->header,strlen(response->header),0);
if(bytesSent<0)
{
printf("Unable to send response isame\n");
closesocket(response->clientDescriptor->descriptor);
return;
}
}
bytesSent+=send(response->clientDescriptor->descriptor,response->buffer,strlen(response->buffer),0);
if(bytesSent<0)
{
printf("Unable to send response usame\n");
closesocket(response->clientDescriptor->descriptor);
return;
}
closesocket(response->clientDescriptor->descriptor);
}

void put(RESPONSE* response,char * data)
{
strcpy(response->buffer,data);
tcpWrite(response);
}

void startServer(SERVER_SOCKET * serverSocket)
{
TMSOCKET* socket=serverSocket->socket;
RESPONSE* response=NULL;
REQUEST* request=NULL;
while(1)
{
response=(RESPONSE *)malloc(sizeof(RESPONSE));
request=(REQUEST *)malloc(sizeof(REQUEST));
TMSOCKET * clientSocket=(TMSOCKET *)malloc(sizeof(TMSOCKET));
listen(socket->descriptor,SOMAXCONN);
printf("Server is in listening mode........\n");
int clientAddressLength=sizeof(clientSocket->address);
int clientDescriptor=accept(socket->descriptor,(struct sockaddr*)&clientSocket->address,&clientAddressLength);
if(clientDescriptor<0)
{
printf("Client connected, but unable to accept client\n");
continue;
}
clientSocket->descriptor=clientDescriptor;
printf("Client connected.\n");
struct in_addr clientIPAddress;
memcpy(&clientIPAddress,&clientSocket->address.sin_addr.s_addr,4);
char *clientIP=inet_ntoa(clientIPAddress);
clientSocket->ipAddress=clientIP;
int clientSocketPortNumber=ntohs(clientSocket->address.sin_port);
printf("Client IP : %s, connected on port : %d\n",clientIP,clientSocketPortNumber);
clientSocket->portNumber=clientSocketPortNumber;
response->clientDescriptor=clientSocket;
response->errorCode=200;
response->status="OK";
response->header=NULL;
response->buffer[0]='\0';
response->mimeType="text/html";
tcpRead(clientSocket,request);
printf("(%d)\n",strlen(request->buffer));
if(strlen(request->buffer)<=1)
{
printf("isamea\n");
continue;
}
parseRequest(request,response);
char *c = strrchr(request->path, '.');
if(!(c && !strcmp(c,".ss")))
{
processFile(request,response);
printf("%s",response->buffer);
tcpWrite(response);
printf("prakash");
}
else
{
void (*function)(REQUEST*,RESPONSE*);
printf("RESOURCE NAME :: %s\n",request->fileName);
function=(void (*)(REQUEST* request,RESPONSE* response))serverSocket->processor(request->fileName);
if(function==NULL)
{
response->errorCode=404;
response->status="PAGE NOT FOUND";
}
else function(request,response);
}
freeRequest(request);
freeResponse(response);
tcpClose(clientSocket);
}
tcpEnd(serverSocket->socket);
}


TMSOCKET* initSocket(char * IP,int portNumber)
{
TMSOCKET *socketServer=(TMSOCKET *)malloc(sizeof(TMSOCKET));
socketServer->portNumber=portNumber;
socketServer->ipAddress=IP;
WSADATA winsockData;
WORD winsockVersion=MAKEWORD(1,1);
int winsockState=WSAStartup(winsockVersion,&winsockData);
if(winsockState!=0)
{
printf("Unable to initialize winsock library\n");
return NULL;
}
int serverDescriptor=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
if(serverDescriptor<0)
{
printf("Unable to create socket\n");
WSACleanup();
return NULL;
}
socketServer->descriptor=serverDescriptor;
struct sockaddr_in serverAddress;
serverAddress.sin_family=AF_INET;
serverAddress.sin_port=htons(portNumber);
serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
socketServer->address=serverAddress;
int bindState=bind(serverDescriptor,(struct sockaddr *)&serverAddress,sizeof(struct sockaddr));
if(bindState<0)
{
printf("Unable to bind socket to %d\n",portNumber);
WSACleanup();
return NULL;
}
return socketServer;
}

SERVER_SOCKET *initServer(char * IP,int portNumber,void * (*p)(const char *url))
{
SERVER_SOCKET* serverSocket=(SERVER_SOCKET *)malloc(sizeof(SERVER_SOCKET));
serverSocket->socket=initSocket(IP,portNumber);
serverSocket->processor=p;
}

//********************************************************************************

void ABCDE(REQUEST* request,RESPONSE* response)
{
COOKIE *cookie=createCookie("firstcookie","ho gya");
COOKIE *cookie1=createCookie("secondcookie","ho gya");
printf("cookie create ho gya\n");
addCookie(response,cookie);
addCookie(response,cookie1);
put(response,"RESPONSE ABCDE SE AAYA");
printf("ABCDE CHALA !!\n");
char *a=getCookieValue(request,"firstcookie");
printf("\ncookie value:%s\n",a);
}

void * serviceResolver(const char *url)
{
printf("done");
if(strcmp(url,"aaa.ss")==0)
{
return ABCDE;
}
return NULL;
}

int main()
{
SERVER_SOCKET* serverSocket=initServer("localhost",8080,serviceResolver);
startServer(serverSocket);
return 0;
}
