a.out : udpclient.c v.out
	g++ udpclient.c
v.out :	md5/md5.h md5/md5.cpp udpserver.c
	g++ udpserver.c md5/md5.cpp -o v.out
	
clear : a.out v.out
	rm *.out md5_value.txt
