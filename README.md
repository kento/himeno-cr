
RDMA Communication Runtime
==========================

This runtime is a client/server model communication runtime over infiniband interconntects.
The runtime is develeped by using ibverbs.



Directly Structure
------------------
* libibtl  
    * src    : Directory for source codes  
    * include: Directory for header files  
    * example: Directory for example codes  



HOW to Build & Install
------------
Run commands below:   

    $ ./configure --prefix=/path/to/install/dir
    $ make 
    $ make install



Quick Start
-------------
### Build examples ###

Run commands below:  
  
    $ cd /path/to/install/dir
    $ cd examples


Edit makefile (examples/makefile):  

    INSTALL_DIR = /path/to/install/dir


Make:  

    $ make

### Run example 1: Simple communication ###
The example codes simply exchanges messages (Ping-Pong) initiated by a client side

Run server code:  

    sierra0$ ./example_server


Run client cond:  

    sierra1$ ./example_client sierra0



### Run example 2: RDMA I/O  ###
The client example code wirte/read a spedified file on the remote server.

Run server code:  

    sierra0$ ./ibio_server


Run client code:  

    sierra1$ ./ibio_test sierra0:/path/to/file 0 # write
    sierra1$ ./ibio_test sierra0:/path/to/file 1 # read



RDMA Communication APIs & Variables
-----------------------
### APIs

#### Initialization
    int fdmi_verbs_init(int *argc, char ***argv)  
This function must be called before any communication functions  

* `argc` [input]: Pointer to the number of arguments  
* `argv` [input]: Argument vector  
    
#### Finalization
    int fdmi_verbs_finalize()  
This function finalize the communications

#### Connection (Used by only clients )
    void fdmi_connection* fdmi_verbs_connect(int id, char *hostname);
This function make connection to a specified server, create mapping from `id` to `hostname`.
`id` is used by the rest of communication function calls (send/redv).

* `id` [input]: integer to assinge the host
* `hostname` [input]: a server to connect
    
#### Non-blocking Send
    int fdmi_verbs_isend (const void* buf, int count, struct fdmi_datatype dataype, int dest, int tag, struct fdmi_communicator *comm, struct fdmi_request* request);
This function provides non-blocking send.

* `buf` [input]: Initial address of send buffer
* `count` [input]: Number of elements in send buffer
* `datatype` [input]: Datatype of each send buffer element
* `dest` [input]: `id` of destination (Assigned `id` via fmdi_verbs_connection)
* `tag` [input]: Message tag
* `comm` [input]: Communicator (Must specify FMI_COMM_WORLD)
* `request` [output]: Communication request

#### Non-blocking Recv
    int fdmi_verbs_irecv(const void* buf, int count, struct fdmi_datatype dataype,  int source, int tag, struct fdmi_communicator *comm, struct fdmi_request* request);
This function provides non-blocking receive.

* `buf` [input]: Initial address of receive buffer
* `count` [input]: Number of elements in receive buffer
* `datatype` [input]: Datatype of each receive buffer element
* `source` [input]: `id` of source (Assigned `id` via fmdi_verbs_connection)
* `tag` [input]: Message tag
* `comm` [input]: Communicator (Must specify FMI_COMM_WORLD)
* `request` [output]: Communication request

#### Test    
    int fdmi_verbs_test(struct fdmi_request *request, struct fdmi_status *staus);
This function returns 1 if the send/recv operation identified by `request` is complete. Otherwise, this function returns 0.

* `request` [input]: Communication request
* `status` [output]: Status object

#### Wait
    int fdmi_verbs_wait(struct fdmi_request* request, struct fdmi_status *status);
This function is blocking fdmi_verbs_test.
This function returns 1 if the send/recv operation identified by `request` is complete. Otherwise, this function blocks the call until the request gets complete.
  
* `request` [input]: Communication request
* `status` [output]: Status object

#### Iprove
    int fdmi_verbs_iprobe(int source, int tag, struct fdmi_communicator* comm, int *flag, struct fdmi_status *status);
This function allows checkpoint of incoming messages without actual receipt of them.
The user can then decide how to receive them, based on the information returned by this function.
This function returns `flag`=1 if messages with `source` and `tag` is ready to receive, then put the information to `status`. Otherwise this function just returns `flag`=0.

* `source` [input]: Source `id ` or FMI_ANY_SOURCE
* `tag` [input]: Tag value or FMI_ANY_TAG
* `comm` [input]: Communicator (Must specify FMI_COMM_WORLD)
* `flag` [output]: Message-waiting flag
* `status` [output]: Status object

### Variables
#### Datatype
* FMI_INT: int
* FMI_DOUBLE: double
* FMI_FLOAT: float
* FMI_BYTE: like unsigned char
* FMI_CHAR: char

#### Status
    FMI_Status
* FMI_SOURCE: `id` of processes sending the message    
* FMI_TAG:`tag` of the message


Example
-------

Ping-Pong examples initiated by a client

### Server side

    FMI_Request req;
    FMI_Status stat;
    int flag;
    char buff[256];

    fdmi_verbs_init(&argc, &argv);


    while (1) {
      fdmi_verbs_iprobe(FMI_ANY_SOURCE, FMI_ANY_TAG, FMI_COMM_WORLD, &flag, &stat);
      /** 
       * After the initiation, a server assinge unique id to each client  when the clients connected
       * The assigned id can be accessed through FMI_status (i.e. stat.FMI_SOURCE).
       */
      if (flag) {
        fprintf(stderr, "Receved message: source=%d , tag=%d\n", stat.FMI_SOURCE, stat.FMI_TAG);

        fdmi_verbs_irecv(buff, sizeof(buff), FMI_BYTE, stat.FMI_SOURCE, stat.FMI_TAG, FMI_COMM_WORLD, &req);
        fdmi_verbs_wait(&req, NULL);
        fprintf(stderr, "               : hostname=%s, assigned_id=%d\n", buff, stat.FMI_SOURCE);

        sprintf(buff, "%d", stat.FMI_SOURCE);

        fdmi_verbs_isend(buff, 1, FMI_INT, stat.FMI_SOURCE, stat.FMI_TAG, FMI_COMM_WORLD, &req);
        fdmi_verbs_wait(&req, NULL);   
        fprintf(stderr, "Send    message: destination_id(assigned_id)=%d\n",  stat.FMI_SOURCE);
    }

### Client side

    FMI_Request req;
    FMI_Status stat;

    char server_hostname[256];
    char buff[256];
    int  server_hostid = 0;
    int  tag = 5;
  
    if (argc != 2) {
      fdmi_err("example_client <server_hostname>");
    }
    sprintf(server_hostname, "%s", argv[1]);
  
    /* Initialization */
    fdmi_verbs_init(&argc, &argv);

    /**
     *  Connect to a server(server_hostname).
     *  Now I can exchange messages via id (server_hostid) 
     *  Note:
     *     Any server_hostid can be assigned to server_hostname.
     *     But the server_hostid must be a unique number amoung servers.
     */
    fdmi_verbs_connect(server_hostid, server_hostname); /*Mapping: server_hostid -> server_hostname*/
    printf("Connected: server_hostname=%s, server_hostid=%d\n", server_hostname, server_hostid);

    gethostname(buff, sizeof(buff));
  
    /* Send my hostname to the connected server*/
    fdmi_verbs_isend(buff, sizeof(buff), FMI_BYTE, server_hostid, tag, FMI_COMM_WORLD, &req);
    fdmi_verbs_wait(&req, &stat);
    printf("Send    : msg=%s\n", buff);

    /* Receive assigned id by the connected server */
    fdmi_verbs_irecv(buff, 1, FMI_INT, server_hostid, FMI_ANY_TAG, FMI_COMM_WORLD, &req);
    fdmi_verbs_wait(&req, &stat);
    printf("Recv    : assigned_id=%s\n", buff);
  
    fdmi_verbs_finalize();
