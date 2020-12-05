#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
// #include <ldap.h>

namespace fs = std::filesystem;
using namespace std;

#define PORT 5000

// splits strings by a certain delimiter
vector<string> splitByDelimiter(string s, string delimiter)
{
    vector<string> splitedArr;
    uint pos = 0;

    while ((pos = s.find(delimiter)) < s.length())
    {
        splitedArr.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }

    return splitedArr;
}

void handleCommand(int *clientSocket, char buffer[])
{
    ofstream writefile;                                          // To write to the filesystem
    ifstream readfile;                                           // To read from the filesystem
    string path = "./Users/";                                    // Default user directory path
    string request(buffer);                                      // Buffer to string
    string command = request.substr(0, 5);                       // Getting the command from the request
    vector<string> requestArr = splitByDelimiter(request, "\n"); // Vector with each line of the request separate

    if (command == "SEND\n")
    {
        cout << "[Server][SEND] from user " << requestArr[1] << endl;
        request.erase(0, 5);

        writefile.open(path + requestArr[1] + ".txt", ios_base::out | ios_base::app);
        writefile << request;
        writefile.close();

        strcpy(buffer, "");
        strcpy(buffer, "OK\n");
        send(*clientSocket, buffer, strlen(buffer), 0);

        requestArr.clear();
    }
    else if (command == "LIST\n")
    {
        string line;
        int index = 1;
        int count = 0;

        string message = "ID | TITLE\n";
        message += "----------\n";

        cout << "[Server][LIST] from user " << requestArr[1] << endl;

        if (fs::exists(path + requestArr[1] + ".txt"))
        {
            readfile.open(path + requestArr[1] + ".txt");
            while (getline(readfile, line))
            {
                if (index == 3)
                {
                    count++;
                    message += to_string(count) + "  | " + line + "\n";
                }

                index++;

                if (line == ".")
                    index = 1;
            }

            string countStr = "\nCOUNT: " + to_string(count) + "\n";
            message.insert(0, countStr);
            strcpy(buffer, "");
            strcpy(buffer, message.c_str());
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
    else if (command == "READ\n")
    {
        string line, message;
        int lineIndex = 1, messageIndex = 1;

        cout << "[Server][READ] from user " << requestArr[1] << endl;

        if (fs::exists(path + requestArr[1] + ".txt"))
        {
            readfile.open(path + requestArr[1] + ".txt");

            while (true)
            {
                if (line == ".")
                    messageIndex++;

                if (messageIndex == std::stoi(requestArr[2]))
                {
                    while (lineIndex != 4 && getline(readfile, line))
                        lineIndex++;

                    getline(readfile, message);
                    line.clear();
                    break;
                }

                getline(readfile, line);
            }

            strcpy(buffer, "");
            strcpy(buffer, message.c_str());
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
    else if (command.substr(0, 4) == "DEL\n")
    {
        string line;
        string openFilePath = path + requestArr[1] + ".txt";
        string writeFilePath = path + requestArr[1] + "_tmp.txt";
        int messageIndex = 1;

        cout << "[Server][DEL] from user " << requestArr[1] << endl;

        if (fs::exists(openFilePath))
        {
            readfile.open(openFilePath);
            writefile.open(writeFilePath);
            
            while (getline(readfile, line))
            {
                if (messageIndex != std::stoi(requestArr[2]))
                {
                    writefile << line + "\n";
                }

                if (line == ".")
                    messageIndex++;
            }
            
            readfile.close();
            writefile.close();

            remove(openFilePath.c_str());
            rename(writeFilePath.c_str(), openFilePath.c_str());

            strcpy(buffer, "");
            strcpy(buffer, "OK\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
}

int main(int argc, char **argv)
{
    printf("Server is starting... \n");

    // Variables
    int listeningSocket, clientSocket;
    char buffer[1024];
    int opt = 1;
    sockaddr_in listeningSocketAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Create a socket
    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Server] Someting went wrong creating Socket");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) // Set socket to reuse adderess
    {
        perror("[Server] setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the ip address and port to a socket
    listeningSocketAddress.sin_family = AF_INET;                // Set address family (IPv4)
    listeningSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Set any ip address
    listeningSocketAddress.sin_port = htons(PORT);              // Set port to use (network byte order)

    if (bind(listeningSocket, (sockaddr *)&listeningSocketAddress, sizeof(listeningSocketAddress)) == -1) // Bind socket do address
    {
        perror("[Server] Something went wrong while binding Socket");
        exit(EXIT_FAILURE);
    }

    // Start Listening (max request in que 6)
    if (listen(listeningSocket, 6) == -1)
    {
        perror("[Server] Something went wrong on listening");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Server is listening on PORT: %d \n", ntohs(listeningSocketAddress.sin_port));

    // Start accepting clients
    while (true)
    {
        clientSocket = accept(listeningSocket, (sockaddr *)&clientAddress, &clientAddressLength); // Accepting client
        if (clientSocket == -1)
        {
            perror("[Server] Something went wrong accepting client");
            exit(EXIT_FAILURE);
        }

        printf("[Server][Connection Accepted] Client connected from %s:%d... \n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // Welcome message
        string message = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        message += "~~~~~~~~~~Hello to my mail server~~~~~~~~~~\n";
        message += "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        message += "Those are valid commands that you can use\n";
        message += "SEND, LIST, READ, DEL, QUIT \n";

        strcpy(buffer, message.c_str());               // Fill the buffer with the welcome message
        send(clientSocket, buffer, strlen(buffer), 0); // Send the welcome message to the client

        while (true)
        {
            memset(buffer, 0, sizeof(buffer));             // Reset the buffer
            recv(clientSocket, buffer, sizeof(buffer), 0); // Recives a request from the client

            handleCommand(&clientSocket, buffer); //handles the requests
        }

        clientSocket = 0;
    }

    close(listeningSocket);
}