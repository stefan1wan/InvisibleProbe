
struct ip_srv_conf{
    int port;
    int sockfd;
    char* servername;
};

int ip_client_connect(struct ip_srv_conf* comm);
int ip_server_connect(struct ip_srv_conf* comm);