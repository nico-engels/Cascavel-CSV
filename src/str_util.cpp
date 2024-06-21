#include "str_util.h"

#include <algorithm>
#include <chrono>
#include <ranges>
#include <spanstream>
#include <string>
#include "runtime_exc.h"
using namespace std;
using namespace std::chrono;
namespace rng = std::ranges;

namespace nes {

  string_view rtrim(string_view str, char ch)
  {
    // Localiza e remove o início com o caractere
    auto pos = str.find_last_not_of(ch);
    if (pos != string_view::npos)
      return str.substr(0, pos + 1);
    return string_view { };
  }

  string_view ltrim(string_view str, char ch)
  {
    // Percorre a string encontrando o caractere a ser removido
    auto pos = str.find_first_not_of(ch);
    if (pos != string_view::npos)
      return str.substr(pos);
    return string_view { };
  }

  string_view trim(string_view str, char ch /*= ' '*/)
  {
    // Chama as funções para as duas extremidades
    return ltrim(rtrim(str, ch), ch);
  }

  string replace_all(string_view str, string_view sub_str, string_view replace_str)
  {
    if (sub_str.empty())
      return string { str };

    string ret;
    ret.reserve(str.size());

    auto it_b = str.begin();
    auto it_e = str.end();
    while ((it_e = rng::search(rng::subrange { it_b, str.end() }, sub_str).begin()) != str.end())
    {
      ret.insert(ret.end(), it_b, it_e);
      ret.insert(ret.end(), replace_str.begin(), replace_str.end());
      it_b = it_e + sub_str.size();
    }
    ret.insert(ret.end(), it_b, str.end());

    return ret;
  }

  template <>
  string str_para<string>(string_view str)
  {
    return string { str };
  }

  template <>
  unsigned str_para<unsigned>(string_view str)
  {
    return static_cast<unsigned>(stoul(string { str }));
  }

  template <>
  int str_para<int>(string_view str)
  {
    return stoi(string { str });
  }

  template <>
  unsigned long str_para<unsigned long>(string_view str)
  {
    return stoul(string { str });
  }

  template <>
  unsigned long long str_para<unsigned long long>(string_view str)
  {
    return stoul(string { str });
  }

  template <>
  float str_para<float>(string_view str)
  {
    return stof(string { str });
  }

  template <>
  year_month_day str_para<year_month_day>(string_view str)
  {
    int ano;
    unsigned int mes, dia;
    char sep1, sep2;
    ispanstream is(str);

    if (is >> ano >> sep1 >> mes >> sep2 >> dia && sep1 == '-' && sep2 == '-')
      return year_month_day { year { ano }, month { mes }, day { dia } };
    else
      throw runtime_exc { "Data inválida!" };
  }

  template <>
  year_month str_para<year_month>(string_view str)
  {
    int ano;
    unsigned int mes;
    char sep;
    ispanstream is(str);

    if (is >> ano >> sep >> mes && sep == '-')
      return year_month { year { ano }, month { mes } };
    else
      throw runtime_exc { "Data inválida!" };
  }

}