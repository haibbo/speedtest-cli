speedtest_cli: main.c
	gcc $< -lcurl -lexpat -lm -o $@

clean:
	rm speedtest_cli
