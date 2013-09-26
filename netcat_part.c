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
#define DATA_LEN 1000


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
    FILE* fp;
    
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
    serv_addr = nc_args->destaddr;
    
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
    fp = fopen(nc_args->filename, "w");
    if ((client_sock = accept(serv_sock, NULL, NULL)) >= 0) {
        while((msgLen = recv(client_sock, data, DATA_LEN, 0)) > 0){
                // Do something with data
            fwrite(data, 1, strlen(data), fp);
            printf("\ndata::%s\n", data);
            memset(data,'\0', strlen(data));
        }
        fclose(fp);
            //close the connection
        close(client_sock);
    }
}



int nc_client(nc_args_t *nc_args) {
    struct sockaddr_in stSockAddr;
    char destAddr[INET_ADDRSTRLEN];
    FILE* fp;
    int SocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (-1 == SocketFD)
    {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    
    stSockAddr = nc_args->destaddr;
    
    if (-1 == connect(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }
    
    int fileSize = 0, numBytes = nc_args->n_bytes, offset = nc_args->offset, buffSize;
    char* buff;
    printf("\nFilename:%s",nc_args->filename);
        //Open the file and read it into a buffer.
    if (fp = fopen(nc_args->filename, "r")) {
        
            //check if file is empty
        fseek(fp, 0, SEEK_END);
        fileSize = ftell(fp);
        rewind(fp);
        if (fileSize == 0) {
            printf("\n Why do you want to send an empty file?");
            exit(0);
        }
        
            //Allocate buffer for data read
        if(fileSize < DATA_LEN) {
            buffSize = fileSize;
        } else {
            buffSize = DATA_LEN;
        }
        
            //Read the file into buffer.
        int countRead =0, remBytes = fileSize;
        while(remBytes > 0) {
            
            if (remBytes < DATA_LEN) {
                buff = (char *)malloc(remBytes) ;
                buffSize = remBytes;
                printf("\nRemaining:%d", remBytes);
            } else {
                buff = (char *)malloc(buffSize);
            }
                
            
            if(buff == NULL) {
                printf("Allocation error!");
                exit(1);
            }
            countRead +=  fread(buff, sizeof(char), buffSize, fp);
            remBytes -= buffSize;
           
            printf("\n%d", buffSize);
           
                //Send the buffer to the server.
            int sendClient = send(SocketFD, buff, strlen(buff)-1, 0);
            if (sendClient == -1) {
                printf("\nError sending!\n");
                exit(EXIT_FAILURE);
            }
        }
        fclose(fp);
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

int parse_args(nc_args_t * nc_args, int argc, char * argv[]){
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
        printf("\nRunning server -\n");
        ncListen(nc_args);  
    } else {
        nc_client(nc_args);
    }
    return 0;
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
