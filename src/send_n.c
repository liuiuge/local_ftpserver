#include "ftpsvr.h"

int send_n(int sfd,char *buf,int post_len){
	int ret = 0, total_send = 0;
	while(total_send<post_len){
		ret = send(sfd,buf+total_send,post_len - total_send,0);
		total_send += ret;
	}
}
void recv_n(int sfd,char *buf,int post_len){
	int ret = 0, total_recv = 0;
	while(total_recv < post_len){
		ret = recv(sfd,buf+total_recv,post_len - total_recv,0);
		total_recv += ret;
	}
}
