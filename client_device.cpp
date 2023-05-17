#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 1024

using namespace std;

class client_devide
{
    public:
        int sockfd;
        struct sockaddr_in server_addr; 

            void socket_creation()
            {
                sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if (sockfd < 0) 
                {
                    cerr << "Error creating socket." << endl;
                    exit(EXIT_FAILURE);
                }

                // Creating a structure which contains the information about the server
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(PORT);
                server_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr("10.1.138.6");
            }


            void send_data()
            {
                ifstream input_file("reportcard.csv"); //open the input file
                if (!input_file.is_open()) 
                {    
                    cout << "Failed to open input file." << endl;
                    exit(EXIT_FAILURE);
                }

                int record_count = 0;
                string line;

                // Reading a line by line data from the input file and storing into a string variable
                while (getline(input_file, line)) 
                {
                    char buffer[BUFFER_SIZE] = {0};
                    
                    // randomly drop some records
                    float drop = (float)(rand() % 100) /100 ;
                    // cout<<random_num<<" ";
                    if (drop < 0.05) 
                    {
                        cout << "Dropping record: " << line << endl;
                        continue;
                    }

                    float duplicate = (float)(rand() % 100) /100 ;  // randomly duplicate some records
                    if (duplicate > 0.05 && duplicate < 0.1) 
                    {
                        cout << "Duplicating : " << line << endl;
                        strcpy(buffer, line.c_str());
                        sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
                    }
                    
                    strcpy(buffer, line.c_str());
                    sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

                    record_count++;

                    // cout << "Sent " << record_count << " records to server." << endl;
                    cout<<"sent :"<<buffer<<"\n";
                    // this_thread::sleep_for(chrono::seconds(1));
                    sleep(0.82);
                }

                input_file.close();
                // cout << "Sent " << record_count << " records to server." << endl;
                close(sockfd);
            }
};

int main() 
{
    client_devide client_dev;
    client_dev.socket_creation();
    client_dev.send_data();
    return 0;
}
