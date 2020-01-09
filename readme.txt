Please read the paper in this folder in order to get a better understanding of the project.

Understanding:
paho_c_sub -t my_topic123 --connection localhost:1883
paho_c_sub -t currentLogTopic --connection localhost:1883

To Install:
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
make
sudo make install


To RUN:
g++ -L /home/Desktop/Temp/paho.mqtt.c/build/output -c try.c -l paho-mqtt3c -lpthread
g++ -L /home/Desktop/Temp/paho.mqtt.c/build/output -o try try.o -l paho-mqtt3c -lpthread
./try


//Sample Requests
m-1-0-1-open-f1.txt-read:
m-1-0-2-read-f1.txt-5:
m-1-0-3-write-f1.txt-"01234":
m-1-0-4-lseek-f1.txt-15:
m-1-0-5-read-f1.txt-5:
m-1-0-6-close-f1.txt:
m-1-1-7-open-f2.txt-write:
m-1-1-8-lseek-f2.txt-10:
m-1-1-9-write-f2.txt-"01234":
m-1-1-10-close-f2.txt:


Files:
1. The files f0.txt, f1.txt, f2.txt.... are files on the server that the clients would like to operate on.
2. The files script1.txt, script2.txt.... have commands that the clients run on the server.
3. The lock table is on the server that has a list of all the files and who are operating on them.
4. The client table has a list of commands that the leader server runs and sends to the backup server.