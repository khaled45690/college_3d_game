\
g++ -std=c++17 -I./include -I./include/glm src/main.cpp src/Model.cpp -o bin/pepsi-man $(pkg-config --cflags --libs gl glu glut) -v 2>&1 | tail -20