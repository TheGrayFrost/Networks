a.out : tcpclient.c v.out
	gcc tcpclient.c
v.out :	md5/md5.h md5/md5.cpp tcpserver.c
	g++ tcpserver.c md5/md5.cpp -o v.out
	
clear : a.out v.out
	rm *.out md5_value.txt
