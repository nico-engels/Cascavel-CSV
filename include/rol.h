#ifndef NES_BD__ROL_H
#define NES_BD__ROL_H

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
#include "runtime_exc.h"
#include "str_util.h"

namespace nes::bd {

  enum class rol_col_t { unknown, str, int_value, float_value, currency };

  class rol_col_data final
  {
    std::string m_name;
    rol_col_t m_type { rol_col_t::unknown };

  public:
    rol_col_data() = default;
    rol_col_data(const char*);
    rol_col_data(std::string);
    rol_col_data(const char*, rol_col_t);
    rol_col_data(std::string, rol_col_t);

    // Gets/Sets
    void name(std::string);
    const std::string& name() const;

    void type(rol_col_t);
    rol_col_t type() const;

    // Comparações
    auto operator<=>(const rol_col_data&) const = default;
  };

  class rol_data final
  {
    std::variant<std::monostate, std::string, long long int, double> m_data;

  public:
    // Construtores
    rol_data() = default;
    rol_data(std::nullopt_t);
    rol_data(char);
    rol_data(const char*);
    rol_data(std::string);
    rol_data(int);
    rol_data(std::size_t);
    rol_data(float);
    rol_data(std::optional<char>);
    rol_data(std::optional<std::string>);
    rol_data(std::optional<int>);
    rol_data(std::optional<std::size_t>);
    rol_data(std::optional<float>);

    template <class E>
      requires std::is_enum_v<E>
    rol_data(std::optional<E> oe)
    {
      if (oe)
      {
        if constexpr (std::is_same_v<std::underlying_type_t<E>, char>)
          m_data = std::string { static_cast<char>(*oe) };
        else
          m_data = static_cast<long long int>(*oe);
      }
    }

    // Gets/Sets
    void set_nullopt();
    bool is_nullopt() const;
    bool empty() const;

    bool is_str() const;
    void str(std::string);
    const std::string& str() const;
    std::string str_or(std::string_view);

    bool is_int_value() const;
    void int_value(int);
    int int_value() const;
    int int_value_or(int) const;

    bool is_size_t_value() const;
    void size_t_value(std::size_t);
    std::size_t size_t_value() const;
    std::size_t size_t_value_or(std::size_t) const;

    bool is_float_value() const;
    void float_value(float);
    float float_value() const;
    float float_value_or(float);

    // Conversões
    operator std::optional<std::string>() const;
    operator std::optional<std::size_t>() const;
    operator std::optional<float>() const;

    template <class E>
      requires std::is_enum_v<E>
    std::optional<E> to_oenum() const
    {
      struct visit_rol_data_t {
        std::optional<E> operator()(std::monostate) const       { return std::nullopt;                   };
        std::optional<E> operator()(const std::string& s) const { return nes::str_para<E>(s);            };
        std::optional<E> operator()(long long int v) const      { return static_cast<E>(v);              };
        std::optional<E> operator()(double) const               { throw runtime_exc { "to_opt_enum!" };  };
      };

      return std::visit(visit_rol_data_t {}, m_data);
    }

    // Representação em string
    std::string to_str() const;
    std::optional<std::string> to_ostr() const;

    // Comparações
    auto operator<=>(const rol_data&) const = default;
  };

  // Representação de tamanho fixo da matriz
  class rol;

  template <std::size_t N>
  class rol_fixed final
  {
    // Matriz com os dados
    std::array<rol_col_data, N> m_cols;
    std::vector<std::array<rol_data, N>> m_data;

  public:
    // Auxiliar para ler os dados
    class rol_const_fixed_linha final
    {
        const rol_fixed<N>* m_rol_fixed_ref { nullptr };
        std::size_t m_no_linha { 0 };

      public:
        rol_const_fixed_linha(const rol_fixed<N>&, std::size_t);

        const rol_data& operator[](std::string_view) const;
    };

    class rol_const_fixed_linhas final
    {
        const rol_fixed<N>* m_rol_fixed_ref { nullptr };

      public:
        class rol_const_fixed_linha_iterator final
        {
          const rol_fixed<N>* m_rol_fixed_ref { nullptr };
          std::size_t m_no_linha { 0 };

        public:
          using difference_type   = std::ptrdiff_t;
          using value_type        = rol_const_fixed_linha_iterator;
          using pointer           = value_type*;
          using const_pointer     = const value_type*;
          using reference         = value_type&;
          using const_reference   = const value_type&;
          using iterator_category = std::bidirectional_iterator_tag;

          rol_const_fixed_linha_iterator() = default;
          rol_const_fixed_linha_iterator(const rol_fixed<N>&, std::size_t);

          rol_const_fixed_linha_iterator& operator++();
          rol_const_fixed_linha_iterator operator++(int);
          rol_const_fixed_linha_iterator& operator--();
          rol_const_fixed_linha_iterator operator--(int);
          rol_const_fixed_linha operator*() const;

          bool operator==(const rol_const_fixed_linha_iterator&) const;
          bool operator!=(const rol_const_fixed_linha_iterator&) const;
        };

        rol_const_fixed_linhas(const rol_fixed<N>&);

        rol_const_fixed_linha_iterator begin() const;
        rol_const_fixed_linha_iterator end() const;

        std::size_t size_row() const;
    };

    class rol_fixed_linha final
    {
        rol_fixed<N>* m_rol_fixed_ref { nullptr };
        std::size_t m_no_linha { 0 };

      public:
        rol_fixed_linha(rol_fixed<N>&, std::size_t);

        rol_data& operator[](std::string_view) const;
    };

    class rol_fixed_linhas final
    {
        rol_fixed<N>* m_rol_fixed_ref { nullptr };

      public:
        class rol_fixed_linha_iterator final
        {
          rol_fixed<N>* m_rol_fixed_ref { nullptr };
          std::size_t m_no_linha { 0 };

        public:
          using difference_type   = std::ptrdiff_t;
          using value_type        = rol_fixed_linha_iterator;
          using pointer           = value_type*;
          using const_pointer     = const value_type*;
          using reference         = value_type&;
          using const_reference   = const value_type&;
          using iterator_category = std::bidirectional_iterator_tag;

          rol_fixed_linha_iterator() = default;
          rol_fixed_linha_iterator(rol_fixed<N>&, std::size_t);

          rol_fixed_linha_iterator& operator++();
          rol_fixed_linha_iterator operator++(int);
          rol_fixed_linha_iterator& operator--();
          rol_fixed_linha_iterator operator--(int);
          rol_fixed_linha operator*() const;

          bool operator==(const rol_fixed_linha_iterator&) const;
          bool operator!=(const rol_fixed_linha_iterator&) const;
        };

        rol_fixed_linhas(rol_fixed<N>&);

        rol_fixed_linha_iterator begin() const;
        rol_fixed_linha_iterator end() const;

        std::size_t size_row() const;
    };

    rol_fixed() = default;
    rol_fixed(std::array<rol_col_data, N>, std::vector<std::array<rol_data, N>> = {});

    // A partir da dinâmica
    explicit rol_fixed(const rol&);

    // Coluna
    rol_col_data& col(std::size_t);
    const rol_col_data& col(std::size_t) const;

    void cols(std::array<rol_col_data, N>);
    const std::array<rol_col_data, N>& cols() const;

    std::size_t size_col() const;

    void cols_name(std::array<std::string, N>);

    // Linha
    void rows(std::vector<std::array<rol_data, N>>);
    std::span<std::array<rol_data, N>> rows();
    const std::vector<std::array<rol_data, N>>& rows() const;

    rol_fixed_linhas named_rows();
    const rol_const_fixed_linhas named_rows() const;

    void insert_row(std::size_t, std::array<rol_data, N>);
    void push_back_row(std::array<rol_data, N>);

    std::size_t size_row() const;

    // Acesso
    rol_data& at(std::size_t, std::string_view);
    const rol_data& at(std::size_t, std::string_view) const;

    // Operadores
    //std::array<rol_data, N>& operator[](std::size_t);
    //const std::array<rol_data, N>& operator[](std::size_t) const;

    rol_data& operator[](std::pair<std::size_t, std::size_t>);
    const rol_data& operator[](std::pair<std::size_t, std::size_t>) const;

    // Comparação
    auto operator<=>(const rol_fixed&) const = default;
  };

  // Representação dinamica da matriz
  class rol final
  {
    // Matriz com as colunas e dados
    std::vector<rol_col_data> m_cols;
    std::vector<rol_data> m_data;

  public:
    // Auxiliar para ler os dados
    class rol_const_linha final
    {
        const rol* m_rol_ref { nullptr };
        std::size_t m_no_linha { 0 };

      public:
        rol_const_linha(const rol&, std::size_t);

        const rol_data& operator[](std::string_view) const;
    };

    class rol_const_linhas final
    {
        const rol* m_rol_ref { nullptr };

      public:
        class rol_const_linha_iterator final
        {
          const rol* m_rol_ref { nullptr };
          std::size_t m_no_linha { 0 };

        public:
          using difference_type   = std::ptrdiff_t;
          using value_type        = rol_const_linha;
          using const_pointer     = const value_type*;
          using const_reference   = const value_type;
          using iterator_category = std::bidirectional_iterator_tag;

          rol_const_linha_iterator() = default;
          rol_const_linha_iterator(const rol&, std::size_t);

          rol_const_linha_iterator& operator++();
          rol_const_linha_iterator operator++(int);
          rol_const_linha_iterator& operator--();
          rol_const_linha_iterator operator--(int);
          rol_const_linha operator*() const;

          bool operator==(const rol_const_linha_iterator&) const;
          bool operator!=(const rol_const_linha_iterator&) const;
        };

        rol_const_linhas(const rol&);

        rol_const_linha_iterator begin() const;
        rol_const_linha_iterator end() const;

        std::size_t size_row() const;
    };

    class rol_linha final
    {
        rol* m_rol_ref { nullptr };
        std::size_t m_no_linha { 0 };

      public:
        rol_linha(rol&, std::size_t);

        rol_data& operator[](std::string_view) const;
    };

    class rol_linhas final
    {
        rol* m_rol_ref { nullptr };

      public:
        class rol_linha_iterator final
        {
          rol* m_rol_ref { nullptr };
          std::size_t m_no_linha { 0 };

        public:
          using difference_type   = std::ptrdiff_t;
          using value_type        = rol_linha;
          using pointer           = value_type*;
          using const_pointer     = const value_type*;
          using reference         = value_type;
          using const_reference   = const value_type;
          using iterator_category = std::bidirectional_iterator_tag;

          rol_linha_iterator() = default;
          rol_linha_iterator(rol&, std::size_t);

          rol_linha_iterator& operator++();
          rol_linha_iterator operator++(int);
          rol_linha_iterator& operator--();
          rol_linha_iterator operator--(int);
          rol_linha operator*() const;

          bool operator==(const rol_linha_iterator&) const;
          bool operator!=(const rol_linha_iterator&) const;
        };

        rol_linhas(rol&);

        rol_linha_iterator begin() const;
        rol_linha_iterator end() const;

        std::size_t size_row() const;
    };

    // Construtores
    rol() = default;
    rol(std::vector<rol_col_data>, std::vector<rol_data> = {});

    // A partir da fixa
    template <std::size_t N>
    explicit rol(const rol_fixed<N>&);

    // Coluna
    void insert_col(std::size_t, rol_col_data);
    void push_back_col(rol_col_data);

    rol_col_data& col(std::size_t);
    const rol_col_data& col(std::size_t) const;

    void cols(std::vector<rol_col_data>);
    const std::vector<rol_col_data>& cols() const;

    std::size_t size_col() const;

    void cols_name(std::vector<std::string>);

    void col_type(std::size_t, rol_col_t);
    rol_col_t col_type(std::size_t) const;

    void col_name_type(std::string_view, rol_col_t);
    rol_col_t col_name_type(std::string_view) const;

    // Linha
    void rows(std::vector<rol_data>);
    std::vector<std::span<rol_data>> rows();
    std::vector<std::span<const rol_data>> rows() const;

    rol_linhas named_rows();
    const rol_const_linhas named_rows() const;

    void insert_row(std::size_t, const std::vector<rol_data>&);
    void push_back_row(const std::vector<rol_data>&);

    std::size_t size_row() const;

    // Acesso
    rol_data& at(std::size_t, std::string_view);
    const rol_data& at(std::size_t, std::string_view) const;

    // Operadores
    //std::span<rol_data> operator[](std::size_t);
    //std::span<const rol_data> operator[](std::size_t) const;

    rol_data& operator[](std::pair<std::size_t, std::size_t>);
    const rol_data& operator[](std::pair<std::size_t, std::size_t>) const;

    // Comparação
    auto operator<=>(const rol&) const = default;
  };

}

#endif
// NES_BD__ROL_H