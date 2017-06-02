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
				if(pd->d_name[0] == '.') continue;
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
		char sha1[41] ,err[256],r_sha1[41];
		int flag_check = 0, file_size = 0, mmap_flag = 0;
		train t;
		struct stat dbuf;
		memset(&dbuf,0,sizeof(dbuf));
		memset(&t,0,sizeof(t));
		memset(r_sha1,0,41);
		memset(sha1,0,41);
		memset(err,0,256);
		int fd = open(name,O_RDONLY);
		if(-1 == fd){
				perror("open");
				return -1;
		}
		stat(name,&dbuf);
		int ret = recv(new_fd, &flag_check, 4, 0);
		GetFileSHA1b(name,sha1,err);
		if(flag_check == 1){
				printf("find same files\n");
				printf("sha1:%s\n",sha1);
				recv(new_fd,&file_size,8,0);
				recv(new_fd,&r_sha1,41,0);
				printf("sha1:%s\n",sha1);
				if(!strcmp(sha1,r_sha1)){
					printf("download from last time\n");		
				}
		}else{
			send(new_fd,&sha1,41,0);
		}
		lseek(fd,file_size,SEEK_SET);
		lseek(fd,0,SEEK_CUR);
		memset(&t,0,sizeof(t));
		t.len = strlen(name);
		strcpy(t.buf,name);
		if(-1 == send(new_fd,&t,4+t.len,0)) return -1;
		recv(new_fd, &t.len, 4, 0);
		if(dbuf.st_size - file_size < (1<<18)){
			send(new_fd, &mmap_flag, 4, 0);
			while(memset(&t,0,sizeof(t)), (t.len = read(fd, t.buf,sizeof(t.buf)))>0){
				if(-1 == send_n(new_fd, (char*)&t,4+t.len)){
						close(new_fd);
						return -1;
				}
			}
			int flag = 0;
			send_n(new_fd,(char*)&flag,sizeof(int));
			return 0;
		}else{
			char *mmap_s = (char *)mmap(NULL,dbuf.st_size - file_size,PROT_READ,MAP_SHARED,fd,0);
			if(mmap_s == (char *)-1){
				mmap_flag = -1;
				perror("mmap");
				send(new_fd,&mmap_flag, 4, 0);
				return -1;
			}
			mmap_flag = 1;
			send(new_fd, &mmap_flag, 4, 0);
			send(new_fd, &dbuf.st_size, 8, 0);
			char *mmap_e = mmap_s + dbuf.st_size - file_size;
			//mmap读取
			char *pcur= mmap_s;
			while((mmap_e>pcur) && (mmap_e - pcur )>512){
				memset(&t,0,sizeof(t));
				t.len = 512;
				memcpy(t.buf,pcur,512);
				if(-1 == send_n(new_fd, (char*)&t,4+t.len)){
						close(new_fd);
						return -1;
				}
				pcur += 512;
			}
			if(mmap_e> pcur){
				memset(&t,0,sizeof(t));
				t.len = mmap_e-pcur;
				memcpy(t.buf,pcur,1000);
				if(-1 == send_n(new_fd, (char*)&t,4+t.len)){
						close(new_fd);
						return -1;
				}
			}
			int flag = 0;
			send_n(new_fd,(char*)&flag,sizeof(int));
			recv(new_fd,&flag_check,4,0);
			munmap(mmap_s,dbuf.st_size - file_size);
			return flag_check;
		}
}
int server_put(int new_fd){
		char buf[1000];
		char sha1[41] = "sha1",str[7] ={0};
		int len = 0,flag_check = 0,fd,mmap_flag = 0;
		long file_size = 0;
		memset(buf,0,1000);
		recv(new_fd, &len,sizeof(int ),0);
		recv(new_fd,buf,len,0);
		struct stat dbuf;
		memset(&dbuf,0,sizeof(dbuf));
		stat(buf,&dbuf);
		fd = open(buf,O_RDWR);
		if(fd > 0){
				printf("file exist\n");
				flag_check = 1;
				send(new_fd, &flag_check, 4, 0);
				send(new_fd,&dbuf.st_size, 8, 0);
				send(new_fd,sha1, 41, 0);
				close(fd);
		}else{
				send(new_fd,&flag_check,4,0);
		}
		fd = open(buf,O_CREAT|O_RDWR,0777);
		if(-1 == fd){
				return -1;
		}
		lseek(fd,dbuf.st_size/2,SEEK_SET);
		send(new_fd,&len,4,0);
		recv(new_fd,&mmap_flag,4,0);
		if(mmap_flag == 0){
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
		}else if(mmap_flag == 1){
			recv(new_fd,&file_size, 8, 0);
			ftruncate(fd,file_size);
			lseek(fd,0,SEEK_CUR);
			printf("%ld\n",file_size);
			char *mmap_s = (char *)mmap(NULL, file_size - dbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd, 0);
			char *pcur = mmap_s, *mmap_e = mmap_s +file_size - dbuf.st_size;
			while(1){
					recv_n(new_fd, (char *)&len,sizeof(int));
					if(len>0){
						memset(buf,0,1000);
						recv_n(new_fd,buf,len);
						memcpy(pcur, buf, len);
						pcur += len;
					}else{
						break;
					}
			}
			munmap(mmap_s,file_size - dbuf.st_size);
			send(new_fd,&mmap_flag, 4, 0);
		}else{
			printf("mmap fail\n");
			return -1;
		}
		printf("upload finish\n");
		return 0;
}
