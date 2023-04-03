#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#define I ++*p;
#define D --*p;
#define L --p;
#define R d(c,p,&s);++p;
#define O putchar(*p);
#define G *p=getchar();
#define S while(*p){
#define E }
void d(uint8_t*c,uint8_t*p,size_t*s){if(p-c==*s-1){p-=(uintptr_t)c;c=realloc(c,*s*2);memset(c+*s,0,*s-1);*s*=2;p+=(uintptr_t)c;}++p;}int main(void){size_t s=256;uint8_t*c=calloc(s,sizeof(unsigned char));uint8_t*p=c;R I I I I I I I I S L I I I I I I I I I R D E L O R I I I I S L I I I I I I I R D E L I O I I I I I I I O O I I I O R R I I I I I I S L I I I I I I I R D E L I I O D D D D D D D D D D D D O R I I I I I I S L I I I I I I I I I R D E L I O L O I I I O D D D D D D O D D D D D D D D O R R R I I I I S L I I I I I I I I R D E L I O R I I I I I I I I I I O free(c);}
