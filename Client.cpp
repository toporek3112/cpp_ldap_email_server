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
            return input;
            continue;
        }
    }
}

string LDAPLogin(int *clientSocket)
{
    while (true)
    {
        string username, password, message, response;
        char buffer[1024];
        int size = 0;

        cout << "Username: ";
        username = getLineCustom(1, 8);
        password.assign(getpass("Password: "));
        // password.assign(getpass("Password: \n"));

        message += "LOGIN\n";
        message += username + "\n" + password;
        strcpy(buffer, message.c_str());               
        send(*clientSocket, buffer, strlen(buffer), 0);

        strcpy(buffer, "");
        recv(*clientSocket, buffer, sizeof(buffer), 0);

        if (size > 0)
            cout << buffer << endl;
        else if (size == 0)
        {
            printf("Connection closed\n");
            close(*clientSocket);
            exit(EXIT_FAILURE);
        }

        response.clear();
        response = buffer;

        if (response == "OK" || response == "OK\n")
        {
            cout << "Login successful!\n"
                 << endl;
            return username;
        }
        else
        {
            cout << "Login unsuccessful!\n"
                 << endl;
            return "QUIT";
        }
    }
}

int main(int argc, char **argv)
{
    int clientSocket, size;
    char buffer[1024];
    sockaddr_in serverAddress;

    string username = "";
    string input = "";
    string message = "";

    // Create a socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Client] Someting went wrong creating Socket");
        exit(EXIT_FAILURE);
    }

    // Bind the ip address and port to a socket
    serverAddress.sin_family = AF_INET;                     // Set address family (IPv4)
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set IP address (localhost)
    serverAddress.sin_port = htons(PORT);                   // Set port to use (network byte order)

    // Connect
    {
        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0)
        {
            cout << "[Client] Connection with server " << inet_ntoa(serverAddress.sin_addr) << " established" << endl;
            size = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (size > 0)
                cout << buffer << endl;
            else if (size == 0)
            {
                printf("Connection closed\n");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            perror("[Client] Something went wrong while connecting");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }
    }

    // strcpy(buffer, "");
    // strcpy(buffer, "Hello There");
    // send(clientSocket, buffer, strlen(buffer), 0);

    // LDAP login
    for (size_t i = 3; i > 0; i--)
    {
        username.clear();
        username = LDAPLogin(&clientSocket);
        if (username == "QUIT")
        {
            if (i == 1)
            {
                cout << "Too many failed attempts, program will now close!" << endl;
                close(clientSocket);
                return EXIT_FAILURE;
            }
            cout << "login failed try again, attempts left: " << i - 1 << endl;
            continue;
        }
        break;
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

            strcpy(buffer, message.c_str());               // Writing Message to buffer
            send(clientSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer));             // Reseting buffer
            recv(clientSocket, buffer, sizeof(buffer), 0); // Receiving response from server

            cout << buffer << endl;
        }
        else if (message == "LIST")
        {
            message += "\n";
            cout << "Enter username (max 8 characters):" << endl;
            message += getLineCustom(8, 8);
            message += ".\n";

            strcpy(buffer, message.c_str());
            send(clientSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer));             // Reseting buffer
            recv(clientSocket, buffer, sizeof(buffer), 0); // Receiving response from server

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
            send(clientSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer));             // Reseting buffer
            recv(clientSocket, buffer, sizeof(buffer), 0); // Receiving response from server

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
            send(clientSocket, buffer, strlen(buffer), 0); // Sending buffer (message) to server

            memset(buffer, 0, sizeof(buffer));             // Reseting buffer
            recv(clientSocket, buffer, sizeof(buffer), 0); // Receiving response from server

            cout << buffer << endl;
        }
        else
        {
            cout << "Invalid Command" << endl;
        }
    }

    cout << "\n Goodbye :)" << endl;
    close(clientSocket);
    return 0;
}