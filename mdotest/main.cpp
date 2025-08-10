#include "main.h"

//server params
#define LISTEN_PORT 80
#define MAX_CONNECTIONS 5

//IO buffer
//this is the only buffer used in this app
#define BUFFER_SIZE 1024
char Buffer[BUFFER_SIZE];

//one client socket for single threaded app
SOCKET ClientSocket = 0;

//get MIME string by file extension
static const char* get_mime_string(const char* file_name)
{
    const char* ext = strrchr(file_name, '.');
    if (ext)
    {
        ++ext;
        if (0 == strcmp(ext, "txt")) return "text/plain";
        if (0 == strcmp(ext, "html")) return "text/html";
        if (0 == strcmp(ext, "jpg")) return "image/jpeg";
        if (0 == strcmp(ext, "jpeg")) return "image/jpeg";
    }
    return "application/octet-stream";
}

//error reply
static void send_error(int http_reply_code)
{
    const char* packet_format = "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s\r\n";

    if (404 == http_reply_code)
        sprintf(Buffer, packet_format, "404 Not Found", "File not found");
    else
        sprintf(Buffer, packet_format, "400 Bad Request", "Request is not supported");

    send(ClientSocket, Buffer, (int)strlen(Buffer), 0);
}

static void send_file(char* buffer_pos, FILE* file, int file_size)
{
    char* buffer_end = Buffer + BUFFER_SIZE;
    fseek(file, 0L, SEEK_SET);
    while (true)
    {
        const size_t max_chunk_size = buffer_end - buffer_pos;

        const size_t bytes_read = fread(buffer_pos, 1, max_chunk_size, file);
        if (bytes_read > 0)
            send(ClientSocket, buffer_pos, (int)bytes_read, 0);

        //assuming end of file or error
        if (bytes_read < max_chunk_size)
            break;

        //reuse io buffer
        buffer_pos = Buffer;
    }
}

static void send_reply(const char* file_name)
{
    //open file
    FILE* file = fopen(file_name, "rb");
    if (NULL == file)
    {
        send_error(404);
        return;
    }

    //get file size
    fseek(file, 0L, SEEK_END);
    const int file_size = ftell(file);

    //write http header
    const char* header_format = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %d\r\n"
                                "\r\n";
    char* buffer_pos = Buffer + sprintf(Buffer, header_format, get_mime_string(file_name), file_size);

    //send file
    send_file(buffer_pos, file, file_size);

    fclose(file);
}

static void process_request(int bytes_read, char* Buffer)
{
    const char* end = Buffer + bytes_read;

    //check HTTP header
    if(bytes_read < 5 || strncmp(Buffer, "GET /", 5) != 0)
    {
        send_error(400);
        return;
    }

    printf("STATE: received GET request:\n%.*s\n", (int)bytes_read, Buffer);

    //get file name
    const char* file_name = NULL;
    for (char* pos = Buffer + 5; pos < end; ++pos)
    {
        if (*pos == ' ')
        {
            *pos = 0;
            file_name = Buffer + 5; //skip "GET /"
            break;
        }

        //file not found
        if (*pos == '\r')
        {
            send_error(404);
            break;
        }
    }
    if (NULL == file_name)
    {
        send_error(404);
        return;
    }

    //reply
    send_reply(file_name);
}

int main()
{
    //init winsock
#ifdef _MSC_VER
    WSADATA wsaData;
    const WORD wVersionRequested = MAKEWORD(2, 2);
    const int result = WSAStartup(wVersionRequested, &wsaData);
    if (result != 0) 
    {
        perror("ERROR: WSAStartup failed");
        return 1;
    }
#endif

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    //create server socket
    const SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0)
    {
        perror("ERROR: create server socket failed");
        exit(EXIT_FAILURE);
    }
    
    //set server socket options
    int opt = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)))
    {
        perror("ERROR: setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    //set server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LISTEN_PORT);
    
    //bind
    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("ERROR: bind() failed");
        exit(EXIT_FAILURE);
    }
    
    //listen
    if(listen(server_socket, MAX_CONNECTIONS) < 0)
    {
        perror("ERROR: listen() failed");
        exit(EXIT_FAILURE);
    }
    
    printf("STATE: listening on port %d...\n", LISTEN_PORT);
    
    while(true)
    {
        //accept
        ClientSocket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if(ClientSocket < 0)
        {
            perror("ERROR: accept() failed");
            continue;
        }
        
        printf("STATE: connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        //read
        const int bytes_read = recv(ClientSocket, Buffer, BUFFER_SIZE - 1, 0);
        if(bytes_read < 0)
        {
            perror("ERROR: read failed");
            close(ClientSocket);
            continue;
        }
        Buffer[bytes_read] = '\0';
        
        //process request
        process_request(bytes_read, Buffer);
        
        close(ClientSocket);
    }
    
    close(server_socket);

#ifdef _MSC_VER
    WSACleanup();
#endif

    return 0;
}
