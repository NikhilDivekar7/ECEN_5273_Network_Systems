#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define SEND_PACKET 512
#define RECEIVE_PACKET 512
#define TIME_INTERVAL 3

int main (int argc, char * argv[] )
{
  printf("[SERVER] Getting things started \n");
  int sock;
  int nbytes,sent_bytes,received_bytes;
  int count, j, i = 0;
  int num_in_fsize = 0;
  int fs_copy;
  int current_packet_id = 0;
  int actual_packet_num = 1;
  int allow_next_packet = 0;
  int packet_modulus_value = 0;
  int nack_counter = 0;
  int expected_packet_num = 0;
  int current_packet = 1;
  int received_iteration = 0;
  int current_iteration = 0;
  int fsize;

  struct sockaddr_in sin, remote;
  int remote_length = sizeof(remote);
  FILE *fp;

  static int total_iterations_left;
  int packet_size;
  char send_buffer[SEND_PACKET];
  char buffer[100];
  char receive_buffer[RECEIVE_PACKET]; 
  char file_name[100];
  char file_suffix[100];
  char packet_id[2];
  char retval[10];
  char split_command[3][3];
  char received_iter[10];

  if(argc != 2)
  {
    printf ("[SERVER] You are missing out on port number \n");
    exit(1);
  }
  else
  {
    printf("[SERVER] Proceeding with socket connection \n");
  }

  bzero(&sin,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(atoi(argv[1]));
  sin.sin_addr.s_addr = INADDR_ANY;


  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    printf("[SERVER] Socket creation failed \n");
  }
  else
  {
    printf("[SERVER] Socket creation successful \n");
  }
  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    printf("[SERVER] Socket binding failed \n");
  }
  else
  {
    printf("[SERVER] Socket binding successful \n");
  }
  struct timeval to = {TIME_INTERVAL,0};
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, (char *)&to, sizeof(to));
  
  while(1)
  {
    actual_packet_num = 1;
    current_packet_id = 0;
    current_packet = 1;
    expected_packet_num = 0;
    allow_next_packet = 0;
    packet_modulus_value = 0;

    bzero(receive_buffer,RECEIVE_PACKET);
    received_bytes = recvfrom(sock,(char *)receive_buffer,RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
    receive_buffer[received_bytes] = '\0';

    if(received_bytes < 0)
    {
      printf("[SERVER] Error while recieving data \n");
    }

    printf("[SERVER] Number of bytes received: %d \n",received_bytes);
    printf("[SERVER] Command received from Client: %s\n", receive_buffer);

    if(strncmp(receive_buffer, "get",3)==0)
    {
      j = 0; 
      count = 0;
      for(i = 0; i < strlen(receive_buffer); i++)
      {
        if(receive_buffer[i] == ' ' || receive_buffer[i] =='\0')
        {
          split_command[count][j] ='\0';
          count ++;
          j = 0;
        }
        else
        {
          split_command[count][j] = receive_buffer[i];
          j++;
        }
      }
      strcpy(file_name, split_command[1]);
      bzero(send_buffer, SEND_PACKET); 
      printf("[SERVER] File name: %s\n",file_name);
      fp = fopen(file_name, "r");
      if(fp == NULL)
      {
        printf("[SERVER] File not found\n");
        bzero(send_buffer, SEND_PACKET);
        strcpy(send_buffer, "File not found");
        sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
      }
      else
      {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        printf("[SERVER] Total File size = %d\n", fsize);
        fs_copy = fsize;

        while(fs_copy != 0)
        {
          num_in_fsize++;
          fs_copy = fs_copy/10;
        }

        printf("[SERVER] Number of digits in file size : %d \n", num_in_fsize);
        bzero(send_buffer,SEND_PACKET);
        sprintf(send_buffer,"%d",fsize);
        printf("[SERVER] File size sent value is : %s \n",send_buffer);
        allow_next_packet = 0;
        while(allow_next_packet != 1)
        {
          sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
          if(sent_bytes < 0)
          {
            printf("[SERVER] Error while sending size\n");
          }
          bzero(receive_buffer, RECEIVE_PACKET);
          received_bytes = recvfrom(sock,(char *)receive_buffer, RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
          receive_buffer[received_bytes] = '\0';
          if(strncmp(receive_buffer,"ack",3)==0)
          {
            allow_next_packet = 1;
          }
          else if(strncmp(receive_buffer,"nack",3)==0)
          {
            allow_next_packet = 0;
          }
        }
        if(fsize < (SEND_PACKET-1))
        {
          total_iterations_left = 1;
        }
        else
        {
          total_iterations_left = 1 + fsize/(SEND_PACKET-1);
        }
        while(total_iterations_left != 0)
        {

          printf("[SERVER] On to the next iteration \n");
          for(int i = 0; i<100000; i++);
          if(total_iterations_left == 1)
          {
            packet_size = fsize%(SEND_PACKET-1);
          }
          else
          {
            packet_size = (SEND_PACKET-1);
          }
          bzero(send_buffer,SEND_PACKET);
          fread(send_buffer, 1, packet_size, fp);

          if(current_packet_id != 9)
            current_packet_id++;
          else
            current_packet_id = 1;

          sprintf(packet_id,"%d",current_packet_id);
          printf("{SERVER] Current packet number : %d \n", actual_packet_num);
          printf("[SERVER] Current packet ID : %d \n", current_packet_id);
          send_buffer[packet_size] = packet_id[0];
          allow_next_packet = 0;
          while(allow_next_packet != 1)
          {
            sent_bytes = sendto(sock, send_buffer, (packet_size+1), 0, (const struct sockaddr *)&remote, remote_length);
            if(sent_bytes < 0)
            {
              printf("[SERVER] Error while sending data \n");
            }
            bzero(receive_buffer, RECEIVE_PACKET);
            received_bytes = recvfrom(sock,(char *)receive_buffer, RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
            receive_buffer[received_bytes] = '\0';
            printf("[SERVER] Nack counter : %d \n", nack_counter);
            if(strncmp(receive_buffer,"ack",3)==0)
            {
              allow_next_packet = 1;
              printf("[SERVER] Iteration number = %d\n",total_iterations_left);
              actual_packet_num++;
            }
            else if(strncmp(receive_buffer,"nack",4)==0)
            {
              nack_counter++;
              strcpy(received_iter ,receive_buffer+4);
              current_iteration = atoi(received_iter);
              printf("[SERVER] Nack received for iteration number: %d\n", total_iterations_left);
              if(current_iteration == total_iterations_left)
              {  
                allow_next_packet = 0; 
                sprintf(packet_id,"%d",current_packet_id);
                send_buffer[packet_size] = packet_id[0];
              }
              else if(current_iteration == total_iterations_left-1)
              {
                break;
              }
            }
          }
          bzero(send_buffer, SEND_PACKET); 
          total_iterations_left--;
        }
        printf("[SERVER] Transfer complete \n");
        fclose(fp);
      }
    }
    else if(strncmp(receive_buffer, "put",3)==0)
    {
      bzero(file_name, 100);
      j = 0; 
      count = 0;
      for(i = 0; i < strlen(receive_buffer); i++)
      {
        if(receive_buffer[i] == ' ' || receive_buffer[i] =='\0')
        {
          split_command[count][j] ='\0';
          count ++;
          j = 0;
        }
        else
        {
          split_command[count][j] = receive_buffer[i];
          j++;
        }
      }
      strcpy(file_name, split_command[1]);
      bzero(receive_buffer,RECEIVE_PACKET);
      printf("[SERVER] Waiting for client \n");
      allow_next_packet = 0;

      received_bytes = recvfrom(sock,(char *)receive_buffer, RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
      if(received_bytes < 0)
      {
        printf("Error while recieving data \n");
      }
      else if(strncmp(receive_buffer,"File not found",14)==0)
      {
        printf("[SERVER] File not found\n");
      }
      else
      {
        allow_next_packet = 0;
        receive_buffer[received_bytes] = '\0';
        if(received_bytes < 0)
        {
          printf("Error while recieving data \n");
        }
        printf("Total File size = %s\n", receive_buffer);
        fsize = atoi(receive_buffer);
        printf("File size : %d \n", fsize);

        if(fsize < SEND_PACKET-1)
        {
          total_iterations_left = 1;
        }
        else
        {
          total_iterations_left = 1 + fsize/(SEND_PACKET-1);
        }

        printf("[SERVER] Iteration left = %d\n", total_iterations_left);
        bzero(receive_buffer, RECEIVE_PACKET);

        strcpy(file_suffix,"put_");
        strcat(file_suffix,file_name);
        printf("[SERVER] File_name is: %s\n",file_suffix);
        fp = fopen(file_suffix, "w+");
        if(fp == NULL)
        {
          printf("File not found\n");
        }
        while(total_iterations_left != 0)
        {
          if(total_iterations_left  == 1)
          {
            packet_size = fsize%(RECEIVE_PACKET-1);
          }
          else
          {
            packet_size = (RECEIVE_PACKET-1);
          }
          allow_next_packet = 0;

          if(expected_packet_num != 9)
            expected_packet_num++;
          else
            expected_packet_num = 1;

          printf("[SERVER] Expected packet ID : %d \n", expected_packet_num);
          printf("[SERVER] Iteration left : %d \n" , total_iterations_left);

          while(allow_next_packet != 1)
          {
            bzero(receive_buffer, RECEIVE_PACKET);
            received_bytes = recvfrom(sock,(char *)receive_buffer, (packet_size+1), 0, (struct sockaddr *)&remote, &remote_length);
            packet_modulus_value = atoi(&receive_buffer[received_bytes-1]);
            if(received_bytes < 0)
            {
              sprintf(retval,"%d",total_iterations_left);
              strcpy(send_buffer,"nack");
              strcat(send_buffer,retval);
              printf("In first if \n");
              printf("[SERVER] Sending nack value: %s\n", send_buffer);
              printf("[SERVER] Expected packet ID: %d Current packet ID : %d \n", expected_packet_num, packet_modulus_value);
              sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
            }
            else if(expected_packet_num == packet_modulus_value)
            {
              strcpy(send_buffer,"ack");
              printf("[SERVER] Sending acknowledgement for packet number: %d \n",current_packet);
              sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
              current_packet++;
              fwrite(receive_buffer,1,packet_size,fp);
              break;
            }
            else if(expected_packet_num != packet_modulus_value)
            {
              printf("In Second if");
              sprintf(retval,"%d",total_iterations_left);
              strcpy(send_buffer,"nack");
              strcat(send_buffer,retval);
              printf("Sending nack value: %s\n", send_buffer);
              printf("Expected packet ID: %d Current packet ID : %d \n", expected_packet_num, packet_modulus_value);
              printf("Iterations ledt : %d", total_iterations_left);
              sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
              /*if(expected_packet_num != packet_modulus_value)
              {
                expected_packet_num = packet_modulus_value;
              }*/
            }
          }
          bzero(receive_buffer, RECEIVE_PACKET);
          total_iterations_left--;
        }
        printf("[SERVER] Transfer done\n");
        fclose(fp);
      }
    }
    
    else if(strncmp(receive_buffer, "delete",6)==0)
    {
      bzero(file_name, 100);
      j = 0; 
      count = 0;
      for(i = 0; i < strlen(receive_buffer); i++)
      {
        if(receive_buffer[i] == ' ' || receive_buffer[i] =='\0')
        {
          split_command[count][j] ='\0';
          count ++;
          j = 0;
        }
        else
        {
          split_command[count][j] = receive_buffer[i];
          j++;
        }
      }
      strcpy(file_name, split_command[1]);
      printf("File to be deleted is %s \n", file_name);
      remove(file_name);
      printf("[SERVER] File deleted successfully \n");
    }

    else if(strncmp(receive_buffer, "exit",4)==0)
    {
      printf("[SERVER] Exiting the Server \n");
      close(sock);
      exit(1);
    }

    else if(strncmp(receive_buffer, "ls", 1)==0)
    {
      printf("[SERVER] ls to be implemented \n");
      system("ls -a > list.txt");
      current_packet_id = 0;

      fp = fopen("list.txt", "r");
      if(fp == NULL)
      {
        printf("[SERVER] File not found\n");
        bzero(send_buffer, SEND_PACKET);
        strcpy(send_buffer, "File not found");
        sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
      }
      else
      {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        printf("[SERVER] Total File size = %d\n", fsize);
        fs_copy = fsize;

        while(fs_copy != 0)
        {
          num_in_fsize++;
          fs_copy = fs_copy/10;
        }

        printf("[SERVER] Number of digits in file size : %d \n", num_in_fsize);
        bzero(send_buffer,SEND_PACKET);
        sprintf(send_buffer,"%d",fsize);
        printf("[SERVER] File size sent value is : %s \n",send_buffer);
        allow_next_packet = 0;
        while(allow_next_packet != 1)
        {
          sent_bytes = sendto(sock, send_buffer, strlen(send_buffer), 0, (const struct sockaddr *)&remote, remote_length);
          if(sent_bytes < 0)
          {
            printf("[SERVER] Error while sending size\n");
          }
          bzero(receive_buffer, RECEIVE_PACKET);
          received_bytes = recvfrom(sock,(char *)receive_buffer, RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
          receive_buffer[received_bytes] = '\0';
          if(strncmp(receive_buffer,"ack",3)==0)
          {
            allow_next_packet = 1;
          }
          else if(strncmp(receive_buffer,"nack",3)==0)
          {
            allow_next_packet = 0;
          }
        }
        if(fsize < (SEND_PACKET-1))
        {
          total_iterations_left = 1;
        }
        else
        {
          total_iterations_left = 1 + fsize/(SEND_PACKET-1);
        }
        while(total_iterations_left != 0)
        {

          printf("[SERVER] On to the next iteration \n");
          for(int i = 0; i<100000; i++);
          if(total_iterations_left == 1)
          {
            packet_size = fsize%(SEND_PACKET-1);
          }
          else
          {
            packet_size = (SEND_PACKET-1);
          }
          bzero(send_buffer,SEND_PACKET);
          fread(send_buffer, 1, packet_size, fp);

          if(current_packet_id != 9)
            current_packet_id++;
          else
            current_packet_id = 1;

          sprintf(packet_id,"%d",current_packet_id);
          printf("{SERVER] Current packet number : %d \n", actual_packet_num);
          printf("[SERVER] Current packet ID : %d \n", current_packet_id);
          send_buffer[packet_size] = packet_id[0];
          allow_next_packet = 0;
          while(allow_next_packet != 1)
          {
            sent_bytes = sendto(sock, send_buffer, (packet_size+1), 0, (const struct sockaddr *)&remote, remote_length);
            if(sent_bytes < 0)
            {
              printf("[SERVER] Error while sending data \n");
            }
            bzero(receive_buffer, RECEIVE_PACKET);
            received_bytes = recvfrom(sock,(char *)receive_buffer, RECEIVE_PACKET, 0, (struct sockaddr *)&remote, &remote_length);
            receive_buffer[received_bytes] = '\0';
            printf("[SERVER] Nack counter : %d \n", nack_counter);
            if(strncmp(receive_buffer,"ack",3)==0)
            {
              allow_next_packet = 1;
              printf("[SERVER] Iteration number = %d\n",total_iterations_left);
              actual_packet_num++;
            }
            else if(strncmp(receive_buffer,"nack",4)==0)
            {
              nack_counter++;
              strcpy(received_iter ,receive_buffer+4);
              current_iteration = atoi(received_iter);
              printf("[SERVER] Nack received for iteration number: %d\n", total_iterations_left);
              if(current_iteration == total_iterations_left-1)
              {
                break;
              }
              else if(current_iteration == total_iterations_left)
              {   
                sprintf(packet_id,"%d",current_packet_id);
                send_buffer[packet_size] = packet_id[0];
              }
            }
          }
          bzero(send_buffer, SEND_PACKET); 
          total_iterations_left--;
        }
        printf("[SERVER] Transfer complete \n");
        fclose(fp);
      }
    } 
  }
  close(sock);
}