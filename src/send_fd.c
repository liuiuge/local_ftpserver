#include "ftpsvr.h"

void send_fd(int sfd,int fd){
		struct msghdr msg;
		memset(&msg,0,sizeof(msg));
		struct iovec iov[2];
		char buf1[3] = "lo";
		char buf2[3] = "ve";
		iov[0].iov_base = buf1;
		iov[0].iov_len = 2;
		iov[1].iov_base = buf2;
		iov[1].iov_len = 2;
		int len = (int)CMSG_LEN(fd);
		struct cmsghdr *cmsg =(struct cmsghdr *) calloc(1,len);
		cmsg->cmsg_len = len;
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		*(int *)CMSG_DATA(cmsg) = fd;
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;
		msg.msg_control = cmsg;
		msg.msg_controllen = len;
		int ret = sendmsg(sfd,&msg,0);
		if(ret == -1){
				perror("sendmsg");
				return ;
		}
		return ;
}
void recv_fd(int sfd,int *fd){
		struct msghdr msg;
		memset(&msg,0,sizeof(msg));
		struct iovec iov[2];
		char buf1[3] = "lo";
		char buf2[3] = "ve";
		iov[0].iov_base = buf1;
		iov[0].iov_len = 2;
		iov[1].iov_base = buf2;
		iov[1].iov_len = 2;
		int len = CMSG_LEN(sizeof(int));
		struct cmsghdr *cmsg = (struct cmsghdr *)calloc(1,len);
		cmsg->cmsg_len = len;
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;
		msg.msg_control = cmsg;
		msg.msg_controllen = len;
		int ret = recvmsg(sfd,&msg,0);
		if(ret == -1){
				perror("resvmsg");
				return ;
		}
		*fd = *(int *)CMSG_DATA(cmsg);
		return ;
}
