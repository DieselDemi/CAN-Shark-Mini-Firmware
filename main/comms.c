#include "comms.h"
#include <stdio.h> 

void send_data(int data) { 
    printf("%i", data); 
}

settings_t get_settings() { 
    char* buffer = malloc(sizeof(char) * 1024); 
    scanf("%s", buffer); 
    
    buffer = NULL; 

    settings_t *settings = malloc(sizeof(settings_t)); 
    
    settings->filter = 0; 
    settings->speed  = 0; 

    return *settings;
}