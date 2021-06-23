# Boot two domains
1. Prepare two images (linux kernel and a helloworld) for post-sbi stage;
Generate a helloworld
```bash
make
```
2. Prepare a device tree image
```bash
cd dtf
make sifive_u.dtb
cd ..
```

3. Run
```bash
./run.sh
```

4. Confirm both domain running.
You should see kernel booted in stdio and then check another domain through telnet client
```bash
telnet localhost 4321
```
Then you should see
```
Trying 127.0.0.1...                                                                                                     
Connected to localhost.                                                                                                 
Escape character is '^]'.                                                                                               
Hello World !               
Hello World !  
Hello World !  
...
```

