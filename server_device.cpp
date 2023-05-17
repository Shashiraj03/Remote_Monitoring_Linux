#include <bits/stdc++.h>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include "MQTTClient.h"


#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "Avgpuplish"
#define TOPIC       "Averages"
#define QOS         1
#define TIMEOUT     10000L //type casting to long int(l)

using namespace std;


#define PORT 8000
#define MAX_MSG_SIZE 1024
#define MAX_RECORDS 10006

// data structure for storing records
struct Record 
{
    long timestamp;
    double speed1;
    double speed2;
    double speed3;
    double pressure1;
    double pressure2;
    double voltage;
};

//data structure for shared memory
struct Shared_memory
{
    float speed_avg = 0;
    float pressure_avg = 0;
    // double voltage_avg = 0;
    int n1 = 0;
    int n2 = 0;
};

// global variables
Record records[MAX_RECORDS];
Shared_memory Shared_memory_t[MAX_RECORDS];

int num_records = 0;
float avg_speed = 0.0;
float avg_pressure = 0.0;
int number_pressure_outof_range = 0;
int number_speed_outof_range = 0;
int number_voltage_outof_range = 0;

// Mutex Initialization
pthread_mutex_t shared_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t log_semaphore;


// Globle veriables for MQTT Protocal
MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;
int rc;

// function to create server socket
int create_server_socket() 
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        cerr << "Failed to create socket\n";
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
   
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr("10.1.138.6");
    server_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
    {
        cerr << "Failed to bind socket\n";
        exit(EXIT_FAILURE);
    }
    return sockfd;
}


// function for MQTT Connection
void MQTT_connect() 
{
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }   
}


// function for MQTT Message publication
void MQTT_PublishMessage(string msg)
{
    char playLoad[1024]={0};
    // memset(&pubmsg,0,sizeof(pubmsg));
    strcpy(playLoad,msg.c_str());
    playLoad[msg.length()] = '\0';
    pubmsg.payload = playLoad;
    pubmsg.payloadlen = (int)strlen(playLoad);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    // cout<<(char*)pubmsg.payload<<endl;

    if ((rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to publish message, return code %d\n", rc);
    }

    //waiting for msg is sending or not 
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    // printf("Message with delivery token %d delivered\n", token);

}


// function for MQTT Subscriber in main thread
void MQTT_Subscriber()
{
    const char* topic ="Averages"; // Topic name for subscription
    MQTTClient_message* message;
    int topic_len = strlen(topic);

    if(MQTTClient_subscribe(client,topic,1) != 0)
    {
        perror("Eroor while subscribing");
    }
    while(1)
    {
        if(MQTTClient_receive(client,(char **)&topic,&topic_len,&message,TIMEOUT) != 0)
        {
            perror("Error while receiving msg");
        }
        if( message  == NULL)
        {
            cout<<"NULL\n";
            break;
        }
        ((char *)message->payload)[message->payloadlen] = '\0';
        cout<<(char*)message->payload<<"\n"; 
        MQTTClient_freeMessage(&message);
    }
}


// function to compute speed average
void compute_speed_average(int i, double speed) 
{
    pthread_mutex_lock(&shared_mutex);
    
    if(Shared_memory_t[i].n1 == 0)
    {
        Shared_memory_t[i].speed_avg = speed;
    }
    if(Shared_memory_t[i].n1 == 1)
    {
        Shared_memory_t[i].speed_avg = (speed + Shared_memory_t[i].speed_avg) / 2;
    }
    if(Shared_memory_t[i].n1 == 2)
    {
        Shared_memory_t[i].speed_avg = ((2 * Shared_memory_t[i].speed_avg) + speed) / 3;

        if(Shared_memory_t[i].n2 == 2)
        {
            string msg = "Timestamp :"+ to_string(records[i].timestamp);
            msg += " , Speed_Avg :"+ to_string(Shared_memory_t[i].speed_avg);
            msg += " , Pressure_Avg :" + to_string(Shared_memory_t[i].pressure_avg);   
            MQTT_PublishMessage(msg);
        }
    }
    
    Shared_memory_t[i].n1++;
    pthread_mutex_unlock(&shared_mutex);
}


//function to compute pressure average
void compute_pressure_average(int i,double pressure)
{
    pthread_mutex_lock(&shared_mutex);
    if(Shared_memory_t[i].n2 == 0)
    {
        Shared_memory_t[i].pressure_avg = pressure;
    }
    if(Shared_memory_t[i].n2 == 1)
    {
        Shared_memory_t[i].pressure_avg = (pressure + Shared_memory_t[i].pressure_avg) / 2;

         if(Shared_memory_t[i].n1 == 3)
        {
            string msg = "Timestamp :"+ to_string(records[i].timestamp);
            msg += " , Speed_Avg :"+ to_string(Shared_memory_t[i].speed_avg);
            msg += " , Pressure_Avg :" +to_string(Shared_memory_t[i].pressure_avg);
             
            MQTT_PublishMessage(msg);
        }
    }
    Shared_memory_t[i].n2++;
    pthread_mutex_unlock(&shared_mutex);
}


// function to add a record to the global array
void add_record(Record record) 
{
    records[num_records] = record;

    cout << num_records <<" Recived\n";
    num_records++;
    // sleep(0.03);
}


//for displaying globle array data
void display()
{
    for(int i=0;i<num_records;i++)
    {
        // cout<<records[i].timestamp<<" ,"<<records[i].speed1<<" ,"<<records[i].speed2<<" ,"<<records[i].speed3;
        // cout<<" ,"<<records[i].pressure1<<" ,"<<records[i].pressure2<<" ,"<<records[i].voltage<<"\n";

        // Shared_memory_t[i].pressure_avg = records[i].pressure1;
        // Shared_memory_t[i].speed_avg = records[i].speed1;

        cout<<records[i].timestamp<<" ,"<<Shared_memory_t[i].speed_avg<<" ,"<<Shared_memory_t[i].pressure_avg<<endl; 
    }
}


// function to split a string into tokens based on a delimiter
vector<string> split(string str, string delimiter) 
{
    vector <string> tokens;

    // Initialize a variable to keep track of the starting position of the next substring
    size_t pos = 0;
    while ((pos = str.find(delimiter)) != std::string::npos) 
    {
        string token = str.substr(0, pos);
        tokens.push_back(token);
        str.erase(0, pos + delimiter.length());
    }
    tokens.push_back(str);
    return tokens;
}


// Pthread for seed1 monitoring
void *speed1_thread(void *arg)
{
    for(int i=0; i<num_records; i++)
    {
        if(!(records[i].speed1 > 1850 && records[i].speed1 < 2350))
        {
            sem_wait(&log_semaphore);

            // creating speed_out_of_range file
            FILE* log_file = fopen("out_of_range.log", "a");
            fprintf(log_file, "Speed 1 out of range: timestamp=%ld, speed1=%.0f\n", records[i].timestamp, records[i].speed1);
            number_speed_outof_range++;
            fclose(log_file);
            sem_post(&log_semaphore);
        }
        // pthread_mutex_lock(&shared_mutex);
        compute_speed_average(i,records[i].speed1);
    }
    pthread_exit(NULL);
}


// Pthread for seed2 monitoring
void *speed2_thread(void *arg)
{
    for(int i=0; i<num_records; i++)
    {
        if(!(records[i].speed2 > 1850 && records[i].speed2 < 2350))
        {
            sem_wait(&log_semaphore);

            // creating speed_out_of_range file
            FILE* log_file = fopen("out_of_range.log", "a");
            fprintf(log_file, "Speed 2 out of range: timestamp=%ld, speed2=%.0f\n", records[i].timestamp, records[i].speed2);
            number_speed_outof_range++;
            fclose(log_file);

            sem_post(&log_semaphore);
        }
        compute_speed_average(i,records[i].speed2);
    }
    pthread_exit(NULL);    
}


// Pthread for seed2 monitoring
void *speed3_thread(void *arg)
{
    // pthread_mutex_lock(&shared_mutex);
    for(int i=0; i<num_records; i++)
    {
        if(!(records[i].speed3 > 1850 && records[i].speed3 < 2350))
        {
            sem_wait(&log_semaphore);

            // creating speed_out_of_range file
            FILE* log_file = fopen("out_of_range.log", "a");
            fprintf(log_file, "Speed 3 out of range: timestamp=%ld, speed3=%.0f\n", records[i].timestamp, records[i].speed3);
            number_speed_outof_range++;
            fclose(log_file);

            sem_post(&log_semaphore);
        }
        compute_speed_average(i,records[i].speed3);
    }
    pthread_exit(NULL);
}


// Pthread for pressure1 monitoring
void *pressure1_thread(void *arg)
{
    for(int i=0; i < num_records; i++)
    {
        if(!(records[i].pressure1 > 0.055 && records[i].pressure1 < 0.095 ))
        {
            sem_wait(&log_semaphore);
            FILE *log_file = fopen("out_of_range.log","a");
            fprintf(log_file,"Pressure 1 out of range: timestamp=%ld pressure1= %.3f\n", records[i].timestamp, records[i].pressure1);
            number_pressure_outof_range++;
            fclose(log_file);
            sem_post(&log_semaphore);
        }

        compute_pressure_average(i,records[i].pressure1);
    }
    pthread_exit(NULL); 
}


// Pthread for pressure2 monitoring
void *pressure2_thread(void *arg)
{
    for(int i=0; i < num_records; i++)
    {
        if(!(records[i].pressure2 > 0.055 && records[i].pressure2 < 0.095 ))
        {
            sem_wait(&log_semaphore);
            FILE *log_file = fopen("out_of_range.log","a");
            fprintf(log_file,"Pressure out of range: timestamp=%ld pressure2= %.3f\n", records[i].timestamp, records[i].pressure2);
            number_pressure_outof_range++;
            fclose(log_file);
            sem_post(&log_semaphore);
        }

        compute_pressure_average(i,records[i].pressure2);
    }
    pthread_exit(NULL);
}


// Pthread for voltage monitoring
void *voltage_thread(void *arg)
{
    for(int i=0; i < num_records; i++)
    {
        if(!(records[i].voltage > 13 && records[i].voltage < 23 ))
        {
            sem_wait(&log_semaphore);
            FILE *log_file = fopen("out_of_range.log","a");
            fprintf(log_file,"Voltage out of range: timestamp=%ld voltage= %.0f\n", records[i].timestamp, records[i].voltage);
            number_voltage_outof_range++;
            fclose(log_file);
            sem_post(&log_semaphore);
        }
    }
    pthread_exit(NULL);
}


// Final Averages logfile
void final_avg_logfile()
{
    FILE* log_file = fopen("final_avg_logfile.log", "a");

    for(int i = 0;i < num_records; i++)
    {
        fprintf(log_file,"Timestamp: %ld, Average of speed: %.3f, Average of pressure: %.3f\n", records[i].timestamp,Shared_memory_t[i].speed_avg,Shared_memory_t[i].pressure_avg);
    }

    fclose(log_file);
}


int main()
{ 
    MQTT_connect(); 
    int sockfd = create_server_socket();
    // create log semaphore
    sem_init(&log_semaphore, 0, 1);

    
    //creating shared memory segment
    int shmid_t = shmget(1235, MAX_RECORDS *(sizeof(Shared_memory)), IPC_CREAT  | 0777);
    if( shmid_t < 0 )
    {
        cout<<"Failed to create sharead memory\n";
    }
    Shared_memory *shmat_t =(Shared_memory*) shmat(shmid_t, NULL, 0); //attaching sharead memory segment to current process
    if(shmat_t == (Shared_memory*) -1)
    {
        perror("shmat_t");
    }

    memset(shmat_t,0,num_records*(sizeof(Shared_memory)));

    // listen for messages from client
    size_t num_bytes,count=0;
    while (1) 
    {
        if(num_records == 10000)
        {
            break;
        }
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        char buffer[1024];
        // sleep(0.);
        num_bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, &client_addr_len);
        // cout<<num_bytes<<"\n";
        
        if (num_bytes == -1) 
        {
            perror("recvfrom");
            // sleep(1);
            continue;
        }
        // cout<<buffer<<" \n";
        // parse message
        buffer[num_bytes] = '\0';
        string msg(buffer);

        // cout<<"mssss = "<<msg<<endl;

        // split function for spliting message
        vector <string> tokens = split(msg, ","); 
        

        // create new record
        Record record;
        record.timestamp = std::stol(tokens[0]);
        record.speed1 = std::stod(tokens[1]);
        record.speed2 = std::stod(tokens[2]);
        record.speed3 = std::stod(tokens[3]);
        record.pressure1 = std::stod(tokens[4]);
        record.pressure2 = std::stod(tokens[5]);
        record.voltage = std::stod(tokens[6]);

        // drop duplicates
        if (num_records > 0 && record.timestamp == records[num_records-1].timestamp) 
        {
            continue;
        }

        // add missing records
        if (num_records > 0 && record.timestamp > records[num_records-1].timestamp + 1)
        {
            for (long t = records[num_records-1].timestamp + 1; t < record.timestamp; t++) 
            {
                Record missing_record = records[num_records-1];
                missing_record.timestamp = t;
                add_record(missing_record);
            }
        }    
        // add new record
        add_record(record);
    }


    // create threads for speed,pressure,voltage
    pthread_t speed1,speed2,speed3,pressure1,pressure2,voltage;
    pthread_create(&speed1,NULL, speed1_thread, (void*) "speed1");
    
    pthread_create(&speed2,NULL, speed2_thread, (void*) "speed2");
    pthread_create(&speed3,NULL, speed3_thread, (void*) "speed3");
    pthread_create(&pressure1,NULL, pressure1_thread, (void*) "pressure1");
    pthread_create(&pressure2,NULL, pressure2_thread, (void*) "pressure2");
    pthread_create(&voltage,NULL, voltage_thread, (void*) "voltage");


    MQTT_Subscriber(); //MQTT subscriber function

    pthread_join(speed1, NULL);
    pthread_join(speed2, NULL);
    pthread_join(speed3, NULL);
    pthread_join(pressure1, NULL);
    pthread_join(pressure2, NULL);
    pthread_join(voltage, NULL);

    // display();


    cout<<"Number of speed outof range : "<<number_speed_outof_range<<endl;
    cout<<"Number of pressure outof range : "<<number_pressure_outof_range<<endl;
    cout<<"Number of voltage outof range : "<<number_voltage_outof_range<<endl;

    // For Disconnecting MQTTClient connection
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
        printf("Failed to disconnect, return code %d\n", rc);


    final_avg_logfile();

    //Shared memory detachment
    shmdt(shmat_t);

    // To delete the shared memory segment
    if (shmctl(shmid_t, IPC_RMID, NULL) == -1) 
    {
        printf("Error deleting shared memory segment\n");
        return 1;
    }

    // To destroy pthread mutex
    if (pthread_mutex_destroy(&shared_mutex) != 0)
    {
        printf("Error destroying mutex\n");
        return 1;
    }
    
    MQTTClient_destroy(&client); //Destroying the MQTTClient 

    sem_destroy(&log_semaphore);  //semaphore destroy
    close(sockfd);
    return 0;
}
  