speedtest_cli: main.c
	gcc $< -lpthread -lcurl -lexpat -lm -o $@

clean:
	rm speedtest_cli
