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
#define BUF 1024

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
        char buffer[BUF];
        int size = 0;

        std::cout << "Username: ";
        username = getLineCustom(1, 8);
        password.assign(getpass("Password: "));
        // password.assign(getpass("Password: \n"));

        message += "LOGIN\n";
        message += username + "\n" + password + "\n";

        strcpy(buffer, message.c_str());
        send(*clientSocket, buffer, strlen(buffer), 0);

        memset(buffer, 0, sizeof(buffer));
        size = recv(*clientSocket, buffer, sizeof(buffer), 0);

        if (size == 0)
        {
            printf("Connection closed\n");
            close(*clientSocket);
            exit(EXIT_FAILURE);
        }

        response.clear();
        response = buffer;

        if (response == "OK\n")
        {
            std::cout << "Login successful!\n"
                      << endl;
            return username;
        }
        else
        {
            std::cout << "Login failed!\n"
                      << endl;
            return response;
        }
    }
}

bool isValidIpAddress(char *st)
{
    int num, i, len;
    char *ch;

    //counting number of quads present in a given IP address
    int quadsCnt = 0;

    len = strlen(st);

    //  Check if the string is valid
    if (len < 7 || len > 15)
        return false;

    ch = strtok(st, ".");

    while (ch != NULL)
    {
        quadsCnt++;

        num = 0;
        i = 0;

        //  Get the current token and convert to an integer value
        while (ch[i] != '\0')
        {
            num = num * 10;
            num = num + (ch[i] - '0');
            i++;
        }

        if (num < 0 || num > 255)
        {
            return false;
        }

        if ((quadsCnt == 1 && num == 0) || (quadsCnt == 4 && num == 0))
        {
            return false;
        }

        ch = strtok(NULL, ".");
    }

    //  Check the address string, should be n.n.n.n format
    if (quadsCnt != 4)
    {
        return false;
    }

    //  Looks like a valid IP address
    return true;
}

int main(int argc, char **argv)
{
    try
    {
        if (!isValidIpAddress(argv[1]) || argv[1] == "")
        {
            cout << "\nERROR: " << argv[0] << " is not a valid IP address or is not set" << endl;
            cout << "The right way to start this client:\n" << endl;
            cout << "./Client [IPADDRESS] [PORT]\n" << endl;
            exit(EXIT_FAILURE);
        }
        if (std::stoi(argv[2]) < 5000 || argv[2] == "")
        {
            cout << "\nERROR: Please select a portnumber thats higher than 4999" << endl;
            cout << "The right way to start this client:\n" << endl;
            cout << "./Client [IPADDRESS] [PORT]\n" << endl;
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    int clientSocket, size;
    char buffer[BUF];
    sockaddr_in serverAddress;

    string input, message, username;
    ;

    // Create a socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Client] Someting went wrong creating Socket");
        exit(EXIT_FAILURE);
    }

    // Bind the ip address and port to a socket
    serverAddress.sin_family = AF_INET;                     // Set address family (IPv4)
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set IP address (localhost)
    serverAddress.sin_port = htons(std::stoi(argv[2]));                   // Set port to use (network byte order)

    // Connect
    {
        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0)
        {
            std::cout << "[Client] Connection with server " << inet_ntoa(serverAddress.sin_addr) << " established" << endl;
            size = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (size > 0)
                std::cout << buffer << endl;
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

    // LDAP login
    for (size_t i = 3; i > 0; i--)
    {
        username = LDAPLogin(&clientSocket);
        if (username == "ERR\n")
        {
            if (i == 1)
            {
                std::cout << "Too many failed attempts, program will now close!" << endl;
                close(clientSocket);
                return EXIT_FAILURE;
            }
            std::cout << "login failed try again, attempts left: " << i - 1 << endl;
            continue;
        }
        else if (username == "ERR2\n")
        {
            std::cout << "User has been locked due to many attempts while logging in. Try again later :)" << endl;
            exit(EXIT_SUCCESS);
        }
        break;
    }

    string commands = "----------------------------------------\n";
    commands += "SEND          Send a new email to someone\n";
    commands += "LIST          List all your emails\n";
    commands += "READ          Read a certain email\n";
    commands += "DEL           Delete a certain email\n";
    commands += "QUIT          Pretty selfexplaining...\n";

    cout << commands << endl;

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
        if (message == "HELP")
            cout << commands << endl;
        else if (message == "SEND") // Send Command
        {
            // Building Message
            message += "\n";
            message += username + "\n";

            cout << "Enter receiver (max 8 characters):" << endl;
            message += getLineCustom(8, 8);
            message += "\n";

            cout << "Enter subject (max 80 characters):" << endl;
            message += getLineCustom(3, 80);
            message += "\n";

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
            message += username + "\n";
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
            message += username + "\n";

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
            message += username + "\n";

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
            std::cout << "Invalid Command" << endl;
        }
    }

    std::cout << "\n Goodbye :)" << endl;
    close(clientSocket);
    return 0;
}