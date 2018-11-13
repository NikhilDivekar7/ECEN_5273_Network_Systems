#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MAXSIZE 1024000
#define KEEP_ALIVE 1
#define PIPELINE 1

int default_page = 0;

int sock, client_sock;;
struct sockaddr_in server_address,client_address;
socklen_t clilen = sizeof(client_address);
int count_value;
struct timeval time_val;
struct hostent *hostp;
int time_optval;

char buffer[MAXSIZE];
char buffer1[MAXSIZE];
char complete_msg[MAXSIZE];

int received_bytes = 0;
int sent_bytes = 0;

int get_flag = 1;
int post_flag = 1;

char original_msg[MAXSIZE];
char pipeline_msg[MAXSIZE];

char * pipeline_buffer;
char * command;
char main_command[MAXSIZE];
char main_protocol[MAXSIZE];
char * file_name;
char * protocol;
char * test1;
char * post_data;

char internal_error[] =
"HTTP/1.1 500 internal server error\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 500:INTERNAL SERVER </h1><br>\r\n";

size_t buffer_size;
int conn;
FILE *fptr;
size_t fsize;
char header_buffer[MAXSIZE];
char * format_buffer;
char * post_value_1;
char * post_value_2;
char file_format[20];
int buffer_a,buffer_a1;
char *data_value;

//Function that handles multiple clients
void new_client_request(int conn)
{
pipeline_goto:
  bzero(buffer,MAXSIZE);
  received_bytes = read(conn, buffer,MAXSIZE);

  if(received_bytes <= 0)
  {
    printf("[SERVER] Error reading data\n");
    close(conn);
    return;
  }
  else
  {
    printf("[SERVER] String received from client: \n%s\n", buffer);

    //Removing null terminals
    int j = 0;
    for(int i = 0; i<received_bytes; i++)
    {
      if(buffer[i] != '\0')
      {
        pipeline_msg[j]=buffer[i];
        j++;
      }
    }
    pipeline_msg[j] = '\0';

    strcpy(original_msg, buffer);

    command = strtok(buffer," \n");
    file_name = strtok(NULL," \n");
    protocol = strtok(NULL," \n");

    strcpy(main_command, command);
    strcpy(main_protocol, protocol);

    printf("[SERVER] Command is %s\n",command);
    if(strncmp(command, "GET", 3) == 0)
    {
      printf("[SERVER] We are in post \n");
      get_flag = 1;
      post_flag = 0;
    }
    else if(strncmp(command, "POST", 4) == 0)
    {
      printf("[SERVER] We are in post \n");
      get_flag = 0;
      post_flag = 1;
    }

    printf("[SERVER] Request is %s\n",file_name);

    printf("[SERVER] Protocol is %s\n",protocol);
  }

  //Default page
  if(strncmp(command,"GET",3)==0 && strcmp(file_name, "/") == 0)
  {
    printf("[SERVER] Default page \n");

    char index_page[MAXSIZE] = "/home/nikhil/NetSys/PA2/www/www/index.html";

    fptr =fopen(index_page,"r");

    if (fptr==NULL)
    {
      printf("[SERVER] ERROR: Invalid File Request Received\n");
      buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] File found\n");
    }
    fseek (fptr , 0 , SEEK_END);
    fsize = ftell (fptr);
    fseek (fptr , 0 , SEEK_SET);

    sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/html",fsize);

    sent_bytes =write(conn, header_buffer, strlen(header_buffer));

    if (sent_bytes<=0)
    {
      printf("[SERVER] Header buffer not Sent\n");
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] Header buffer Sent\n");
    }
    bzero(header_buffer,sizeof(header_buffer));
    buffer_size = fread(buffer,1,MAXSIZE,fptr);
    buffer_a=write(conn, buffer, buffer_size);
    bzero(buffer,sizeof(buffer));
    fclose(fptr);
    close(conn);
  }

  if(strncmp(command,"POST\0",5)==0)
  {
    int j = 0;
    for(int i = 0; i<received_bytes; i++)
    {
      if(buffer[i] != '\0')
      {
        complete_msg[j]=buffer[i];
        j++;
      }
    }
    complete_msg[j] = '\0';

    printf("[SERVER] Full message: %s \n", complete_msg);

    test1 = strstr(complete_msg,"Connection");

    printf("[SERVER] Connection string: %s \n", test1);

    printf("[SERVER] Command is POST \n");

    data_value = test1 + 24;
    
    printf("[SERVER] Data to be posted: %s\n",data_value);

    sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n<html><body><pre><h1>%s</h1></pre>\r\n", "text/html",(int)strlen(data_value),data_value);
    
    printf("[SERVER] Header value sent is:%s\n",header_buffer);
    
    send(conn,header_buffer,strlen(header_buffer),0);

    char original_dir[MAXSIZE] = "/home/nikhil/NetSys/PA2/www/www";

    strcat(original_dir, file_name);

    printf("%s \n", original_dir);

    fptr =fopen(original_dir,"r");

    if (fptr==NULL)
    {
      printf("[SERVER] ERROR: Invalid File Request Recieved\n");
      buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] File found\n");
    }
    fseek (fptr , 0 , SEEK_END);
    fsize = ftell (fptr);
    fseek (fptr , 0 , SEEK_SET);

    format_buffer = strrchr(file_name,'.');
    strcpy(file_format,format_buffer);

    //checking extension
    if (strcmp(file_format,".html")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/html",fsize);
    }
    else if(strcmp(file_format,".txt")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/plain",fsize);
    }
    else if(strcmp(file_format,".png")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/png",fsize);
    }
    else if(strcmp(file_format,".gif")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/gif",fsize);
    }
    else if(strcmp(file_format,".jpg")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/jpg",fsize);
    }
    else if(strcmp(file_format,".css")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/css",fsize);
    }
    else if(strcmp(file_format,".js")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","application/javascript",fsize);
    }
    else
    {
      printf("[SERVER] Error: Invalid file file_format\n");
      buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
      close(conn);
      return;
    }
    
    printf("[SERVER] Header sent is %s\n",header_buffer);
    sent_bytes =write(conn, header_buffer, strlen(header_buffer));
    if (sent_bytes<=0)
    {
      printf("[SERVER] Header buffer not Sent\n");
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] Header buffer Sent\n");
    }
    bzero(header_buffer,sizeof(header_buffer));
    buffer_size = fread(buffer,1,MAXSIZE,fptr);
    buffer_a=write(conn, buffer, buffer_size);
    bzero(buffer,sizeof(buffer));
    fclose(fptr);
    close(conn);
  }
  //Checking commands
  if(strncmp(main_command,"GET",3)==0)
  {
    printf("[SERVER] Command is GET \n");
  }
  else if(strncmp(main_command,"POST",4)==0)
  {
    printf("[SERVER] Command is POST \n");
  }
  else
  {
    printf("[SERVER] Error: Invalid Command\n");
    buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
    close(conn);
    return;
  }

  //checking protocols
  printf("Protocol is %s\n", protocol);
  if((strncmp(main_protocol,"HTTP/1.1",8)==0) || (strncmp(main_protocol,"HTTP/1.0",8)==0))
  {
    printf("[SERVER] Proper HTTP protocol \n");
  }
  else
  {
    printf("[SERVER] Error: Invalid Protocol \n");
    buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
    close(conn);
    return;
  }
  if(strncmp(command,"GET",3)==0)
  {
    printf("[SERVER] In get \n");

    char dir[MAXSIZE] = "/home/nikhil/NetSys/PA2/www/www";

    strcat(dir,file_name);
    printf("[SERVER] File-path is %s\n",dir);

    printf("[SERVER] This is you should read : %s\n", file_name);

    fptr =fopen(dir,"r");

    if (fptr==NULL)
    {
      printf("[SERVER] ERROR: Invalid File Request Recieved\n");
      buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] File found\n");
    }
    fseek (fptr , 0 , SEEK_END);
    fsize = ftell (fptr);
    fseek (fptr , 0 , SEEK_SET);

    format_buffer = strrchr(file_name,'.');
    strcpy(file_format,format_buffer);

    if (strcmp(file_format,".html")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/html",fsize);
    }
    else if(strcmp(file_format,".txt")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/plain",fsize);
    }
    else if(strcmp(file_format,".png")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/png",fsize);
    }
    else if(strcmp(file_format,".gif")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/gif",fsize);
    }
    else if(strcmp(file_format,".jpg")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","image/jpg",fsize);
    }
    else if(strcmp(file_format,".css")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","text/css",fsize);
    }
    else if(strcmp(file_format,".js")==0)
    {
      sprintf(header_buffer,"HTTP/1.1 200 DOCUMENT FOLLOWS\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n","application/javascript",fsize);
    }
    else
    {
      printf("[SERVER] Error: Invalid file file_format\n");
      buffer_a1 = send(conn,internal_error, strlen(internal_error), 0);
      close(conn);
      return;
    }
    
    printf("Header sent is %s\n",header_buffer);
    sent_bytes=write(conn, header_buffer, strlen(header_buffer));
    if (sent_bytes<=0)
    {
      printf("[SERVER] Header buffer not Sent\n");
      close(conn);
      return;
    }
    else
    {
      printf("[SERVER] Header buffer Sent\n");
    }
    bzero(header_buffer,sizeof(header_buffer));
    buffer_size = fread(buffer,1,MAXSIZE,fptr);
    printf("[SERVER] Buffer: %s \n", buffer);
    buffer_a = send(conn, buffer, buffer_size,0);
    printf("[SERVER] Number of bytes sent %d \n", buffer_a);
    bzero(buffer,sizeof(buffer));
    fclose(fptr);

    // Pipelining here
    if(PIPELINE == 1)
    {
      printf("[SERVER] Original message :%s\n",pipeline_msg);
      pipeline_buffer = strstr(pipeline_msg,"Connection: keep-alive");
      printf("[SERVER] Pattern matched: %s\n",pipeline_buffer);

      if(pipeline_buffer != NULL)
      {
        printf("[SERVER] In pipeline, Waiting for some time \n");
        time_val.tv_sec = 15;
        int timeout_val = setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&time_val, sizeof(struct timeval));
        printf("[SERVER] Value: %d\n",timeout_val);
        printf("[SERVER] Entering pipelining \n");
        count_value++;
        printf("[SERVER] Pipeline Counter: %d \n",count_value);
        goto pipeline_goto;
      }
      else
      {
        printf("[SERVER] Pipeline closed \n");
        time_val.tv_sec = 0;
        int timeout_val1 = setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&time_val, sizeof(struct timeval));
        printf("[SERVER] Value: is %d",timeout_val1);
        count_value = 0;
      }
    }
    close(conn);
  }
}


int main (int argc, char * argv[])
{
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(atoi(argv[1]));  
  server_address.sin_addr.s_addr = INADDR_ANY;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
  {
    printf("[SERVER] Socket creation failed \n");
  }
  else
  {
    printf("[SERVER] Socket creation successful \n");
  }

  //set timer
  time_optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&time_optval , sizeof(int));

  //bind address
  if(bind(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    printf("[SERVER] Unable to bind socket\n");
  }
  else
  {
    printf("[SERVER] Binded Successfully!\n");
  }

  //listen to incoming connections
  listen (sock, 10);

  printf("[SERVER] Server ready, waiting for Client \n");
  
  while(1)
  {
    client_sock = accept (sock, (struct sockaddr *) &client_address, &clilen);

    if (client_sock < 0)
    {
      printf("[SERVER] Client not accepted \n");
      exit(1);
    }
    else
    {
      // Multithreading using fork
      if(!fork())
      {
        printf("[SERVER] New client request \n");
        new_client_request(client_sock);
        exit(1);
      }
      else
      {
        close(client_sock);
        printf("[SERVER] Child closed\n");
      }
    }
  }
}
