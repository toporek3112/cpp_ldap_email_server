#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

using namespace std;

#define PORT 5000

string getLineCustom(int min, int max)
{
    string input = "";

    while (true)
    {
        input.clear();
        getline(cin, input);

        if (input == "QUIT")
        {
            exit(EXIT_SUCCESS);
        }
        else if (input.length() > max || input.length() < min)
        {
            cout << "Input to long or to short try again: " << endl;
        }
        else
        {
            input += "\n";
            return input;
            continue;
        }
    }
}

int main(int argc, char **argv)
{
    int clinetSocket, size;
    char buffer[1024];
    sockaddr_in serverAddress;

    string username = "if20b039";
    string input = "";
    string message = "";

    // Create a socket
    if ((clinetSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Client] Someting went wrong creating Socket");
        exit(EXIT_FAILURE);
    }

    // Bind the ip address and port to a socket
    serverAddress.sin_family = AF_INET;                     // Set address family (IPv4)
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set IP address (localhost)
    serverAddress.sin_port = htons(PORT);                   // Set port to use (network byte order)

    strcpy(buffer, "");

    // Connect
    if (connect(clinetSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0)
    {
        cout << "[Client] Connection with server " << inet_ntoa(serverAddress.sin_addr) << " established" << endl;
        size = recv(clinetSocket, buffer, sizeof(buffer), 0);
        if (size > 0)
        {
            printf("Bytes received: %d\n \n", size);
            cout << buffer << endl;
        }
        else if (size == 0)
            printf("Connection closed\n");
    }
    else
    {
        perror("[Client] Something went wrong while connecting");
        exit(EXIT_FAILURE);
    }

    // reading and sending commands in while loop
    while (true)
    {
        message.clear();
        input.clear();
        memset(buffer, 0, sizeof(buffer)); // Reseting buffer

        printf("> "); 
        getline(cin, message); // Read Command

        transform(message.begin(), message.end(), message.begin(), ::toupper); // Commands to Upper

        if (message == "QUIT")
            break;
        else if (message == "SEND") // Send Command
        {
            // Building Message
            message += "\n";
            cout << "Enter sender (max 8 characters):" << endl;
            message += getLineCustom(8, 8);

            cout << "Enter receiver (max 8 characters):" << endl;
            message += getLineCustom(8, 8);

            cout << "Enter subject (max 80 characters):" << endl;
            message += getLineCustom(3, 80);

            cout << "Enter message:" << endl;
            getline(cin, input);
            message += input;
            message += "\n.\n";

            strcpy(buffer, message.c_str()); // Writing Message to buffer
            send(clinetSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer)); // Reseting buffer
            recv(clinetSocket, buffer, sizeof(buffer), 0); // Receiving response from server 

            cout << buffer << endl;
        }
        else if (message == "LIST")
        {
            message += "\n";
            cout << "Enter username (max 8 characters):" << endl;
            message += getLineCustom(8, 8);
            message += ".\n";

            strcpy(buffer, message.c_str());
            send(clinetSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer)); // Reseting buffer
            recv(clinetSocket, buffer, sizeof(buffer), 0); // Receiving response from server 

            cout << buffer << endl;
        }
        else if (message == "READ")
        {
            message += "\n";
            cout << "Enter username (max 8 characters):" << endl;
            message += getLineCustom(8, 8);

            cout << "Enter message number:" << endl;
            getline(cin, input);
            message += input;
            message += "\n.\n";

            strcpy(buffer, message.c_str());
            send(clinetSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer)); // Reseting buffer
            recv(clinetSocket, buffer, sizeof(buffer), 0); // Receiving response from server 

            cout << "\ntext:" << endl;
            cout << buffer << endl;
            cout << endl;
        }
        else if (message == "DEL")
        {
            message += "\n";
            cout << "Enter username (max 8 characters):" << endl;
            message += getLineCustom(8, 8);

            cout << "Enter message number:" << endl;
            getline(cin, input);
            message += input;
            message += "\n.\n";

            strcpy(buffer, message.c_str());
            send(clinetSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer)); // Reseting buffer
            recv(clinetSocket, buffer, sizeof(buffer), 0); // Receiving response from server 

            cout << buffer << endl;
        }
        else
        {
            cout << "Invalid Command" << endl;
        }
    }

    cout << "\n Goodbye :)" << endl;
    close(clinetSocket);
    return 0;
}