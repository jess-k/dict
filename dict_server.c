#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

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

sqlite3 *db;
sqlite3 *words_db;
char *err=NULL;
char *err_words=NULL;
int rg_flag=0;
int lg_flag=0;
int confd;
//char usr_buf[64];
//char passwd_buf[64];
char history_buf[1024];

int word_flag=0;
//char word_buf[128];
char desc_buf[512];

int callback_rg(void* notuse,int argc,char**val,char**colname)
{
	rg_flag=1;
	return 0;
}
int callback_lg(void* notuse,int argc,char**val,char**colname)
{
	lg_flag=1;
	return 0;
}
int callback_qu(void* notuse,int argc,char**val,char**colname)
{
	strcpy(history_buf,val[2]);
	return 0;
}


int callback_words(void* notuse,int argc,char**val,char**colname)
{
	word_flag=1;
//	bzero(word_buf,sizeof(word_buf));
	bzero(desc_buf,sizeof(desc_buf));

//	strcpy(word_buf,val[0]);
	strcpy(desc_buf,val[1]);
	return 0;
}


int do_register(int confd,msg_t* msg,sqlite3* db);
int do_login(int confd,msg_t* msg,sqlite3* db);
int do_history(int confd,msg_t* msg,sqlite3* db);
int do_query(int confd,msg_t* msg,sqlite3* db,sqlite3* words_db);

int main(int argc, const char *argv[])
{
	int sockfd;
	int re_bind,re_recv,re_listen;
	int ret;
	int optval=1;

	struct sockaddr_in seraddr;
	struct sockaddr_in srcaddr;
	int len=sizeof(srcaddr);
	msg_t *msg=(msg_t*)malloc(sizeof(msg_t));

	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(50000);
	seraddr.sin_addr.s_addr=inet_addr("0.0.0.0");

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("fail to socket");
		return -1;
	}

	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));

	re_bind=bind(sockfd,(struct sockaddr*)&seraddr,sizeof(seraddr));
	if(re_bind<0)
	{
		perror("fail to bind");
		return -1;
	}

	re_listen=listen(sockfd,5);
	if(re_listen<0)
	{
		perror("fail to listen");
		return -1;
	}

	ret=sqlite3_open("./user.db",&db);
	if(ret)
	{
		fprintf(stderr,"can't open database user.db");
		sqlite3_close(db);
		return -1;
	}
	ret=sqlite3_open("./words.db",&words_db);
	if(ret)
	{
		fprintf(stderr,"can't open database words_db");
		sqlite3_close(words_db);
		return -1;
	}

	char *s="create table user (name char(64),password char(64),history char(1024))";
	ret=sqlite3_exec(db,s,NULL,NULL,&err);
	if(ret)
	{
		fprintf(stderr,"%s\n",err);
		sqlite3_close(db);
		return -1;
	}
	signal(SIGCHLD,SIG_IGN);
	while(1)
	{
		pid_t pid;

		confd=accept(sockfd,(struct sockaddr*)&srcaddr,&len);
		if(confd<0)
		{
			perror("fail to accept");
			return -1;
		}
		pid=fork();
		if(pid<0)
		{
			perror("fail to fork");
			return -1;
		}
		if(pid==0)
		{ 
			close(sockfd);
			while(1)
			{
				bzero(msg,sizeof(msg_t));
				re_recv=recv(confd,msg,sizeof(msg_t),0);
				if(re_recv<0)
				{
					perror("fail to recv");
					exit(EXIT_FAILURE);
				}
				if(re_recv==0)
				{
					close(confd);
					exit(EXIT_SUCCESS);
				}
				switch(msg->type)
				{
				case LOGIN:
					do_login(confd,msg,db);
					printf("insert\n");
					break;
				case REGISTER:
					do_register(confd,msg,db);
					break;
				case HISTORY:
					do_history(confd,msg,db);
					break;
				case QUERY:
					do_query(confd,msg,db,words_db);
					break;
				default:
					break;
				}
			}

		}
		if(pid>0)
		{	
			close(confd);
		}
	
	}
	send(confd,msg->data,sizeof(msg->data),0);
	return 0;
}


int do_register(int sockfd,msg_t* msg,sqlite3* db)
{
	rg_flag=0;
	char str[64];
	int re_sqlite;

	bzero(str,sizeof(str));
	sprintf(str,"select * from user where name=='%s'",msg->name);
	re_sqlite=sqlite3_exec(db,str,callback_rg,NULL,&err);
	if(re_sqlite)
	{
		printf("error2:%s\n",err);
		sqlite3_close(db);
		return -1;
	}

	if(rg_flag==1)
	{
		bzero(msg,sizeof(msg_t));
		msg->type=EXIST;
		send(confd,msg,sizeof(msg_t),0);	
	}
	else
	{
		sprintf(str,"insert into user values('%s','%s',NULL)",msg->name,msg->data);
		re_sqlite=sqlite3_exec(db,str,NULL,NULL,&err);
		if(re_sqlite)
		{
			printf("error3:%s\n",err);
			sqlite3_close(db);
			return -1;
		}
		bzero(msg,sizeof(msg_t));
		msg->type=OK;
		send(confd,msg,sizeof(msg_t),0);	
		return 0;
	}
}


int do_login(int sockfd,msg_t* msg,sqlite3* db)
{
	lg_flag=0;
	char str[128];
	int re_sqlite;

	bzero(str,sizeof(str));
	sprintf(str,"select * from user where name=='%s' and password=='%s'",msg->name,msg->data);
	re_sqlite=sqlite3_exec(db,str,callback_lg,NULL,&err);

	if(re_sqlite)
	{
		printf("error4:%s\n",err);
		sqlite3_close(db);
		return -1;
	}

	if(lg_flag==1)
	{
		bzero(msg,sizeof(msg_t));
		msg->type=OK;
		send(confd,msg,sizeof(msg_t),0);	
	}
	else
	{
		bzero(msg,sizeof(msg_t));
		msg->type=ERROR;
		send(confd,msg,sizeof(msg_t),0);	
	}
	return 0;
}
int do_history(int confd,msg_t* msg,sqlite3* db)
{
	char str[1024];
	int re_sqlite;
	sprintf(str,"select * from user where name=='%s'",msg->name);
	re_sqlite=sqlite3_exec(db,str,callback_qu,NULL,&err);
	if(re_sqlite)
	{
		printf("error8:%s\n",err);
		sqlite3_close(db);
		return -1;
	}
	bzero(msg->data,sizeof(msg->data));
	msg->type=OK;
	strcpy(msg->data,history_buf);
	send(confd,msg,sizeof(msg_t),0);	
	return 0;
}

int do_query(int confd,msg_t* msg,sqlite3*db,sqlite3* words_db)
{
	
	word_flag=0;

	char str[1024];
	int ret_sqlite;
	bzero(str,sizeof(str));

	sprintf(str,"select * from words where word=='%s'",msg->name);

	ret_sqlite=sqlite3_exec(words_db,str,callback_words,NULL,&err_words);
	if(ret_sqlite)
	{
		printf("error5:%s\n",err_words);
		sqlite3_close(words_db);
		return -1;
	}

	if(word_flag==1)
	{

		bzero(str,sizeof(str));
		sprintf(str,"select * from user where name=='%s'",msg->name);

		ret_sqlite=sqlite3_exec(db,str,callback_qu,NULL,&err);
		if(ret_sqlite)
		{
			printf("error6:%s\n",err);
			sqlite3_close(db);
			return -1;
		}

		bzero(str,sizeof(str));
		strcat(history_buf,"\n");
		strcat(history_buf,msg->name);
		strcat(history_buf,"\n");
		strcat(history_buf,desc_buf);
		sprintf(str,"update user set history=='%s' where name=='%s'",history_buf,msg->name);

		ret_sqlite=sqlite3_exec(db,str,callback_qu,NULL,&err);
		if(ret_sqlite)
		{
			printf("error7:%s\n",err);
			sqlite3_close(db);
			return -1;
		}

		bzero(msg,sizeof(msg_t));
		msg->type=OK;
	//	strcpy(msg->name,word_buf);
		strcpy(msg->data,desc_buf);

		send(confd,msg,sizeof(msg_t),0);	
		return 0;
	}
	else
	{
		bzero(msg,sizeof(msg_t));
		msg->type=ERROR;
		send(confd,msg,sizeof(msg_t),0);
		return -1;
	}
}

