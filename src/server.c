#include "ftpsvr.h"

int get_args(char *argv,char *args1,char *args2,char *args3){
		FILE *fd_conf = fopen(argv,"r");
		char ser_addr[16],ser_port[5],ser_proc[2];
		bzero(ser_addr,16);
		bzero(ser_port,5);
		bzero(ser_proc,2);
		if(fd_conf == NULL){
				perror("fopen");
				return -1;
		}
		fscanf(fd_conf,"%s %s %s",ser_addr,ser_port,ser_proc);
		strcpy(args1, ser_addr);
		strcpy(args2, ser_port);
		strcpy(args3, ser_proc);
		fclose(fd_conf);
		return 0;
}

int main(int argc,char* argv[])
{
		if(argc != 2){
				printf("Err args:%d\n",argc);
				return -1;
		}
		int ret_conf = 0,ret_bind = 0,ret_listen = 0,ret_Epoll = 0;
		char ser_addr[16],ser_port[5],ser_proc[2];
		if((ret_conf = get_args(argv[1],ser_addr,ser_port,ser_proc)) == -1){
				printf("Err conf:%d\n",ret_conf);
				return -1;
		}
		system("clear");
		printf(">server:\n\tip:%s\n\tport:%s\n\tproc:%s\n",ser_addr,ser_port,ser_proc);

		int pro_num=atoi(ser_proc);
		pdata p=(pdata)calloc(pro_num,sizeof(Data));
		makechild(p,pro_num);

		struct sockaddr_in ftp_local;
		ftp_local.sin_family = AF_INET;
		ftp_local.sin_addr.s_addr = inet_addr(ser_addr);
		ftp_local.sin_port = htons(atoi(ser_port));
		int ftp_fd = socket(AF_INET, SOCK_STREAM, 0),cli_fd;
		if((ret_bind = bind(ftp_fd, (struct sockaddr *)&ftp_local,sizeof(ftp_local))) == -1){
				perror("bind");
				return -1;
		}
		printf(">bind success!\n");
		int cnt_proc = atoi(ser_proc);
		if((ret_listen = listen(ftp_fd,cnt_proc)) == -1){
				perror("listen");
				return -1;
		}
		printf(">listen success!\n");

		int new_fd=-1;
		struct epoll_event eve,*evs;
		evs = ( struct epoll_event* )calloc(cnt_proc+1, sizeof(eve));
		memset(&eve,0,sizeof(eve));
		int epfd = epoll_create(1);
		eve.events=EPOLLIN;
		eve.data.fd=ftp_fd ;
		ret_Epoll=epoll_ctl(epfd,EPOLL_CTL_ADD,ftp_fd ,&eve);
		if(-1==ret_Epoll)
		{
				perror("epoll_ctl");
				return -1;
		}
		int i;
		for(i=0;i<pro_num;i++)
		{
				memset(&eve,0,sizeof(eve));
				eve.events=EPOLLIN;
				eve.data.fd=p[i].sfd;
				epoll_ctl(epfd,EPOLL_CTL_ADD,p[i].sfd,&eve);
		}
		int j;
		char flag;
		while(1)
		{
				memset(evs,0,(pro_num+1)*sizeof(eve));
				ret_Epoll=epoll_wait(epfd,evs,pro_num+1,-1);
				for(i=0;i<ret_Epoll;i++)
				{
						if(evs[i].data.fd==ftp_fd )
						{
								new_fd=accept(ftp_fd ,NULL,NULL);
								for(j=0;j<pro_num;j++)
								{
										if(0==p[j].busy_flg)			
										{
												send_fd(p[j].sfd,new_fd);
												p[j].busy_flg = 1;
												printf("child %d is busy\n",p[j].pid);
												break;
										}
								}
								close(new_fd);	
						}
						for(j=0;j<pro_num;j++)
						{
								if(evs[i].data.fd==p[j].sfd)
								{
										read(p[j].sfd,&flag,sizeof(flag));
										p[j].busy_flg = 0;
										printf("child %d is not busy\n",p[j].pid);
										break;				
								}
						}
				}	
		}	
		wait(NULL);
		return 0;
}
