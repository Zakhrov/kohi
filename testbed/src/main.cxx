#include <stdio.h>
#include <core/logger.h>
#include <core/asserts.h>
#include <core/application.h>
int main(void){

    ApplicationConfig config;
    config.startX = 100;
    config.startY = 100;
    config.name = (char*)"Kohi Testbed";
    config.startWidth = 1280;
    config.startHeight = 720;

    application_create(&config);
    application_run();
    
    
    return 0;
}

