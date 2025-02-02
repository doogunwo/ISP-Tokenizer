#include "symbol.h"

Symbol::Symbol(uint32_t character, int prevIndex, int nextIndex, std::size_t length)
    : c(character), prev(prevIndex), next(nextIndex), len(length) {}

void Symbol::merge_with(const Symbol& other, uint32_t new_c){
  this->c = new_c;
  this->len = this->len + other.len;
  this->next= other.next;
}

