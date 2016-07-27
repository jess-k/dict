#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>

#define LOGIN 1
#define REGISTER 2
#define HISTORY 3
#define QUERY 4
#define EXIST 5
#define OK 6
#define ERROR 7

typedef struct msg_node{
	int type;
	char name[128];
	char data[1024];
}msg_t;

int do_register(int sockfd,msg_t* msg);
int do_login(int sockfd,msg_t* msg);
int do_query(int sockfd,msg_t* msg);
int do_history(int sockfd,msg_t* msg);

int main(int argc, const char *argv[])
{
	char c;
	int sockfd;
	int re_connect;
	struct sockaddr_in seraddr;
	msg_t *msg=(msg_t*)malloc(sizeof(msg_t));

	if(argc!=3)
	{
		fprintf(stderr,"Usage:%s is wrong\n",argv[0]);
		return -1;
	}

	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(atoi(argv[2]));
	seraddr.sin_addr.s_addr=inet_addr(argv[1]);

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("fail to socket");
		return -1;
	}
	re_connect=connect(sockfd,(struct sockaddr*)&seraddr,sizeof(seraddr));
	if(re_connect<0)
	{
		perror("fail to connect the server");
		return -1;
	}
	
	while(1)
	{
		printf("************************************************\n");
		printf("******* 1:register  2:login  3:quit  ***********\n");
		printf("************************************************\n");
//		printf("\n");
		scanf("\n%c",&c);
	//	printf("\n");
		switch(c)
		{
			case '1':
				do_register(sockfd,msg);
				break;
			case '2':
				do_login(sockfd,msg);
				break;

			case '3':
			//	printf("\n");
				close(sockfd);
				exit(0);
			//	break;
			default:
				printf("this command is not exist!\n");
				break;
		}
	
	}
	
	return 0;
}
int do_register(int sockfd,msg_t* msg)
{
	bzero(msg,sizeof(msg_t));

	msg->type=REGISTER;
	printf("input a user name:\n");
	scanf("%s",msg->name);
	printf("set your password:\n");
	scanf("%s",msg->data);

	send(sockfd,msg,sizeof(msg_t),0);
	bzero(msg,sizeof(msg_t));
	recv(sockfd,msg,sizeof(msg_t),0);
	if(msg->type==EXIST)
	{
		printf("this name is be used\n");
		return -1;
	}

	if(msg->type==OK)
	{
		printf("register succeed;login please!\n");
		return 0;
	}
	
}
int do_login(int sockfd,msg_t* msg)
{
	bzero(msg,sizeof(msg_t));

	msg->type=LOGIN;
	printf("input your user name:\n");
	scanf("%s",msg->name);
	printf("input your password:\n");
	scanf("%s",msg->data);

	
	send(sockfd,msg,sizeof(msg_t),0);
	bzero(msg,sizeof(msg_t));
	recv(sockfd,msg,sizeof(msg_t),0);
	if(msg->type==ERROR)
	{
		printf("Account is error,check your account or register for new user.\n");
		return -1;
	}

	if(msg->type==OK)
	{
		char cmd;
		printf("login succeed,querry now:\n");
		printf("************************************************\n");
		while(1)
		{
			printf("**** 1:query_word  2:history_word  3:quit  *****\n");
			printf("************* please choose: *******************\n");
			scanf("\n%c",&cmd);
			switch(cmd)
			{
				case '1':
					printf("\n");
					do_query(sockfd,msg);
					break;
				case '2':
					printf("\n");
					do_history(sockfd,msg);
					break;
				case '3':
					close(sockfd);
					exit(0);
				default :
					printf("wrong selected!\n");
					break;
			}
		}
		return 0;
	}

}

int do_query(int sockfd,msg_t* msg)
{
	bzero(msg,sizeof(msg_t));
	msg->type=QUERY;

	printf("************ input you word:  ******************\n");
//	fgets(msg->name,sizeof(msg->name),stdin);
	scanf("%s",msg->name);
	send(sockfd,msg,sizeof(msg_t),0);
	bzero(msg,sizeof(msg_t));
	recv(sockfd,msg,sizeof(msg_t),0);
	if(msg->type==ERROR)
	{
		printf("this word is not exist in the words_db\n");
		return -1;
	}

	if(msg->type==OK)
	{
		printf("word:%s\n",msg->name);
		printf("description:%s\n",msg->data);
		return 0;
	}
}
int do_history(int sockfd,msg_t* msg)
{
	msg->type=HISTORY;

	send(sockfd,msg,sizeof(msg_t),0);

	bzero(msg,sizeof(msg_t));
	recv(sockfd,msg,sizeof(msg_t),0);

	if(msg->type==ERROR)
	{
		printf("No histories\n");
		return -1;
	}

	if(msg->type==OK)
	{
		printf("%s\n",msg->name);
		printf("%s\n",msg->data);
		return 0;
	}
}


