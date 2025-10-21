#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

#define SERVER_PORT 9909
#define BROADCAST_PORT 9910

// Function to auto-detect the server IP using UDP broadcast
string detectServerIP() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        cout << "UDP socket creation failed.\n";
        return "";
    }

    sockaddr_in recvAddr{};
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(BROADCAST_PORT);
    recvAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&recvAddr, sizeof(recvAddr)) < 0) {
        cout << "UDP bind failed.\n";
        close(udpSocket);
        return "";
    }

    cout << "Searching for server on LAN...\n";

    char buffer[256];
    sockaddr_in sender{};
    socklen_t senderLen = sizeof(sender);

    // Wait for broadcast message from server
    int bytes = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
                         (sockaddr*)&sender, &senderLen);
    close(udpSocket);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        string msg(buffer);
        size_t pos = msg.find("SERVER_IP:");
        if (pos != string::npos) {
            string ip = msg.substr(pos + 10);
            cout << "Server Found at: " << ip << endl;
            return ip;
        }
    }

    cout << "Server not found. Try again later.\n";
    return "";
}

int main() {
    string serverIP = detectServerIP();
    if (serverIP.empty()) return -1;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << "Socket creation failed.\n";
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    cout << "Connecting to server...\n";

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cout << "Connection failed. Check server status.\n";
        close(clientSocket);
        return -1;
    }

    cout << "Connected to server.\n";

    // Send client name
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

        // Receive message from server
        if (FD_ISSET(clientSocket, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                cout << "\nServer disconnected.\n";
                break;
            }
            cout << "\n" << buffer << "\nYou: ";
            fflush(stdout);
        }

        // Send user message
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            string msg;
            getline(cin, msg);
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }
    }

    close(clientSocket);
    return 0;
}
