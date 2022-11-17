rm client
rm server
gcc -pthread client.c -o client 
gcc -pthread server.c -o server
