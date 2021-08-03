TAR := main.out
SRC := main.o CConnectPool.o

$(TAR): $(SRC)
	gcc -I mysql -L mysqllib $^ -o $@ -lmysqlclient -lpthread -lstdc++
%.o:%.cpp
	gcc -c $^ -o $@

clean:
	rm -f $(TAR) $(SRC)