#include <stdio.h>
#include <string.h>

void split_into_bytes(const char *input){
  const unsigned char *ptr = (const unsigned char *)input;
  int index = 0;

  while(*ptr){
    printf("Byte %d: `%c` (0x%02X)\n", index, *ptr, (*ptr >= 32 && *ptr <= 126) ? *ptr : '.');
    ptr++;
    index++;
  }
}

int main(){
  const char *text = "한국어입니다.";
  split_into_bytes(text);
  return 0;
}
