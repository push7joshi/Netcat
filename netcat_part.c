#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <openssl/hmac.h> // need to add -lssl to compile

#define BUF_LEN 1024

/** Warning: This is a very weak supplied shared key...as a result it is not
 * really something you'd ever want to use again :)
 */
static const char key[16] = { 0xfa, 0xe2, 0x01, 0xd3, 0xba, 0xa9,
0x9b, 0x28, 0x72, 0x61, 0x5c, 0xcc, 0x3f, 0x28, 0x17, 0x0e };

/**
 * Structure to hold all relevant state
 **/
typedef struct nc_args{
  struct            sockaddr_in destaddr; //destination/server address
  unsigned short    port; //destination/listen port
  unsigned short    listen; //listen flag
  int               n_bytes; //number of bytes to send
  int               offset; //file offset
  int               verbose; //verbose output info
  int               website; // retrieve website at client 
  char*             filename; //input/output file
}nc_args_t;


void ncListen(nc_args_t *nc_args) {
    
    char destAddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(nc_args->destaddr.sin_addr), destAddr, INET_ADDRSTRLEN);
    printf("\nport:%d\tdest:%s\tlisten:%d", nc_args->port, destAddr, nc_args->listen);
    fflush(stdout);

    //Server socket for listening on nc_args
    int serv_sock, client_sock, client_addr_size;
    struct sockaddr_in serv_addr, client_addr;
    char data[BUF_LEN];
    
    //open the socket
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        printf("\n Cannot create socket!");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    //Set the struct sockaddr
    serv_addr.sin_family = nc_args->destaddr.sin_family;
    serv_addr.sin_port   = nc_args->port;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //optionally bind() the sock
    if( bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("Failed to bind the socket!");
        close(serv_sock);
        exit(EXIT_FAILURE);
    }

    //set listen to up to 5 queued connections
    if ( listen(serv_sock, 5) == -1) {
        printf("Listen failed!");
        exit(EXIT_FAILURE);
    }
    //could put the accept procedure in a loop to handle multiple clientsl
    //accept a client connection
    int msgLen;
    if ((client_sock = accept(serv_sock, NULL, NULL)) >= 0) {
        while((msgLen = recv(client_sock, data, BUF_LEN, 0)) > 0){
            // Do soomething with data
            printf("\n\n%d\tdata::%s\t%d\n",msgLen, data, msgLen);
        }
        //close the connection
        close(client_sock);
    }
}



int nc_client(nc_args_t *nc_args) {
    struct sockaddr_in stSockAddr;
    char data[BUF_LEN];
    char destAddr[INET_ADDRSTRLEN];

    int SocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 
    if (-1 == SocketFD)
    {
      perror("cannot create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(stSockAddr));
 
    stSockAddr.sin_family = nc_args->destaddr.sin_family;
    stSockAddr.sin_port = nc_args->port;
    int Res = inet_ntop(AF_INET, &(nc_args->destaddr.sin_addr), destAddr, INET_ADDRSTRLEN);
    
    if (0 > Res)
    {
      perror("error: first parameter is not a valid address family");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
      perror("char string (second parameter does not contain valid ipaddress)");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
      perror("connect failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    /* perform read write operations ... 
    char * sendBuff = (char *)malloc(100);
    memset(sendBuff, 0, 100);*/
    printf("\nEnter data to send\n");
    scanf("%s", data);
    int sendClient = send(SocketFD, data, strlen(data), 0);
    if (sendClient != -1)
    {
        printf("\ndone!\n");
    }

    (void) shutdown(SocketFD, SHUT_RDWR);
 
    close(SocketFD);
    return EXIT_SUCCESS;
}

/**
 * usage(FILE * file) -> void
 *
 * Write the usage info for netcat_part to the give file pointer.
 */
void usage(FILE * file){
  fprintf(file,
         "netcat_part [OPTIONS]  dest_ip file \n"
         "\t -h           \t\t Print this help screen\n"
         "\t -v           \t\t Verbose output\n"
         "\t -w           \t\t Enable website get mode at client\n"
         "\t -p port      \t\t Set the port to connect on (dflt: 6767)\n"
         "\t -n bytes     \t\t Number of bytes to send, defaults whole file\n"
         "\t -o offset    \t\t Offset into file to start sending\n"
         "\t -l           \t\t Listen on port instead of connecting and write output to file\n"
         "                \t\t and dest_ip refers to which ip to bind to (dflt: localhost)\n"
         );
}

/**
 *parse_args(nc_args_t * nc_args, int argc, char * argv[]) -> void
 *
 * Given a pointer to a nc_args struct and the command line argument
 * info, set all the arguments for nc_args to function use getopt()
 * procedure.
 *
 * Return:
 *     void, but nc_args will have return resutls
 **/

void parse_args(nc_args_t * nc_args, int argc, char * argv[]){
  int ch;
  struct hostent * hostinfo;

  //set defaults
  nc_args->n_bytes  = 0;
  nc_args->offset   = 0;
  nc_args->listen   = 0;
  nc_args->port     = 6767;
  nc_args->verbose  = 0;

  while ((ch = getopt(argc, argv, "hvp:n:o:l")) != -1) {
    switch (ch) {
    case 'h': //help
      usage(stdout);
      exit(0);
      break;
    case 'l': //listen
      nc_args->listen = 1;
      break;
    case 'p': //port
      nc_args->port = atoi(optarg);
      break;
    case 'o'://offset
      nc_args->offset = atoi(optarg);
      break;
    case 'n'://bytes
      nc_args->n_bytes = atoi(optarg);
      break;
    case 'v':
      nc_args->verbose = 1;
      break;
    case 'w':
      nc_args->website = 1;
      break;
    default:
      fprintf(stderr,"ERROR: Unknown option '-%c'\n",ch);
      usage(stdout);
      exit(1);
    }
  }

  argc -= optind;
  argv += optind;
    
  if (argc < 2){
    fprintf(stderr, "ERROR: Require ip and file\n");
    usage(stderr);
    exit(1);
  }

  /* Initial the sockaddr_in based on the parsing */
  if(!(hostinfo = gethostbyname(argv[0]))){
    fprintf(stderr,"ERROR: Invalid host name %s",argv[0]);
    usage(stderr);
    exit(1);
  }

  nc_args->destaddr.sin_family = hostinfo->h_addrtype;
  bcopy((char *) hostinfo->h_addr,
        (char *) &(nc_args->destaddr.sin_addr.s_addr),
        hostinfo->h_length);
  
  //listen or accept data on port specified
    nc_args->destaddr.sin_port = htons(nc_args->port);
  
  /* Save file name */
  nc_args->filename = malloc(strlen(argv[1])+1);
  strncpy(nc_args->filename,argv[1],strlen(argv[1])+1);
  if(nc_args->listen == 1) {
      printf("\ncalling server\n");
      ncListen(nc_args);  
  } else {
    nc_client(nc_args);
  }

  return;

}

int main(int argc, char * argv[]){

  nc_args_t nc_args;
  int sockfd;
  char input[BUF_LEN];

  //initializes the arguments struct for your use
  parse_args(&nc_args, argc, argv);


  /**
   * FILL ME IN
   **/

  return 0;
}