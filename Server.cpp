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
#include <thread>
#include <ldap.h>
#include <mutex>

namespace fs = std::filesystem;
using namespace std;

#define LDAP_URI "ldap://ldap.technikum-wien.at:389"
#define SEARCHBASE "dc=technikum-wien,dc=at"
#define SCOPE LDAP_SCOPE_SUBTREE
#define FILTER "(uid=if*b*)"
#define BIND_USER "" /* anonymous bind with user and pw empty */
#define BIND_PW ""

#define BUF 1024
#define LOCKTIME 300

std::mutex _listManagerLock;
std::vector<std::thread> threads;

// splits strings by a certain delimiter
vector<string> split(string str, string token, int count)
{
    vector<string> result;
    int i = 0;
    while (str.size())
    {
        if (i < count || count == 0)
        {
            int index = str.find(token);
            if (index != string::npos)
            {
                result.push_back(str.substr(0, index));
                str = str.substr(index + token.size());
                i++;
                if (str.size() == 0)
                    result.push_back(str);
            }
            else
            {
                result.push_back(str);
                str = "";
            }
        }
        else
        {
            result.push_back(str);
            break;
        }
    }
    return result;
}

string replaceAll(std::string str, const std::string &from, const std::string &to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

size_t getFilesCount(std::filesystem::path path)
{
    std::lock_guard<std::mutex> lock(_listManagerLock);
    using std::filesystem::directory_iterator;
    return std::distance(directory_iterator(path), directory_iterator{});
    _listManagerLock.unlock();
}

// LDAP Login
bool HandleLogin(string credentials)
{
    vector<string> messElements = split(credentials, "\n", NULL);

    LDAP *ld, *ld2;          /* LDAP resource handle */
    LDAPMessage *result, *e; /* LDAP result handle */
    BerElement *ber;         /* array of attributes */
    char *attribute;
    BerValue **vals;

    BerValue *servercredp;
    BerValue cred;
    cred.bv_val = (char *)BIND_PW;
    cred.bv_len = strlen(BIND_PW);
    int i, rc = 0;
    int rc2 = 0;

    const char *attribs[] = {"uid", "cn", NULL}; /* attribute array for search */
    string uid = "uid=" + messElements[1];

    int ldapversion = LDAP_VERSION3;

    /* setup LDAP connection */
    if (ldap_initialize(&ld, LDAP_URI) != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_init failed");
        return false;
    }

    printf("connected to LDAP server %s\n", LDAP_URI);

    if ((rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldapversion)) != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return false;
    }

    if ((rc = ldap_start_tls_s(ld, NULL, NULL)) != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return false;
    }

    /* anonymous bind */
    rc = ldap_sasl_bind_s(ld, BIND_USER, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &servercredp);

    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return false;
    }
    else
    {
        printf("anonymous bind successful\n");
    }

    /* perform ldap search */
    rc = ldap_search_ext_s(ld, SEARCHBASE, SCOPE, uid.c_str(), (char **)attribs, 0, NULL, NULL, NULL, 500, &result);

    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP search error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ld, NULL, NULL);
        return false;
    }

    printf("Total results: %d\n", ldap_count_entries(ld, result));

    string userDN = "";

    for (e = ldap_first_entry(ld, result); e != NULL; e = ldap_next_entry(ld, e))
    {

        userDN = ldap_get_dn(ld, e);

        for (attribute = ldap_first_attribute(ld, e, &ber); attribute != NULL; attribute = ldap_next_attribute(ld, e, ber))
        {
            if ((vals = ldap_get_values_len(ld, e, attribute)) != NULL)
            {
                for (i = 0; i < ldap_count_values_len(vals); i++)
                {
                    printf("\t%s: %s\n", attribute, vals[i]->bv_val);

                    /* setup LDAP connection */
                    if (ldap_initialize(&ld2, LDAP_URI) != LDAP_SUCCESS)
                    {
                        fprintf(stderr, "ldap_init failed");
                        return false;
                    }

                    printf("connected to LDAP server %s\n", LDAP_URI);

                    if ((rc = ldap_set_option(ld2, LDAP_OPT_PROTOCOL_VERSION, &ldapversion)) != LDAP_SUCCESS)
                    {
                        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
                        ldap_unbind_ext_s(ld2, NULL, NULL);
                        return false;
                    }

                    if ((rc = ldap_start_tls_s(ld2, NULL, NULL)) != LDAP_SUCCESS)
                    {
                        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
                        ldap_unbind_ext_s(ld2, NULL, NULL);
                        return false;
                    }

                    string pwd = messElements[2];

                    cred.bv_val = (char *)pwd.c_str();
                    cred.bv_len = strlen(pwd.c_str());

                    //TODO: BIND user with password
                    rc2 = ldap_sasl_bind_s(ld2, userDN.c_str(), LDAP_SASL_SIMPLE, &cred, NULL, NULL, &servercredp);

                    if (rc2 != LDAP_SUCCESS)
                    {
                        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
                        ldap_unbind_ext_s(ld2, NULL, NULL);
                        ldap_value_free_len(vals);
                        ldap_memfree(attribute);
                        ldap_msgfree(result);
                        ldap_unbind_ext_s(ld, NULL, NULL);
                        if (ber != NULL)
                            ber_free(ber, 0);
                        return false;
                    }
                    else
                    {
                        printf("bind successful\n");
                        ldap_value_free_len(vals);
                        ldap_memfree(attribute);
                        ldap_msgfree(result);
                        ldap_unbind_ext_s(ld, NULL, NULL);
                        if (ber != NULL)
                            ber_free(ber, 0);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// User management

void lockUser(string clientIp)
{
    {
        std::lock_guard<std::mutex> lock(_listManagerLock);
        ofstream writeStream;
        string path = "./Black_List/";

        if (!fs::exists(path))
            fs::create_directory(path);

        time_t t = time(0);
        string tstamp = to_string(t + LOCKTIME);

        if (!fs::exists(path + clientIp + ".txt"))
        {
            writeStream.open(path + clientIp + ".txt");
            writeStream << tstamp;
            writeStream.close();
        }
        else
        {
            writeStream.open(path + clientIp + ".txt", fstream::out);
            writeStream << tstamp;
            writeStream.close();
        }

        _listManagerLock.unlock();
    }
}

void unlockUser(string clientIp)
{
    {
        std::lock_guard<std::mutex> lock(_listManagerLock);
        string path = "./Black_List/";

        fs::remove(path + clientIp + ".txt");

        _listManagerLock.unlock();
    }
}

bool checkUserNotLocked(string clientIp)
{
    {
        int lockTime;
        string path = "./Black_List/";

        try
        {
            std::lock_guard<std::mutex> lock(_listManagerLock);

            if (!fs::exists(path))
                fs::create_directory(path);

            if (fs::exists(path + clientIp + ".txt"))
            {
                ifstream readStream;
                readStream.open(path + clientIp + ".txt");
                readStream >> lockTime;
                readStream.close();
            }
            _listManagerLock.unlock();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return true;
        }

        time_t t = time(0);

        if (lockTime < t)
        {
            return true;
        }

        return false;
    }
}

// Server commands
void handleCommand(int *clientSocket, char buffer[])
{
    ofstream writefile;                                     // To write to the filesystem
    ifstream readfile;                                      // To read from the filesystem
    string path = "./Users/";                               // Default user directory path
    string request(buffer);                                 // Buffer to string
    string command = request.substr(0, 5);                  // Getting the command from the request
    vector<string> requestArr = split(request, "\n", NULL); // Vector with each line of the request separate
    string userPath = path + requestArr[1] + "/";           // Sets the path of the users directory
    string line;
    int id = 1;

    if (command == "SEND\n") // Sends new mail
    {

        cout << "[Server][SEND] from user " << requestArr[1] << endl; // Server output
        request.erase(0, 5);                                          // Delete the first 5 characters (the SEND)

        std::lock_guard<std::mutex> lock(_listManagerLock);

        if (!fs::exists(userPath)) // Check if user repository exists
        {
            fs::create_directory(userPath); // If not create one
            writefile.open(userPath + "id.txt");
            writefile << "1";
            writefile.close();
        }
        else
        {
            readfile.open(userPath + "id.txt");
            getline(readfile, line); // Else get the filecount from the users direcotry
            id = std::stoi(line);
        }

        writefile.open(userPath + "/" + to_string(id) + "_" + replaceAll(requestArr[3], " ", "_") + ".txt", ios_base::out); // Create new File (messagenumber_subject.txt)
        writefile << request;                                                                                               // Write to file
        writefile.close();                                                                                                  // Clos file

        writefile.open(userPath + "id.txt");
        writefile << ++id;
        writefile.close();

        strcpy(buffer, "");                             // Reset buffer
        strcpy(buffer, "OK\n");                         // Write to buffer
        send(*clientSocket, buffer, strlen(buffer), 0); // Send response to client

        requestArr.clear(); // clear array
    }
    else if (command == "LIST\n") // Lists all mail of a user
    {
        vector<string> nameArr;
        string tmpPath;

        string message = "ID | TITLE\n";
        message += "----------\n";

        cout << "[Server][LIST] from user " << requestArr[1] << endl;

        if (fs::exists(userPath)) // Check if user directory exists
        {
            int filesCount = getFilesCount(userPath) - 1;
            for (const auto &file : fs::directory_iterator(userPath)) // Interate through the directory
            {
                tmpPath = file.path(); // Gets the whole path
                if (tmpPath != userPath + "id.txt")
                {
                    tmpPath.erase(0, userPath.length());               // Gets the filename
                    tmpPath.resize(tmpPath.length() - 4);              // Gets rid of the .txt
                    nameArr = split(tmpPath, "_", 1);                  // Spliting the filename on '_' only once
                    message += nameArr[0] + " | " + nameArr[1] + "\n"; // Builds the message
                }
            }

            string countStr = "\nCOUNT: " + to_string(filesCount) + "\n"; // Build the first line of the response (count)
            message.insert(0, countStr);                                  // Insert the emails count at the beginning of the message
            strcpy(buffer, "");                                           // Reset buffer
            strcpy(buffer, message.c_str());                              // Write message to buffer
            send(*clientSocket, buffer, strlen(buffer), 0);               // Send response to client
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
    else if (command == "READ\n") // Reads a email (mail number)
    {
        vector<string> pathArr;
        string message, tmpPath;
        size_t found;
        int lineIndex = 1;
        cout << "[Server][READ] from user " << requestArr[1] << endl;

        if (fs::exists(userPath)) // Check if uer directory exists
        {
            for (const auto &file : fs::directory_iterator(userPath)) // Interate through the directory
            {
                tmpPath = file.path();                  // Gets the whole path
                pathArr = split(tmpPath, "/", NULL);    // Splits the path on '/'
                found = pathArr[3].find(requestArr[2]); // Searchers for the message number in the filname

                if (found != string::npos && found == 0) // Checks if message number was found and if it is on the first position
                {
                    readfile.open(tmpPath); // Opens the file

                    while (getline(readfile, line)) // reads the files
                    {
                        if (lineIndex == 4) // Checks if it is the right line
                        {
                            message += line; // builds the message
                            break;
                        }
                        lineIndex++;
                    }
                }
            }

            if (message == "") // Checks if massage is empty
            {
                strcpy(buffer, "");
                strcpy(buffer, "ERR\n");
                send(*clientSocket, buffer, strlen(buffer), 0);
            }
            else
            {
                strcpy(buffer, "");
                strcpy(buffer, message.c_str());
                send(*clientSocket, buffer, strlen(buffer), 0);
            }
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
    else if (command.substr(0, 4) == "DEL\n") // deletes a email (mail number)
    {
        vector<string> pathArr;
        string tmpPath, line;
        size_t found;
        bool removed = false;
        int lineIndex = 1;
        cout << "[Server][DEL] from user " << requestArr[1] << endl;

        if (fs::exists(userPath)) // Check if uer directory exists
        {
            for (const auto &file : fs::directory_iterator(userPath)) // Interate through the directory
            {
                tmpPath = file.path();                  // Gets the whole path
                pathArr = split(tmpPath, "/", NULL);    // Splits the path on '/'
                found = pathArr[3].find(requestArr[2]); // Searchers for the message number in the filname

                if (found != string::npos && found == 0) // Checks if message number was found and if it is on the first position
                {
                    remove(tmpPath.c_str());
                    removed = true;
                    break;
                }
            }

            if (removed == false) // Checks if massage is empty
            {
                strcpy(buffer, "");
                strcpy(buffer, "ERR\n");
                send(*clientSocket, buffer, strlen(buffer), 0);
            }
            else
            {
                strcpy(buffer, "");
                strcpy(buffer, "OK\n");
                send(*clientSocket, buffer, strlen(buffer), 0);
            }
        }
        else
        {
            strcpy(buffer, "");
            strcpy(buffer, "ERR\n");
            send(*clientSocket, buffer, strlen(buffer), 0);
        }
    }
}

void handleClient(int clientSocket, string clientIp)
{
    string request;
    char buffer[BUF];
    int size;
    int tries = 0;
    bool loggedIn = false;

    do
    {
        size = recv(clientSocket, buffer, BUF, 0);
        if (size > 0)
        {
            if (!loggedIn)
            {

                if (checkUserNotLocked(clientIp))
                {
                    unlockUser(clientIp);
                }
                else
                {
                    memset(buffer, 0, sizeof(buffer)); // Rest buffer
                    strcpy(buffer, "ERR2\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                    break;
                }

                loggedIn = HandleLogin(buffer);

                memset(buffer, 0, sizeof(buffer)); // Rest buffer
                if (loggedIn)
                {
                    strcpy(buffer, "OK\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                }
                else
                {
                    tries++;
                    if (tries == 3)
                    {
                        cout << "[Server] User:" << clientIp << " has been locked" << endl;
                        lockUser(clientIp);
                        break;
                    }

                    strcpy(buffer, "ERR\n");
                    send(clientSocket, buffer, strlen(buffer), 0);
                }
            }
            else
            {
                handleCommand(&clientSocket, buffer);
            }
        }
        else if (size == 0)
        {
            cout << "Client: " << clientIp << " closed the connection \n"
                 << endl;
            break;
        }
        else
        {
            perror("Error while receiving ");
            break;
        }

    } while (strncmp(buffer, "quit", 4) != 0);

    close(clientSocket);
}

int main(int argc, char **argv)
{
    printf("Server is starting... \n");

    try
    {
        if (std::stoi(argv[1]) < 5000 || argv[2] == "")
        {
            cout << "\nERROR: Please select a portnumber thats higher than 4999" << endl;
            cout << "The right way to start this server:\n"
                 << endl;
            cout << "./Server [PORT]\n"
                 << endl;
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    // Variables
    int listeningSocket, clientSocket;
    int opt = 1;
    char buffer[BUF];
    string clientIp;
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
    listeningSocketAddress.sin_family = AF_INET;                 // Set address family (IPv4)
    listeningSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);  // Set any ip address
    listeningSocketAddress.sin_port = htons(std::stoi(argv[1])); // Set port to use (network byte order)

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

    // Welcome message
    string message = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    message += "~~~~~~~~~~Hello to my mail server~~~~~~~~~~\n";
    message += "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

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

        clientIp += inet_ntoa(clientAddress.sin_addr); // Get the clientIp

        strcpy(buffer, message.c_str());               // Fill the buffer with the welcome message
        send(clientSocket, buffer, strlen(buffer), 0); // Send the welcome message to the client

        memset(buffer, 0, sizeof(buffer)); // Reset the buffer

        threads.push_back(std::thread(handleClient, clientSocket, clientIp));

        clientSocket = 0;
        clientIp = "";
    }

    // free(&listeningSocket);
    // free(&clientSocket);
    // free(&buffer);
    // free(&clientIp);
    // free(&message);
    close(listeningSocket);
}