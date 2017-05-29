#include "ftpsvr.h"

void child_work(int sfd){
		signal(SIGPIPE, SIG_IGN);
		int new_fd = -1;
		chdir("/home/lagrange/local_ftp/usr_lag1");
		char bufrecv[32] = {0},bufsend[128] = {0};
		bzero(bufrecv,32);
		bzero(bufsend,128);
		char flag = 'f';
		int i,ret = 0;
		while(1){
				recv_fd(sfd,&new_fd);
				printf("work begin;\n");
				while(1){
						printf(">");
						memset(bufrecv,0,128);
						ret = recv(new_fd,bufrecv,sizeof(bufrecv),0);
						if(ret > 0){
								printf("%s\n",bufrecv);
								if ( !strcmp(bufrecv, "cd" )){
										send(new_fd,"input the path ",15,0);
										bzero(bufrecv,32);
										recv(new_fd,bufrecv,sizeof(bufrecv),0);
										ret = server_cd(bufrecv);
										ret ++;
										if(ret == 1){
												printf("chdir %s\n",bufrecv);
												send(new_fd,(char *)&ret,4,0);
										}else{
												printf("wrong path\n");
												send(new_fd,(char *)&ret,4,0);
										}
								}else if ( !strcmp(bufrecv, "ls" )){
										printf("ls begin\n");
										getcwd(bufsend,128);
										server_ls(new_fd,bufsend);
										printf("ls finish\n");
								}else if(!strcmp(bufrecv,"remove")){
										send(new_fd,"input the name",15,0);
										bzero(bufrecv,32);
										recv(new_fd,bufrecv,sizeof(bufrecv),0);
										ret  = server_remove(bufrecv);
										ret++;
										if(ret == 1){
												printf("remove %s\n",bufrecv);
												send(new_fd,(char *)&ret,4,0);
										}else{
												printf("wrong name\n");
												send(new_fd,(char *)&ret,4,0);
										}
								}else if(!strcmp(bufrecv,"pwd")){
										strcpy(bufsend,getcwd(NULL,0));
										bufsend[strlen(bufsend)] = 47;
										for(i=33;i<sizeof(bufsend);i++){
												bufsend[i-33] = bufsend[i];
										}
										printf("%s\n",bufsend);
										send(new_fd, bufsend,strlen(bufsend),0);
										bzero(bufsend,128);
								}else if(!strcmp(bufrecv,"exit")){
										goto end;
								}else if(!strcmp(bufrecv,"get")){
										send(new_fd,"input the name ",15,0);
										bzero(bufrecv,32);
										recv(new_fd,bufrecv,sizeof(bufrecv),0);
										ret = open(bufrecv,O_RDONLY);
										ret ++;
										if(ret >0){
												send(new_fd,(char *)&ret,4,0);
												close(ret - 1);
												printf("download start...\n");
												server_get(new_fd,bufrecv);
												printf("download finish...\n");
										}else{
												printf("wrong name\n");
												send(new_fd,(char *)&ret,4,0);
										}
								}else if(!strcmp(bufrecv,"put")){
										ret = 1;
										send(new_fd,(char *)&ret,4,0);
										printf("upload start...\n");
										server_put(new_fd);
										printf("upload finish...\n");
								}else{
										send(new_fd,"err input",9,0);
								}
						}else
								break;
				}
end:
				printf("client log out\n");
				write(sfd,&flag,sizeof(flag));			
		}
}

int makechild(pdata p,int cnt_proc){
		int fds[2],i = 0;
		pid_t pid;
		for(;i < cnt_proc; i ++ ){
				socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
				pid = fork();
				if(!pid){
						close(fds[1]);
						child_work(fds[0]);
				}
				close(fds[0]);
				printf("child %d begin!\n",pid);
				p[i].pid = pid;
				p[i].sfd = fds[1];
				p[i].busy_flg = 0;
		}
		return 0;
}
