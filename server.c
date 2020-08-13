#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAXLINE 8192
#define SERV_PORT 8000
#define OPEN_MAX 5000

int main(int argc, char * argv[])
{
	int i, lfd, cfd, sockfd;
	int n;
	ssize_t nready, efd, res;
	char buf[BUFSIZ];

	struct sockaddr_in serv_addr, clit_addr;
	socklen_t clit_addr_len = sizeof(clit_addr);
	memset(&serv_addr, 0, sizeof(serv_addr));
	struct epoll_event tep, ep[OPEN_MAX];
	
	lfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(lfd, 128);

	efd = epoll_create(OPEN_MAX);
	if (efd == -1)
	{
		perror("epoll_create error");
		exit(1);
	}

	tep.events = EPOLLIN;
	tep.data.fd = lfd;

	res = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);
	if(res == -1)
	{
		perror("epoll_ctl error");
		exit(1);
	}

	while(1)
	{
		nready = epoll_wait(efd, ep, OPEN_MAX, -1);
		if (nready == -1)
		{
			perror("epoll_wait error");
			exit(1);
		}
		for(i = 0; i < nready; i++)
		{
			if(ep[i].data.fd == lfd)
			{
				cfd = accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
				tep.events = EPOLLIN;
				tep.data.fd = cfd;
				res = epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tep);
				if(res == -1)
				{
					perror("epoll_ctl error");
					exit(1);
				}
			} 
			else 
			{
				sockfd = ep[i].data.fd;
				n = read(sockfd, buf, sizeof(buf));
				if (n == 0)
				{
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					if (res == -1)
					{
						perror("epll_ctl error");
						exit(1);
					}
					close(sockfd);
					printf("client[%d] closed connection\n", sockfd);
				}
				else if(n > 0)
				{
					for (i = 0; i < n; i++)
					{
						buf[i] = tolower(buf[i]);
					}
					write(sockfd, buf, n);
					write(STDOUT_FILENO, buf, n);
				}
			}
		}
	}
	close(lfd);
	return 0;
}

 
