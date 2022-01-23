#
#makefile for cjson test
#
#
target: test mytest

cJSON.o: cJSON.c cJSON.h
	cc -c cJSON.c

test: test.c cJSON.o
	cc -c test.c
	cc -o test test.o cJSON.o -lm

mytest: mytest.c cJSON.o
	cc -c mytest.c
	cc -o mytest mytest.o cJSON.o -lm
