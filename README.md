# speedtest-cli
[C Version]Command line interface for testing internet bandwidth using speedtest.net

## Introduce
This project depends libcurl-devl and libexpat-devl, and can work on MAC and Linux OS.

This project is a C version of [sivel/speedtest-cli](https://github.com/sivel/speedtest-cli), But they have some differences.
I will expain more about my implement later.


## Test
### Run on my linux VPS:
```shell
[root@host speedtest-cli]# ./speedtest_cli
Retrieving speedtest.net configuration...
Retrieving speedtest.net server list...
Testing from IT7 Networks (65.49.209.201)...
Selecting best server based on ping...
Bestest server: speedtest.lax.gigenet.com(1.30KM)
Server latency is 2ms
Testing download speed...
Download speed: 732.04Mbps
Testing upload speed.........................
Upload speed: 721.83Mbps
```
### Run on my Mac OS:
```shell
~/t/s/speedtest-cli git:master ❯❯❯ ./speedtest_cli
Retrieving speedtest.net configuration...
Retrieving speedtest.net server list...
Testing from China Telecom jiangsu (114.218.xx.xxx)...
Selecting best server based on ping...
Bestest server: hgh.unpergroup.cn(86.86KM)
Server latency is 120ms
Testing download speed........................
Download speed: 27.34Mbps
Testing upload speed.........
Upload speed: 8.85Mbps
```

## Test Step
Speedtest.net operates entirely over HTTP for maximum compatibility. It tests ping (latency), download speed and upload speed.

### Ping
1. Get server list from speedtest.net
2. Select 10 closest servers according to their longitude/latitude and your computer location
3. Sends HTTP requests to the selected server, and measures the time it takes to get a response.

### Download Speed
1. Your computer downloads small binary files from the web server to the client, and we measure that download to estimate the connection speed.
2. Based on this result, we choose how much data to download for the real test. Our goal is to pick the right amount of data that you can download in 10 seconds, ensuring we get enough for an accurate result but not take too long.
3. Once we start downloading, we use up to four HTTP threads to saturate your connection and get an accurate measurement.
4. Calculate speed 5 times per second, and save them into 50 slices
5. The fastest 10% and slowest 20% of the slices are then discarded.
6. The remaining slices are averaged together to determine the final result.

### Upload Test

1. A small amount of random data is generated in the client and sent to the web server to estimate the connection speed.
2. Based on this result, and appropriately sized chunck of randomly generated data is selected for upload.
3. The upload test is then performed in chunks of uniform size, pushed to the server-side script via POST.
4. We'll use up to four HTTP threads here as well to saturate the connection.
5. Chunks are sorted by speed, and The fastest 10% and slowest 20% of the slices are then discarded and determine the result.

