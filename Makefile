speedtest_cli: main.c
	gcc $< -lcurl -o $@

clean:
	rm speedtest_cli *.o
