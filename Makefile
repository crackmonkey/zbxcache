all: zbxcache

clean:
	rm -f zbxcache

zbxcache: zbxcache.c
	gcc -g -o zbxcache zbxcache.c -lmemcached

test: zbxcache
	./zbxcache ./testscript 30 test.now
	./zbxcache ./testscript 30 test.hi
	./zbxcache ./testscript 30 test.fail


