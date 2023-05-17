all: data_gen server client data_exe server_exe client_exe

server:server_device.cpp
	g++ server_device.cpp -o server -pthread -lpaho-mqtt3c

client:client_device.cpp
	g++ client_device.cpp -o client

data_gen:CSV_file.cpp
	g++ CSV_file.cpp -o report_card

data_exe:report_card
	./report_card

server_exe:server
	gnome-terminal -- bash -c './server; exec bash'

client_exe:client
	gnome-terminal -- bash -c './client; exec bash'


