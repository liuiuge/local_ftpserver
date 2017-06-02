#include "ftpsvr.h"

void write_time(int fd){
	time_t t1;
	time(&t1);
	char log_time[32];
	bzero(log_time,32);
	strcpy(log_time,asctime(gmtime(&t1)));
	write(fd,log_time,strlen(log_time));
}

int server_reg(int new_fd){
	chdir("/home/lagrange/local_ftp1/usrinfo");
	char name[16];
	char miwen[16];
	DIR *dir;
	int fd;
	int ret = 0;
	int flag = 0;
	send(new_fd,"input the name ",15,0);
regstt:	
	memset(name,0,16);
	memset(miwen,0,16);
	ret = recv(new_fd,name,16,0);
	if(ret > 0){
		dir = opendir(".");
		struct dirent *p;
		while((p = readdir(dir))!= NULL){
			if(!strcmp(p->d_name,name)){
				send(new_fd,(char *)&flag,4,0);
				goto regstt;
			}
		}
		flag = 1;
		send(new_fd,(char *)&flag,4,0);
		fd = open(name,O_CREAT|O_RDWR,0666);
		send(new_fd,"input the pswd ",15,0);
		ret = recv(new_fd,miwen,16,0);
		write(fd,miwen,strlen(miwen));
		close(fd);
		char path[64];
		memset(path,0,64);
		strcpy(path,"/home/lagrange/local_ftp1/usr_");
		strncat(path,name,3);
		mkdir(path,0777);
		chdir(path);
		mkdir("Music",0777);
		mkdir("Video",0777);
		mkdir("Photo",0777);
		mkdir("Docmt",0777);
		return 1;
	}else{
		return -1;
	}
}

int server_log(int new_fd,char *name){
	char buf[16];
	char miwen[2][16];
	char path[128] = "/home/lagrange/local_ftp1/usrinfo/";
	send(new_fd,"input the name ",15,0);
	memset(buf,0,16);
	memset(miwen,0,32);
	int ret = recv(new_fd,buf,16,0);
	if(ret > 0){
		strcat(path,buf);
		send(new_fd,"input the pswd ",15,0);
		ret = recv(new_fd,miwen[0],16,0);
		printf("%s\n",miwen[0]);
		if(ret <=0 ) return -1;
		int fd = open(path,O_RDONLY);
		int flag = 0;
		if(-1 == fd ){
			printf("err openfile");
			return -1;
		}
		read(fd,miwen[1],16);
		printf("%s\n",miwen[1]);
		if(!strcmp(miwen[0],miwen[1])){
			strncpy(name,buf,4);
			name[3] = '\0';
			return 1;
		}else{
			return -1;
		}	
	}else{
		return -1;
	}
}

void child_work(int sfd){
	signal(SIGPIPE, SIG_IGN);
	int new_fd = -1, log_fd = -1, act_fd= -1;
	char bufrecv[32] = {0},bufsend[128] = {0};
	bzero(bufrecv,32);
	bzero(bufsend,128);
	char flag = 'f';
	int i,ret_lch = 0,ret_cmd = 0,ret;
	char path[64];
	memset(path,0,64);
	while(1){
		recv_fd(sfd,&new_fd);
		printf("work begin;\n");
		while(1){
launch_page:
			memset(bufrecv,0,128);
			ret_lch = recv(new_fd,bufrecv,sizeof(bufrecv),0);
			if(ret_lch > 0){
				printf(">");
				printf("%s\n",bufrecv);
				if( !strcmp(bufrecv,"exit") ){
					goto end;
				}
				else if(!strcmp(bufrecv,"log")){
					char name[4] = {0};
					bzero(name,4);
					ret = server_log(new_fd,name);
					log_fd = open(".logging.dat",O_CREAT|O_RDWR,0666);
					ret ++;
					if(ret == 0){
						send(new_fd,(char *)&ret,4,0);
						lseek(log_fd,0,SEEK_END);
						int retw = write(log_fd,"\nlog failed ",12);
						printf("%d\n",retw);
						write_time(log_fd);
						close(log_fd);
						goto launch_page;
					}else{
						send(new_fd,(char *)&ret,4,0);
						lseek(log_fd,0,SEEK_END);
						write(log_fd,"\nlog success ",13);
						write_time(log_fd);
						close(log_fd);
						strcpy(path,"/home/lagrange/local_ftp1/usr_");
						strcat(path,name);
						chdir(path);			
					}
					act_fd = open(".logging.dat",O_CREAT|O_RDWR,0666);
					while(1){				
						lseek(act_fd,0,SEEK_END);
						memset(bufrecv,0,32);
						//printf("path:%s\n",path);
						write(act_fd,"\n>",2);
						ret_cmd = recv(new_fd,bufrecv,sizeof(bufrecv),0);
						if(ret_cmd > 0){
							write(act_fd,bufrecv,strlen(bufrecv));
							write_time(act_fd);
							if ( !strcmp(bufrecv, "cd" )){
								send(new_fd,"input the path ",15,0);
								bzero(bufrecv,32);
								recv(new_fd,bufrecv,sizeof(bufrecv),0);
								ret = server_cd(bufrecv);
								ret ++;
								if(ret == 1){
									write(act_fd," chdir ",7);
									write(act_fd,bufrecv,strlen(bufrecv));
									send(new_fd,(char *)&ret,4,0);
								}else{
									write(act_fd," wrong path ",12);
									send(new_fd,(char *)&ret,4,0);
								}
							}else if ( !strcmp(bufrecv, "ls" )){
								//printf("ls begin\n");
								getcwd(bufsend,128);
								server_ls(new_fd,bufsend);
								//printf("ls finish\n");
							}else if(!strcmp(bufrecv,"remove")){
								send(new_fd,"input the name",15,0);
								bzero(bufrecv,32);
								recv(new_fd,bufrecv,sizeof(bufrecv),0);
								ret  = server_remove(bufrecv);
								ret++;
								if(ret == 1){
									write(act_fd, " remove ",8);
									write(act_fd,bufrecv,strlen(bufrecv));
									send(new_fd,(char *)&ret,4,0);
								}else{
									write(act_fd," wrong name ",12);
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
								close(act_fd);
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
									write(act_fd," download start ",16);
									//printf("download start...\n");
									server_get(new_fd,bufrecv);
									//printf("download finish...\n");
									write(act_fd," download finish ",17);
								}else{
									write(act_fd," wrong name ",12);
									send(new_fd,(char *)&ret,4,0);

								}
							}else if(!strcmp(bufrecv,"put")){
								ret = 1;
								send(new_fd,(char *)&ret,4,0);
								write(act_fd," upload start ",14);
								server_put(new_fd);
								write(act_fd," upload finish ",15);
							}else{
								send(new_fd,"err input",9,0);
							}//if(strcmp(bufrecv,all above)	
						}//if(ret_cmd>0)
						else{
							goto end;
						}//else(ret_cmd>0)		
					}//while(1)
				}//if(!strcmp(bufrecv,"log"))
				else if(!strcmp(bufrecv,"reg")){
					ret = server_reg(new_fd);
					if(ret < 0){
						send(new_fd, (char *)&ret, 4, 0);
						goto end;
					}//if(ret < 0)
					send(new_fd, (char *)&ret, 4, 0);
					printf("reg success\n");
				}//if(!strcmp(bufrecv,"reg"))
				else{
					send(new_fd,"Err input",9,0);
				}//if(strcmp(bufrecv,all_above))
			}//if(ret_lch > 0)
			else{
				break;
			}//else(ret_lch > 0)
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
