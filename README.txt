Compiling and execution:


Makefile:

	Use following commands to build the executable for programs.

		make all   --- compiles netcat_part.c & netcat_hmac.c. Creates executables 'ncat' & 'ncatcrypto'
		make clean --- cleans (removes) the executables.

	You can build specific files using following commands.
		make ncat 	   --- compiles netcat_part.c
		make ncatCrypt --- compiles netcat_hmac.c

Execution:

	Server:
	./ncat -l localhost server.txt
	Client:
	./ncat localhost client.txt
	./ncat -w localhost TMP		(Get web response for GET request mentioned in TMP)

Use different switches mentioned below for various modes.

Inputs: These inputs should be provided as command line arguments in the following order.
IpAddress - This is the ipAddress of the server to be setup or the server the client wishes to connect to.
File - A file which has different purposes in the various modes.

Switches
The program accepts the following switches
-h  - Prints the list of options/switches
-v  - Output is verbose : specify what the process is doing.
-p  - Set the port number(Default value is 6767)
-o  - Set the offset. Used in file transfer mode to speciy number of bytes to ignore from the start of the file while sending to the server.
-n  - Set the number of bytes to be sent. Used in file transfer mode to specify the number of bytes of file sent by the client.
-l  - Server Mode. Starts a server at ipaddress(specified in the input) which listens for connections and writes incoming data to the file specified in the input.
-w  - Website mode. Sends a query to the ipaddress(generally a web server) specified in the input. The query must be in file provided.
      Generally the query will be a get query. The result is writen into a file called 'webFile.txt'(data is overwritten).

default mode: If -l or -w is not present the program is started as client which sends file(Needs an active server listening on specified port). The file to be sent is specified as a cmd line argument. The file is sent to the ip address and the port specified by -p(default 6767). The -o and -n parameters are applied when specified.


Overview of the types/methods
type nc_args_t: This stores all the information about the inputs as well as the options/switches. Has flags corresponding to each switch which is set during the function parse_args().

function ncListen(nc_args_t *nc_args): This method sets up a server for data transfer. It is invoked in the server mode that is when the -l switch is used. Sets up a socket to listen to the incoming connections. It stores all the data it receives in the file specified as cmd line argument.

function nc_client(mc_args_t* nc_args): This sets up a client for the data transfer.
If the -w is used a  web client is set up that sends a get request to the web site specified in the file argument. It creates a socket sends a request as in the input file to the web server and waits for the response. Response is written into webFile.txt
If neither -w nor -l is used, a client is created. It reads from the file provided as input sends it to the server whose ip is given as input. It creates a socket and sends data through that socket. If the file is larger than 1024 bytes data is sent in the form of chunks. The offset and number of bytes parameters(-o and -n switches respectively) are applied.

function usage() - Prints the help menu. Called when the -h flag is used

function parse_args() - It parses the arguments and options provided and populates N_args_t type struct. The calls to the appropriate functions are made in this function.

function main() - The entry point to the program. Handles the main program flow.

netcat_hmac.c - This contains the portions corresponding to HMAC. We were not able to integrate this with the net_cat but have made the HMAC feature. When the make command is run, this creates ncat_crypto. When run in the regular client server mode, this reads from the input file from the client side and computes the hash. The hash + message + hash_len is sent to the server. The server recomputes hash and if the hashes match the output is written to the stdout. The data structure Hash_Msg is used to represent the hash, hash_length and data/message. The method get_hash computes the hash using SHA1. The check_hash method checks takes a Hash_Msg pointer as input. It computes the hash for the data and checks it with the hash it has.




