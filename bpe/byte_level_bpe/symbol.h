#ifndef SYMBOL_H
#define SYMBOL_H

#include <cstdint>

class Symbol {
public:
  uint32_t c;
  int prev;
  int next;
  std::size_t len;

  Symbol(uint32_t character, int prevIndex, int nextIndex, std::size_t length);
  void merge_with(const Symbol& other, uint32_t new_c);
};

#endif 
