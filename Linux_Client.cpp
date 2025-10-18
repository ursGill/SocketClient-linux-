#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

#define SERVER_IP "192.168.43.25"  // <-- Replace this with your PC's IP
#define PORT 9909

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << "Socket creation failed." << endl;
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    cout << "Connecting to server..." << endl;

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cout << "Connection failed. Check IP or server status." << endl;
        close(clientSocket);
        return -1;
    }

    cout << "Connected to server." << endl;

    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    send(clientSocket, name.c_str(), name.size(), 0);

    fd_set readfds;
    char buffer[512];

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = max(clientSocket, STDIN_FILENO) + 1;
        int activity = select(maxfd, &readfds, NULL, NULL, NULL);

        if (activity < 0) continue;

        // Check if server sent message
        if (FD_ISSET(clientSocket, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                cout << "Server disconnected." << endl;
                break;
            }
            cout << "\n" << buffer << endl;
        }

        // Check if user typed message
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string msg;
            getline(cin, msg);
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    }

    close(clientSocket);
    return 0;
}
