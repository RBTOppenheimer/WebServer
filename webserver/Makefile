COMP = gcc
ARG = -v
OBJ = ./objects
SRC = ./src
INC = ./include/

webserver: $(OBJ)/webserver.o
	$(COMP) $(ARG) -o webserver $(OBJ)/webserver.o
$(OBJ)/webserver.o: $(SRC)/webserver.c
	$(COMP) $(ARG) -c $(SRC)/webserver.c -o $(OBJ)/webserver.o
