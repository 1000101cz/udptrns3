all:
	gcc src/client/client.c src/client/main_func.c src/sha256/sha256.c src/crc32/crc32.c src/libs/confirmations.c src/libs/file_operations.c -o client
	gcc src/server/server.c src/server/main_func.c src/sha256/sha256.c src/crc32/crc32.c src/libs/confirmations.c src/libs/file_operations.c -o server
clean:
	rm server
	rm client
