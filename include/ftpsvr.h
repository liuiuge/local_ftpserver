#ifndef __FTPSVR_H__
#define __FTPSVR_H__
#include "head.h"

typedef struct data{
	pid_t pid;
	int sfd;
	short busy_flg;
}Data, *pdata;

typedef struct tdata{
	int len;
	char buf[1000];
}train, *ptrain;

//服务器功�?
int server_cd(char *);
int server_ls(int,char *);
int server_pwd(char *);
int server_remove(char *);
int server_get(int,char *);
int server_put(int);

int makechild(pdata,int);
void send_fd(int, int);
void recv_fd(int, int *);
int send_n(int, char *, int);
void recv_n(int ,char *,int );
int send_file(int);
int send_msg(int, char*);



#endif
