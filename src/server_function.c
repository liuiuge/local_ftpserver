#include "ftpsvr.h"

int server_cd(char *tgtpath){
		int i,cnt = 0;
		char p1[128];
		bzero(p1,128);
		for(i=0;i<strlen(tgtpath);i++){
				if(tgtpath[i] == '.' && tgtpath[i-1] == '.'){
						cnt++;
						if(cnt > 1){
								return -1;
						}
				}
		}
		if(!strcmp("..",tgtpath)){
				if(!strcmp("/home/lagrange/local_ftp/usr_lag1",getcwd(NULL,0))){
						return -1;
				}
		}
		p1[0] = 46;
		p1[1] = 47;
		strcat(p1,tgtpath);
		int ret=chdir(p1);
		return ret;	
}

int server_ls(int new_fd,char *path){
		DIR *dir1;
		dir1 = opendir(path);
		struct dirent *pd;
		train t;
		char mode = '0',buf1[32],time1[32],time3[32];
		while((pd = readdir(dir1))!= NULL){
				memset(buf1,0,32);
				memset(time1,0,32);
				memset(time3,0,32);
				memset(&t,0,sizeof(t));
				struct stat dbuf;
				bzero(&dbuf,sizeof(dbuf));
				stat(pd->d_name,&dbuf);
				if(pd->d_type == 4){
						mode = 'd';
				}else{
						mode = '-';
				}
				strcpy(time1,ctime(&dbuf.st_mtime));
				strncpy(time3,time1,strlen(time1)-5);
				sprintf(buf1,"%c%s%16ld",mode," ",dbuf.st_size);
				sprintf(t.buf,"%s%s%s%s%s",buf1," ",time3," ",pd->d_name);
				t.len = strlen(t.buf);
				send_n(new_fd,(char *)&t,4+t.len);
				//printf("%s\n",t.buf);
		}
		int flag= 0;
		send_n(new_fd,(char*)&flag,4);
		return 0;
}
int server_remove(char *name){
		DIR *dir1;
		char newname[32];
		int ret = 0;
		dir1 = opendir(".");
		struct dirent *pd;
		while((pd = readdir(dir1))!= NULL){
				struct stat dbuf;
				bzero(&dbuf,sizeof(dbuf));
				stat(pd->d_name,&dbuf);
				if(pd->d_type != 4 && !strcmp(pd->d_name,name)){
						sprintf(newname,".%s",name);
						ret = rename(name,newname);
						return ret;
				}
		}
		//int ret = unlink(name);
		return -1;
}
int server_get(int new_fd,char *name){
		int fd = open(name,O_RDONLY);
		if(-1 == fd){
				perror("open");
				return -1;
		}
		train t;
		memset(&t,0,sizeof(t));
		t.len = strlen(name);
		strcpy(t.buf,name);
		send(new_fd,&t,4+t.len,0);
		while(memset(&t,0,sizeof(t)), (t.len = read(fd, t.buf,sizeof(t.buf)))>0){
				if(-1 == send_n(new_fd, (char*)&t,4+t.len)){
						close(new_fd);
						return -1;
				}
		}
		int flag=0;
		send_n(new_fd,(char*)&flag,sizeof(int));
		return 0;
}
int server_put(int new_fd){
		char buf[1000];
		int len = 0;
		memset(buf,0,1000);
		recv(new_fd, &len,sizeof(int ),0);
		recv(new_fd,buf,len,0);
		int fd = open(buf,O_CREAT|O_RDWR,0777);
		if(-1 == fd){
				return -1;
		}
		while(1){
				recv_n(new_fd,(char *)&len,sizeof(int));
				if(len>0){
						memset(buf,0,1000);
						recv_n(new_fd,buf,len);
						write(fd,buf,len);
				}else{
						break;
				}
		}
		return 0;
}
