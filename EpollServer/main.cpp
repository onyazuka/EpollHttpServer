#include <cstdio>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <iostream>

using namespace std;

int main()
{
    static constexpr int PORT = 8080;
    static constexpr int MAX_LISTENING_CLIENTS = 128;
    static constexpr int MAX_EPOLL_EVENTS = 10;
    char buf[4 * 1024 * 1024];
    int serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (serverFd < 0) {
        cerr << "Error while creating socket\n";
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error while binding socket\n";
        close(serverFd);
        return -1;
    }

    if (listen(serverFd, MAX_LISTENING_CLIENTS) < 0) {
        cerr << "Error while listening socket\n";
        close(serverFd);
        return -1;
    }

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        cerr << "Error while creating epoll\n";
        close(serverFd);
        return -1;
    }

    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLIN /* | EPOLLOUT | EPOLLHUP | EPOLLERR */;
    event.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) < 0) {
        cerr << "Error on epoll ctl\n";
        close(serverFd);
        close(epollFd);
        return -1;
    }

    while (true) {
        int numEvents = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, -1);
        if (numEvents < 0) {
            cerr << "Error on epoll waiting\n";
            close(serverFd);
            close(epollFd);
            return -1;
        }
        for (int i = 0; i < numEvents; ++i) {
            if (events[i].data.fd == serverFd) {
                // handle new connection
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if ((clientFd < 0) && (errno != EAGAIN)) {
                    cerr << "Failed to accept client connection\n";
                    continue;
                }
                else if (errno == EAGAIN) {
                    ;
                }
                else {
                    event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
                    event.data.fd = clientFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) < 0) {
                        cerr << "Failed to add client socket to epoll instance\n";
                        close(clientFd);
                        continue;
                    }
                    cout << "Handing client " << clientFd << endl;
                }
            }
            else {
                int clientFd = events[i].data.fd;
                // handle client data
                if (events[i].events & EPOLLIN) {
                    cout << "Handling client data " << clientFd << endl;
                    bool err = false;
                    for (;;) {
                        ssize_t n = read(clientFd, buf, sizeof(buf));
                        if (n <= 0 && (errno != EAGAIN)) {
                            epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
                            close(clientFd);
                            err = true;
                            break;
                        }
                        else if (errno == EAGAIN)
                        {
                            continue;
                        }
                        else {
                            cout << "Read " << n << " bytes from " << clientFd << endl;
                            //break;
                        }
                    }
                    if (err) {
                        ;
                    }
                }
                else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                    cout << "Client connection closed " << clientFd << endl;
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);
                    continue;
                }
                continue;
            }
        }
    }

    return 0;
}