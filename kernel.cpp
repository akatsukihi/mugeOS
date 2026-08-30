
void printf(char* s){
    unsigned short* screenOut = (unsigned short*)0xb8000;
    for(int i=0; s[i] !='\0'; i++){
        screenOut[i] = (screenOut[i] & 0xFF00) | s[i];
    }
}

extern "C" void mgKernelMain(void * multiboot_structure , unsigned int magicnumber){
    printf("hello muge, Please complete your study of operating systems.\n");
    while(1);
}