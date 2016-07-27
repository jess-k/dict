#include <stdio.h>
#include <sqlite3.h>
#include <strings.h>
#include <string.h>

int callback(void*notuse,int argc,char**val,char**colname)
{
	int i;
	for(i=0;i<argc;i++)
	{
		printf("%s\n",val[i]);
	}
	return 0;
}
int main(int argc, const char *argv[])
{
	sqlite3 *db;
	int rc;
	FILE* fp;
	char buf[1024];
	char w_buf[1024];
	char d_buf[1024];
	char insert_buf[1024];
	int i=0;
	rc=sqlite3_open("./words.db",&db);
	if(rc)
	{
		fprintf(stderr,"can't open database:%s",sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	
	}
	char *error;
	char *s="create table words (word char(128),description char(512))";
	rc=sqlite3_exec(db,s,NULL,NULL,&error);
	if(rc)
	{
		printf("error1:%s\n",error);
		sqlite3_close(db);
		return -1;
	}
	fp=fopen("./dict.txt","r");
	if(fp==NULL)
	{
		perror("fail to open file\n");
		return -1;
	}

	while(1)
	{
		i=0;
		bzero(buf,sizeof(buf));
		if(NULL==fgets(buf,sizeof(buf),fp))
		{
			break;
		}

	//	printf("%s",buf);
		while(buf[i]!=' ')
		{
			i++;
		}

		bzero(w_buf,sizeof(w_buf));
		strncpy(w_buf,buf,i);
		printf("word:%s\n",w_buf);

		while(buf[i]==' ')
		{
			i++;
		}

		bzero(d_buf,sizeof(d_buf));
		strncpy(d_buf,buf+i,strlen(buf+i)-2);
		printf("description:%s\n",d_buf);

		bzero(insert_buf,sizeof(insert_buf));
		sprintf(insert_buf,"insert into words values('%s','%s')",w_buf,d_buf);
		rc=sqlite3_exec(db,insert_buf,NULL,NULL,&error);

		if(rc)
		{
			printf("error2:%s\n",error);
			sqlite3_close(db);
			return -1;
		}


	}
	char*ss="select * from words";
	rc=sqlite3_exec(db,ss,callback,NULL,&error);
	if(rc)
	{
		printf("error:%s\n",error);
		sqlite3_close(db);
		return -1;
	}
	sqlite3_close(db);
	return 0;
}
