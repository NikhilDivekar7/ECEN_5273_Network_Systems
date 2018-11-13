#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>

#define RECEIVE_PACKET 512
#define SEND_PACKET 512
#define TIME_INTERVAL 3

int main (int argc, char * argv[])
{

  int nbytes;
  int sent_bytes;
  int received_bytes;
  int sock;
  int total_iterations_left = 0;
  int addr_length = sizeof(struct sockaddr);
  int packet_size = 0;
  int fsize;
  int packet_modulus_value = 0;
  int returned_val = 0;
  int count, j, i = 0;
  int current_packet_num = 0;
  int actual_packet_num = 1;
  int nack_counter = 0;
  int allow_next_packet = 0;
  int expected_packet_num = 0;
  int current_packet = 1;
  int valid_file = 1;

  char buffer[100];
  char buffer_recv[RECEIVE_PACKET];
  char buffer_send[SEND_PACKET];
  char file_name[100];
  char packet_id[10];
  char retval[10];
  char command[20]; 
  char split_command[3][3];
  char returned_iter[10];
  
  struct sockaddr_in remote;
  struct sockaddr_in from_addr;
  
  FILE *fp;
  int File_size;
  char File_name[100];
  char file_suffix[100];

  printf("[CLIENT] Getting things started \n");

  if (argc < 3)
  {
    printf("[CLIENT] You are missing out on port number or I/P address \n");
    exit(1);
  }

  bzero(&remote,sizeof(remote));
  remote.sin_family = AF_INET;
  remote.sin_port = htons(atoi(argv[2]));
  remote.sin_addr.s_addr = inet_addr(argv[1]);

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    printf("[CLIENT] Socket creation failed \n");
  }
  else
  {
    printf("[CLIENT] Soxket creation successful \n");
  }

  struct timeval to = {TIME_INTERVAL,0};
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, (char *)&to, sizeof(to));
  
  while(1)
  {
    current_packet = 1;
    expected_packet_num = 0;
    allow_next_packet = 0;
    packet_modulus_value = 0;

    printf("[CLIENT] Please select the appropriate command from below: \n");
    printf("1. get <file_name> \n");
    printf("2. put <file_name> \n");
    printf("3. delete <file_name> \n");
    printf("4. ls \n");
    printf("5. exit \n");

    bzero(command,sizeof(command));
    gets(command);
    sent_bytes = sendto(sock, command, strlen(command), 0, (const struct sockaddr *)&remote, sizeof(remote));
    command[sent_bytes] = '\0';

    printf("[CLIENT] Command is %s \n", command);
    
    if(strncmp(command, "get",3)==0)
    {
      bzero(File_name, 100);
      j = 0; 
      count = 0;
      for(i = 0; i < strlen(command); i++)
      {
        if(command[i] == ' ' || command[i] =='\0')
        {
          split_command[count][j] ='\0';
          count ++;
          j = 0;
        }
        else
        {
          split_command[count][j] = command[i];
          j++;
        }
      }
      strcpy(File_name, split_command[1]);

      printf("[CLIENT] File name received is : %s\n", File_name);

      bzero(buffer_send, SEND_PACKET);
      bzero(buffer_recv, RECEIVE_PACKET);

      allow_next_packet = 0;
      while(allow_next_packet == 0)
      {
        bzero(buffer_recv, RECEIVE_PACKET);
        received_bytes = recvfrom(sock,(char *)buffer_recv, RECEIVE_PACKET, 0, (struct sockaddr *)&from_addr, &addr_length);
        if(received_bytes < 0)
        {
          printf("[CLIENT] Error recieving data\n");
        }
        else if(strncmp(buffer_recv,"File not found",14)==0)
        {
          printf("[CLIENT] File not found \n");
          allow_next_packet = 1;
          valid_file = 0;
        }
        else if(strncmp(buffer_recv,"File not found",14)!=0)
        {
          valid_file = 1;
          strcpy(buffer_send,"ack");
          printf("[CLIENT] Acknowledgement sent for size \n");
          sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
          break;
        }
        buffer_recv[received_bytes] = '\0';
      }
      if(valid_file == 1)
      {
        printf("[CLIENT] Total File size = %s\n", buffer_recv);
        File_size = atoi(buffer_recv);
        total_iterations_left = 1 + File_size/(RECEIVE_PACKET-1);
        bzero(buffer_recv, RECEIVE_PACKET);

        printf("[CLIENT] Number of iterations to send entire data = %d\n", total_iterations_left);
        strcpy(file_suffix,"get_");
        strcat(file_suffix,File_name);
        
        fp = fopen(file_suffix, "w+");
        if(fp == NULL)
        {
          printf("[CLIENT] File not created \n");
        }

        while(total_iterations_left > 0)
        {
          if(total_iterations_left == 1)
          {
            packet_size = File_size % (RECEIVE_PACKET-1);
          }
          else
          {
            packet_size = (RECEIVE_PACKET-1);
          }
          
          if(expected_packet_num != 9)
            expected_packet_num++;
          else
            expected_packet_num = 1;

          printf("[CLIENT] Expected packet ID : %d \n", expected_packet_num);

          allow_next_packet = 0;
          while(allow_next_packet == 0)
          {
            bzero(buffer_send, SEND_PACKET);
            bzero(buffer_recv, RECEIVE_PACKET);

            received_bytes = recvfrom(sock,(char *)buffer_recv, RECEIVE_PACKET, 0, (struct sockaddr *)&from_addr, &addr_length);
            packet_modulus_value = atoi(&buffer_recv[received_bytes-1]);
            if(received_bytes < 0)
            {
              printf("[CLIENT] Error while receiving data \n");
              strcpy(buffer_send,"nack");
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
            }
            else if(expected_packet_num == packet_modulus_value)
            {
              printf("[CLIENT] Sending Acknowledgement for packet %d \n", current_packet);
              strcpy(buffer_send,"ack");
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
              current_packet++;
              break;
            }
            else if(expected_packet_num != packet_modulus_value)
            {
              sprintf(returned_iter,"%d",total_iterations_left);
              strcpy(buffer_send,"nack");
              strcat(buffer_send,returned_iter);
              printf("[CLIENT] Sending nack for iteration %d \n", total_iterations_left);
              printf("Expected packet ID: %d Current packet ID : %d \n", expected_packet_num, packet_modulus_value);
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
            }
          }
          fwrite(buffer_recv,1,packet_size, fp);
          total_iterations_left--;
        }
        fclose(fp);
      }
    }

    else if(strncmp(command, "put",3)==0)
    {
      bzero(file_name, 100);
      j = 0; 
      count = 0;
      for(i = 0; i < strlen(command); i++)
      {
        if(command[i] == ' ')
        {
          split_command[count][j] ='\0';
          count ++;
          j = 0;
        }
        else
        {
          split_command[count][j] = command[i];
          j++;
        }
      }
      strcpy(file_name, split_command[1]);
  
      bzero(buffer_send, SEND_PACKET); 
      fp = fopen(file_name, "r");
      printf("[CLIENT] File name is: %s\n",file_name);
      if(fp == NULL)
      {
        printf("[CLIENT] File not found\n");
        bzero(buffer_send,SEND_PACKET);
        strcpy(buffer_send,"File not found");
        sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (const struct sockaddr *)&remote,sizeof(remote));
      }
      else
      {
        fseek(fp,0,SEEK_END);
        fsize = ftell(fp);
        fseek(fp,0,SEEK_SET);
        printf("[CLIENT] File size: %d\n", fsize);
        sprintf(buffer_send,"%d",fsize);
        sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (const struct sockaddr *)&remote,sizeof(remote));
        bzero(buffer_recv,sizeof(buffer));

        if(fsize < SEND_PACKET-1)
        {
          total_iterations_left = 1;
        }
        else
        {
          total_iterations_left = 1 + fsize/(SEND_PACKET-1);
        }
        while(total_iterations_left != 0)
        {
          for(int i = 0; i<1000; i++);
          if(total_iterations_left == 1)
          {
            packet_size = fsize%(SEND_PACKET-1);
          }
          else
          {
            packet_size = (SEND_PACKET-1);
          }
          bzero(buffer_send,SEND_PACKET);
          fread(buffer_send, 1, packet_size, fp);

          if(current_packet_num != 9)
            current_packet_num++;
          else
            current_packet_num = 1;

          sprintf(packet_id,"%d", current_packet_num);
          buffer_send[packet_size] = packet_id[0];

          printf("{SERVER] Current packet number : %d \n", actual_packet_num);
          printf("[SERVER] Current packet ID : %d \n", current_packet_num);
          allow_next_packet = 0;
          while(allow_next_packet != 1)
          {
            sent_bytes = sendto(sock, buffer_send, (packet_size+1), 0, (const struct sockaddr *)&remote, sizeof(remote));
            if(sent_bytes < 0)
            {
              printf("[CLIENT] Error recieving data \n");
            }
            bzero(buffer_recv, RECEIVE_PACKET);
            received_bytes = recvfrom(sock,(char *)buffer_recv, RECEIVE_PACKET, 0, (struct sockaddr *)&from_addr, &addr_length);
            buffer_recv[received_bytes] = '\0';
            printf("[SERVER] Nack counter : %d \n", nack_counter);
            if(received_bytes <= 0)
            {
              printf("[CLIENT] Error recieving data \n");
            }
            else if(strncmp(buffer_recv,"ack",3)==0)
            {
              printf("[CLIENT] Iteration count: %d\n",total_iterations_left);
              actual_packet_num++;
              break;
            }
            else if(strncmp(buffer_recv,"nack",4)==0)
            {
              nack_counter++;
              strcpy(retval,buffer_recv+4);
              returned_val = atoi(retval);
              printf("[CLIENT] Nack received for iteration number: %d",returned_val);
              if(returned_val == total_iterations_left-1)
              {
                break;
              }
          } 
        }
        bzero(buffer_send, SEND_PACKET); 
        total_iterations_left--;
      }
      printf("[CLIENT] Transfer complete \n");
      fclose(fp); 
      }
    }

    else if(strncmp(command, "ls",2)==0)
    {
      bzero(buffer_send, SEND_PACKET);
      sent_bytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&remote, sizeof(remote));
      expected_packet_num = 0;

      strcpy(File_name, "Files_on_server_side.txt");

      printf("[CLIENT] File name received is : %s\n", File_name);

      bzero(buffer_send, SEND_PACKET);
      bzero(buffer_recv, RECEIVE_PACKET);

      allow_next_packet = 0;
      while(allow_next_packet == 0)
      {
        bzero(buffer_recv, RECEIVE_PACKET);
        received_bytes = recvfrom(sock,(char *)buffer_recv, RECEIVE_PACKET, 0, (struct sockaddr *)&from_addr, &addr_length);
        if(received_bytes < 0)
        {
          printf("[CLIENT] Error recieving file size\n");
        }
        else if(strncmp(buffer_recv,"File not found",14)==0)
        {
          printf("[CLIENT] File not found \n");
          allow_next_packet = 1;
          valid_file = 0;
        }
        else
        {
          allow_next_packet = 1;
          valid_file = 1;
          strcpy(buffer_send,"ack");
          printf("[CLIENT] Acknowledgement sent for file size \n");
          sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
        }
        buffer_recv[received_bytes] = '\0';
      }
      if(valid_file == 1)
      {
        printf("[CLIENT] Total File size = %s\n", buffer_recv);
        File_size = atoi(buffer_recv);
        total_iterations_left = 1 + File_size/(RECEIVE_PACKET-1);
        bzero(buffer_recv, RECEIVE_PACKET);

        printf("[CLIENT] Number of iterations to send entire data = %d\n", total_iterations_left);
        strcpy(File_name,"Files_on_server_side.txt");
        
        fp = fopen(File_name, "w+");
        if(fp == NULL)
        {
          printf("[CLIENT] File not created \n");
        }

        while(total_iterations_left > 0)
        {
          if(total_iterations_left == 1)
          {
            packet_size = File_size % (RECEIVE_PACKET-1);
          }
          else
          {
            packet_size = (RECEIVE_PACKET-1);
          }

          
          if(expected_packet_num != 9)
            expected_packet_num++;
          else
            expected_packet_num = 1;

          printf("[CLIENT] Expected packet ID : %d \n", expected_packet_num);

          allow_next_packet = 0;
          while(allow_next_packet == 0)
          {
            bzero(buffer_send, SEND_PACKET);
            bzero(buffer_recv, RECEIVE_PACKET);

            received_bytes = recvfrom(sock,(char *)buffer_recv, RECEIVE_PACKET, 0, (struct sockaddr *)&from_addr, &addr_length);
            packet_modulus_value = atoi(&buffer_recv[received_bytes-1]);
            if(received_bytes < 0)
            {
              printf("[CLIENT] Error while recieving data \n");
              strcpy(buffer_send,"nack");
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
            }
            else if(expected_packet_num == packet_modulus_value)
            {
              allow_next_packet = 1;
              printf("[CLIENT] Sending Acknowledgement for packet %d \n", current_packet);
              strcpy(buffer_send,"ack");
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
              current_packet++;
            }
            else if(expected_packet_num != packet_modulus_value)
            {
              sprintf(returned_iter,"%d",total_iterations_left);
              strcpy(buffer_send,"nack");
              strcat(buffer_send,returned_iter);
              printf("[CLIENT] Sending nack for iteration %d \n", total_iterations_left);
              printf("Expected packet ID: %d Current packet ID : %d \n", expected_packet_num, packet_modulus_value);
              sent_bytes = sendto(sock, buffer_send, strlen(buffer_send), 0, (struct sockaddr *)&remote, sizeof(remote));
            }
          }
          fwrite(buffer_recv,1,packet_size, fp);
          total_iterations_left--;
        }
        fclose(fp);
      }
    } 

    else if(strncmp(command, "delete",6)==0)
    {
      bzero(buffer_send, SEND_PACKET);
      sent_bytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&remote, sizeof(remote));
      printf("Filename sent Successfully\n");
    }
    else if(strncmp(command, "exit",4)==0)
    {
      printf("Exiting the client \n");
      close(sock);
      exit(0);
    }
  }
  close(sock);
}
