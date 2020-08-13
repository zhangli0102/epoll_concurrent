#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define SERV_PORT 6666

int main(int argc, char* argv[])
{
	struct sockaddr_in serv_addr, clit_addr;
	socklen_t clit_addr_len = sizeof(clit_addr);

	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(lfd, 128);

	struct epoll_event tmp, ep[1024];
	int efd = epoll_create(1024);
	if (efd == -1)
	{
		perror("epoll_create error");
		exit(1);
	}

	tmp.data.fd = lfd;
	tmp.events = EPOLLIN;	

	int ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tmp);
	if(ret == -1)
	{
		perror("epoll_ctl error");
		exit(1);
	}

	int i, j, cfd, n;
	char buf[8092];

	while(1)
	{
		ret = epoll_wait(efd, ep, 1024, -1);
		if(ret == -1)
		{
			perror("epoll_wait error");
			exit(1);
		}

		for (i = 0; i < ret; i++)
		{
			if(ep[i].data.fd == lfd)
			{
				cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
				tmp.data.fd = cfd;
				tmp.events = EPOLLIN;
				epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &tmp);
				printf("Established a connection with %d\n", cfd);	
			}
			else
			{
				n = read(ep[i].data.fd, buf, sizeof(buf));
				if(n == 0)
				{
					epoll_ctl(efd, EPOLL_CTL_DEL, ep[i].data.fd, NULL);
					close(ep[i].data.fd);
					printf("Connection to %d is closed\n", ep[i].data.fd);
				}
				else if (n > 0)
				{
					for (j = 0; j < n; j++)
					{
						buf[j] = toupper(buf[j]);
					}
					write(ep[i].data.fd, buf, n);
					write(STDOUT_FILENO, buf, n);
				}
			}
		}
	}
	close(lfd);
	return 0;		
}





