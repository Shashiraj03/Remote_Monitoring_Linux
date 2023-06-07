# Remote_Monitoring_Linux

Discription of the project:
Sensor data (temperatures, pressures and voltages) related to a typical process is collected at an edge device. 

This data is communicated to the cloud using MQTT protocol. A monitoring application accesses this datacthrough subscription to the relevant topics and processes this data. 
Processing is performed in a multi-threaded fashion, one per each sensor. Processing involves computing averages of related sensor data and detection of out of range values. 
The averages and out of range values are logged in to log files for archival purposes.

Sensor data needed to be generated in a separate module, using random number generation, as the hardware was not available.
Development was done using C++, Visual studio code and Linux. Pthread library was used for multi-threaded operation. 
Extensive synchronisation (mutexes, semaphores) was used to support concurrent processing by the multiple threads. 
Socket programming was used for communication.

for running program just download zip file and type make

1. In terminal first need to export the library file for MQTT protocal library
2. command "exprot LD_LIBRARY_PATH=/mqtt lib file path:$LIBRARY_PATH.
3. Then type make and run. 
