#include "util.h"
#include <cctype>
#include <cstring>
#include <iostream>

using namespace std;

int clampInt(int val, int minVal, int maxVal){
    
    if(val<minVal){
    return minVal; }

    if(val>maxVal){
    return maxVal; }

    return val;  }


void trimWhitespace(char *str){
    
    if(str == nullptr){
    return;  }

    int start=0;
    while(str[start] != '\0' && isspace(static_cast<unsigned char>(str[start]))){
    start++; }

    int length=static_cast<int>(strlen(str+start));

    for(int i=0; i<=length; i++){
    str[i]=str[start+i]; }

    while(length>0 && isspace(static_cast<unsigned char>(str[length-1]))){
    length--;
    str[length]='\0'; }  

}


bool stringEquals(const char *a, const char *b){
    
    if(a==nullptr || b==nullptr){
    return false; }

    return strcmp(a,b) == 0;

}


void printHeader(const char *title ){
    if(title==nullptr){
    return;}

    cout<<"\n=== "<<title<<" ===\n"; 

}

void clearScreen(){
    cout<<"\n";

}

