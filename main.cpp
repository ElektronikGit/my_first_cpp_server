#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <cstdio>
#include <fcntl.h>
#include <sys/select.h>

void handle_error(const std::string &msg) {
    perror(&msg[0]);
    exit(EXIT_FAILURE);
}

int main() {
    sockaddr_in addr;
    fd_set master, readfds;

    memset(&addr, 0, sizeof addr);

    addr.sin_family = PF_INET;
    addr.sin_port = htons(9831);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        handle_error("Socket creation failed");
    }

    if (bind(sock_fd, (sockaddr *) &addr, sizeof addr) == -1) {
        handle_error("Binding socket failed");
    }

    if (listen(sock_fd, 32) == -1) {
        handle_error("Listening to socket failed");
    }

    if (fcntl(sock_fd, F_SETFD, O_NONBLOCK) == -1) {
        handle_error("Configuring socket failed");
    }

    FD_ZERO(&master);
    FD_ZERO(&readfds);

    FD_SET(sock_fd, &master);

    int maxfd = sock_fd;

    int new_sockfd;
    char buffer[128];
    ssize_t num_bytes;

    for (;;) {
        memcpy(&readfds, &master, sizeof master);

        std::cout << "Selecting a socket..." << std::endl;
        int num_ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (num_ready == -1) {
            handle_error("Selecting ready sockets failed");
        }

        std::cout << num_ready << " sockets are ready" << std::endl;

        for (int i = 0; i <= maxfd && num_ready > 0; i++) {
            if (FD_ISSET(i, &readfds)) {
                num_ready--;

                if (i == sock_fd) {
                    std::cout << "Trying to accept new connection..." << std::endl;

                    new_sockfd = accept(sock_fd, NULL, NULL);
                    if (new_sockfd == -1) {
                        if (errno != EWOULDBLOCK) {
                            handle_error("Accepting socket failed");
                        }

                        break;
                    } else {
                        if (fcntl(new_sockfd, F_SETFD, O_NONBLOCK) == -1) {
                            handle_error("Configuring new socket failed");
                        }

                        FD_SET(new_sockfd, &master);

                        if (new_sockfd > maxfd) {
                            maxfd = new_sockfd;
                        }
                    }
                } else {
                    std::cout << "Receiving data from descriptor..." << std::endl;

                    memset(&buffer, '\0', sizeof buffer);
//                    char buffer[128];
                    num_bytes = recv(i, buffer, sizeof buffer, 0);
                    if (num_bytes == -1) {
                        if (errno != EWOULDBLOCK) {
                            handle_error("Receiving data from socket failed");
                        }

                        break;
                    }

                    std::cout << "Received message: " << buffer << std::endl;

                    std::cout << "Sending message back" << std::endl;

                    num_bytes = send(i, buffer, sizeof buffer, 0);
                    if (num_bytes == -1) {
                        if (errno != EWOULDBLOCK) {
                            handle_error("Sending data back failed");
                        }

                        break;
                    }
                }
            }
        }
    }

    return 0;
}