#include <winsock2.h>
#include <winuser.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>

//#pragma comment(lib, "ws2_32.lib")

class HttpServer {
private:
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    std::mutex clientMutex;
		std::string machine, ip, user;
    
    // Инициализация WinSock
    bool initWinSock() {
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        return result == 0;
    }

    // Настройка серверного сокета
    bool setupServerSocket(int port) {
        // Создание сокета для прослушивания
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Ошибка создания сокета: " << WSAGetLastError() << std::endl;
            return false;
        }
        
        // Настройка адреса сервера
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        
        // Связывание сокета с адресом
        if (bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Ошибка bind: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            return false;
        }
        
        // Начало прослушивания
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Ошибка listen: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            return false;
        }
        return true;
    }
    
    // Обработка клиентского подключения
    void handleClient() {
        std::vector<char> buffer(4096);
        std::string request;
        
        // Получение данных от клиента
        int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Ошибка при получении данных" << std::endl;
            return;
        }
        
        request.assign(buffer.data(), bytesReceived);
        LASTINPUTINFO last_input;
				last_input.cbSize = sizeof(LASTINPUTINFO);
				
				bool ok = GetLastInputInfo(&last_input);
				DWORD currentTime = GetTickCount();
				DWORD duration = (currentTime - last_input.dwTime) / 1000;

        // Проверка запроса на /ping
        if (request.find("/ping") != std::string::npos && ok && duration < 10) {
            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 38\r\n\r\npong" + machine + " " + ip + "" + user;
            send(clientSocket, response.c_str(), response.length(), 0);
        //} else if () {
					
				} else {
            std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        
        closesocket(clientSocket);
    }
    
    // Основной цикл обработки подключений
    void startListening() {
        std::cout << "Сервер запущен. Ожидание подключений..." << std::endl;
        
        while (true) {
            clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Ошибка accept: " << WSAGetLastError() << std::endl;
                continue;
            }
            
            // Создание нового потока для обработки клиента
            std::thread clientThread(&HttpServer::handleClient, this);
            clientThread.detach();
        }
    }
		
		bool sendMessage(std::string &m) {
			WSADATA ws;
			int iResult = WSAStartup(MAKEWORD(2, 2), &ws);
			if (iResult != NO_ERROR) {
				std::cerr << "WSAStartup function failed with error: " << iResult << std::endl;
        return false;
			}
			
			SOCKET ConnectSocket;
			ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (ConnectSocket == INVALID_SOCKET) {
				std::cerr << "socket function failed with error: " << WSAGetLastError() << std::endl;
				WSACleanup();
				return false;
			}
			sockaddr_in clientService;
			clientService.sin_family = AF_INET;
			clientService.sin_addr.s_addr = inet_addr("10.10.10.72");
			clientService.sin_port = htons(8888);
			
			iResult = connect(ConnectSocket, (SOCKADDR *) & clientService, sizeof (clientService));
			if (iResult == SOCKET_ERROR) {
				std::cerr << "connect function failed with error: %ld\n" << WSAGetLastError() << std::endl;
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
					std::cerr << "closesocket function failed with error: %ld\n" << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
			}
			std::string request("POST /adduser HTTP/1.1\r\n");
      request += "Host: 10.10.10.72\r\n";
      request += "Content-Type: application/json\r\n";
      request += "Content-Length: " + std::to_string(m.size()) + "\r\n";
      request += "Connection: close\r\n\r\n";
      request += m;
			int bsent = send(ConnectSocket, request.c_str(), request.size(), 0);
			iResult = closesocket(ConnectSocket);
			if (iResult == SOCKET_ERROR) {
				std::cout << "closesocket function failed with error: %ld\n" << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
			}
			WSACleanup();
			return true;
		}
public:
    HttpServer(int port) {
        if (!initWinSock()) {
            throw std::runtime_error("Ошибка инициализации WinSock");
        }
        
        if (!setupServerSocket(port)) {
            WSACleanup();
            throw std::runtime_error("Ошибка настройки серверного сокета");
        }
				
				char buffer[1024];
				//get machine name
				gethostname(buffer, sizeof(buffer));
				machine = std::string(buffer);
				
				//get ip
				hostent *host = gethostbyname(buffer);
				ip = std::string(inet_ntoa(*(struct in_addr *)*host->h_addr_list));
				// get user
				DWORD username_len = 1024;
				GetUserName(buffer, &username_len);
				user = std::string(buffer);
				std::string m = "{\"domain\":\"test\",\"mashine\":\"" + machine + "\",\"ip\":\"" + ip + "\",\"user\":\"" + user + "\"}";

				if (!sendMessage(m)) {
					return;
				}
        startListening();
				
    }
    
    ~HttpServer() {
        if (listenSocket != INVALID_SOCKET) {
            closesocket(listenSocket);
        }
        WSACleanup();
    }
};

int main() {
    try {
        HttpServer server(7777);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}