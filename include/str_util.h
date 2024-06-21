#ifndef NES__STR_UTIL_H
#define NES__STR_UTIL_H

#include <cstdlib>
#include <string_view>

namespace nes {

  // Vem removendo o caractere da direita para esquerda da string
  std::string_view rtrim(std::string_view, char);
  // Vem removendo o caractere da esquerda para direita da string
  std::string_view ltrim(std::string_view, char);
  // Vem removendo o caractere das duas extremidades
  std::string_view trim(std::string_view, char = ' ');

  // Substitui todas as ocorrências da substring na string
  std::string replace_all(std::string_view, std::string_view, std::string_view);

  // Conversão de strings
  template <class T>
    requires (!std::is_enum_v<T>)
  T str_para(std::string_view);

  template <class E>
    requires std::is_enum_v<E>
  E str_para(std::string_view str)
  {
    return static_cast<E>(std::atoi(str.data()));
  }
}

#endif
// NES__STR_UTIL_H