#include <iostream>
#include <fstream>
#include <cstdlib>


int main(int argc, char* argv[]){

    if(argc != 2){
        std::cerr << "incorrect amount of argc" << std::endl;
        exit(1);
    }

    std::ifstream file(argv[1]); 
    std::cout << "lalala" << std::endl;



    return 0;
}