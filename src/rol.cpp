#include "rol.h"

#include <algorithm>
#include <stdexcept>

#include "runtime_exc.h"
using namespace std;
namespace rng = std::ranges;
using namespace nes;
using namespace nes::bd;

// Funções utilitárias
void validar(span<const rol_col_data>, span<const rol_data>);

namespace nes::bd {

  // Alias para usar static_casts
  using vec_diff_type = std::vector<rol_data>::difference_type;

  // rol_col_data
  rol_col_data::rol_col_data(const char* s)
    : m_name { string { s }}
  {

  }

  rol_col_data::rol_col_data(string s)
    : m_name { move(s) }
  {

  }

  rol_col_data::rol_col_data(const char* s, rol_col_t t)
    : m_name { string { s } }
    , m_type { t }
  {

  }

  rol_col_data::rol_col_data(string s, rol_col_t t)
    : m_name { move(s) }
    , m_type { t }
  {

  }

  void rol_col_data::name(string s)
  {
    m_name = move(s);
  }

  const string& rol_col_data::name() const
  {
    return m_name;
  }

  void rol_col_data::type(rol_col_t t)
  {
    m_type = t;
  }

  rol_col_t rol_col_data::type() const
  {
    return m_type;
  }

  // rol_data
  rol_data::rol_data(nullopt_t)
  {

  }

  rol_data::rol_data(char c)
    : m_data { string { c }}
  {

  }

  rol_data::rol_data(const char* s)
    : m_data { string { s }}
  {

  }

  rol_data::rol_data(string s)
    : m_data { move(s) }
  {

  }

  rol_data::rol_data(int v)
  {
    this->int_value(v);
  }

  rol_data::rol_data(size_t v)
  {
    this->size_t_value(v);
  }

  rol_data::rol_data(float f)
  {
    this->float_value(f);
  }

  rol_data::rol_data(optional<char> c)
  {
    if (c)
      m_data = string { *c };
  }

  rol_data::rol_data(optional<string> s)
  {
    if (s)
      m_data = move(*s);
  }

  rol_data::rol_data(optional<int> v)
  {
    if (v)
      this->int_value(*v);
  }

  rol_data::rol_data(optional<size_t> v)
  {
    if (v)
      this->size_t_value(*v);
  }

  rol_data::rol_data(optional<float> f)
  {
    if (f)
      this->float_value(*f);
  }

  void rol_data::set_nullopt()
  {
    m_data = monostate {};
  }

  bool rol_data::is_nullopt() const
  {
    return get_if<monostate>(&m_data);
  }

  bool rol_data::empty() const
  {
    return get_if<monostate>(&m_data);
  }

  bool rol_data::is_str() const
  {
    return get_if<string>(&m_data);
  }

  void rol_data::str(string s)
  {
    m_data = move(s);
  }

  const string& rol_data::str() const
  {
    return get<string>(m_data);
  }

  string rol_data::str_or(string_view sv)
  {
    return this->is_str() ? this->str() : string { sv };
  }

  bool rol_data::is_int_value() const
  {
    return get_if<long long int>(&m_data);
  }

  void rol_data::int_value(int v)
  {
    m_data = static_cast<long long int>(v);
  }

  int rol_data::int_value() const
  {
    return static_cast<int>(get<long long int>(m_data));
  }

  int rol_data::int_value_or(int v) const
  {
    return this->is_int_value() ? this->int_value() : v;
  }

  bool rol_data::is_size_t_value() const
  {
    return get_if<long long int>(&m_data);
  }

  void rol_data::size_t_value(size_t v)
  {
    m_data = static_cast<long long int>(v);
  }

  size_t rol_data::size_t_value() const
  {
    return static_cast<size_t>(get<long long int>(m_data));
  }

  size_t rol_data::size_t_value_or(size_t v) const
  {
    return this->is_size_t_value() ? this->size_t_value() : v;
  }

  bool rol_data::is_float_value() const
  {
    return get_if<double>(&m_data);
  }

  void rol_data::float_value(float f)
  {
    m_data = static_cast<double>(f);
  }

  float rol_data::float_value() const
  {
    return static_cast<float>(get<double>(m_data));
  }

  float rol_data::float_value_or(float f)
  {
    return this->is_float_value() ? this->float_value() : f;
  }

  rol_data::operator optional<string>() const
  {
    if (this->is_nullopt())
      return nullopt;

    if (this->is_str())
      return this->str();
    else
      throw runtime_exc { "Não pode converter para opt<str>!" };
  }

  rol_data::operator optional<size_t>() const
  {
    if (this->is_nullopt())
      return nullopt;

    if (this->is_size_t_value())
      return this->size_t_value();
    else
      throw runtime_exc { "Não pode converter para opt<size_t>!" };
  }

  rol_data::operator optional<float>() const
  {
    if (this->is_nullopt())
      return nullopt;

    if (this->is_float_value())
      return this->float_value();
    else
      throw runtime_exc { "Não pode converter para opt<float>!" };
  }

  string rol_data::to_str() const
  {
    struct visit_rol_data_t {
      string operator()(monostate) const       { return "<N>";        };
      string operator()(const string& s) const { return s;            };
      string operator()(long long int v) const { return to_string(v); };
      string operator()(double d) const        { return to_string(d); };
    };

    return visit(visit_rol_data_t {}, m_data);
  }

  optional<string> rol_data::to_ostr() const
  {
    struct visit_rol_data_t {
      optional<string> operator()(monostate) const       { return nullopt;      };
      optional<string> operator()(const string& s) const { return s;            };
      optional<string> operator()(long long int v) const { return to_string(v); };
      optional<string> operator()(double d) const        { return to_string(d); };
    };

    return visit(visit_rol_data_t {}, m_data);
  }

  // rol_fixed<N>
  // Auxiliar
  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linha::rol_const_fixed_linha(const rol_fixed<N>& rol_fixed_ref, size_t no_linha)
    : m_rol_fixed_ref { &rol_fixed_ref }
    , m_no_linha { no_linha }
  {

  }

  template <size_t N>
  const rol_data& rol_fixed<N>::rol_const_fixed_linha::operator[](string_view col_name) const
  {
    if (m_rol_fixed_ref)
      return m_rol_fixed_ref->at(m_no_linha, col_name);

    throw runtime_exc { "rol_const_fixed_linha nullprt!" };
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::rol_const_fixed_linha_iterator(
    const rol_fixed<N>& rol_fixed_ref, size_t no_linha)
    : m_rol_fixed_ref { &rol_fixed_ref }
    , m_no_linha { no_linha }
  {

  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator&
    rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator++()
  {
    m_no_linha++;

    return *this;
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator
    rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator++(int)
  {
    rol_const_fixed_linha_iterator it { *this };
    ++*this;
    return it;
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator&
    rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator--()
  {
    m_no_linha--;

    return *this;
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator
    rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator--(int)
  {
    rol_const_fixed_linha_iterator it { *this };
    --*this;
    return it;
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linha
    rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator*() const
  {
    return rol_fixed<N>::rol_const_fixed_linha { *m_rol_fixed_ref, m_no_linha };
  }

  template <size_t N>
  bool rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator==(const rol_const_fixed_linha_iterator& outro) const
  {
    return m_rol_fixed_ref == outro.m_rol_fixed_ref && m_no_linha == outro.m_no_linha;
  }

  template <size_t N>
  bool rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator::operator!=(const rol_const_fixed_linha_iterator& outro) const
  {
    return !(*this == outro);
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linhas(const rol_fixed<N>& rol_fixed_ref)
    : m_rol_fixed_ref { &rol_fixed_ref }
  {

  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator
    rol_fixed<N>::rol_const_fixed_linhas::begin() const
  {
    if (!m_rol_fixed_ref)
      throw runtime_exc { "rol_const_fixed_linhas::begin nullptr!" };

    return rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator { *m_rol_fixed_ref, 0 };
  }

  template <size_t N>
  rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator
    rol_fixed<N>::rol_const_fixed_linhas::end() const
  {
    if (!m_rol_fixed_ref)
      throw runtime_exc { "rol_const_fixed_linhas::end nullptr!" };

    return rol_fixed<N>::rol_const_fixed_linhas::rol_const_fixed_linha_iterator { *m_rol_fixed_ref, this->size_row() };
  }

  template <size_t N>
  size_t rol_fixed<N>::rol_const_fixed_linhas::size_row() const
  {
    return m_rol_fixed_ref->size_row();
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linha::rol_fixed_linha(rol_fixed<N>& rol_fixed_ref, size_t no_linha)
    : m_rol_fixed_ref { &rol_fixed_ref }
    , m_no_linha { no_linha }
  {

  }

  template <size_t N>
  rol_data& rol_fixed<N>::rol_fixed_linha::operator[](string_view col_name) const
  {
    if (m_rol_fixed_ref)
      return m_rol_fixed_ref->at(m_no_linha, col_name);

    throw runtime_exc { "rol_fixed_linha nullprt!" };
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::rol_fixed_linha_iterator(
    rol_fixed<N>& rol_fixed_ref, size_t no_linha)
    : m_rol_fixed_ref { &rol_fixed_ref }
    , m_no_linha { no_linha }
  {

  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator&
    rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator++()
  {
    m_no_linha++;

    return *this;
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator
    rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator++(int)
  {
    rol_fixed_linha_iterator it { *this };
    ++*this;
    return it;
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator&
    rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator--()
  {
    m_no_linha--;

    return *this;
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator
    rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator--(int)
  {
    rol_fixed_linha_iterator it { *this };
    --*this;
    return it;
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linha
    rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator*() const
  {
    return rol_fixed<N>::rol_fixed_linha { *m_rol_fixed_ref, m_no_linha };
  }

  template <size_t N>
  bool rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator==(const rol_fixed_linha_iterator& outro) const
  {
    return m_rol_fixed_ref == outro.m_rol_fixed_ref && m_no_linha == outro.m_no_linha;
  }

  template <size_t N>
  bool rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator::operator!=(const rol_fixed_linha_iterator& outro) const
  {
    return !(*this == outro);
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linhas(rol_fixed<N>& rol_fixed_ref)
    : m_rol_fixed_ref { &rol_fixed_ref }
  {

  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator
    rol_fixed<N>::rol_fixed_linhas::begin() const
  {
    if (!m_rol_fixed_ref)
      throw runtime_exc { "rol_fixed_linhas::begin nullptr!" };

    return rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator { *m_rol_fixed_ref, 0 };
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator
    rol_fixed<N>::rol_fixed_linhas::end() const
  {
    if (!m_rol_fixed_ref)
      throw runtime_exc { "rol_fixed_linhas::end nullptr!" };

    return rol_fixed<N>::rol_fixed_linhas::rol_fixed_linha_iterator { *m_rol_fixed_ref, this->size_row() };
  }

  template <size_t N>
  size_t rol_fixed<N>::rol_fixed_linhas::size_row() const
  {
    return m_rol_fixed_ref->size_row();
  }

  // Construtores
  template <size_t N>
  rol_fixed<N>::rol_fixed(array<rol_col_data, N> cols, vector<array<rol_data, N>> data /*= {}*/)
  {
    // Valida
    if (!data.empty())
      validar(cols, span { data.data()->data(), data.size() * cols.size() });

    // Seta
    m_cols = move(cols);
    m_data = move(data);
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed(const rol& r)
  {
    vector<array<rol_data, N>> data;

    data.resize(r.size_row());
    const auto& r_rows = r.rows();
    for (size_t i = 0; i < r.size_row(); i++)
      rng::copy(r_rows[i], data[i].begin());

    // Seta
    rng::copy(r.cols(), m_cols.begin());
    m_data = move(data);
  }

  template <size_t N>
  rol_col_data& rol_fixed<N>::col(size_t i)
  {
    return m_cols[i];
  }

  template <size_t N>
  const rol_col_data& rol_fixed<N>::col(size_t i) const
  {
    return m_cols[i];
  }

  template <size_t N>
  void rol_fixed<N>::cols(array<rol_col_data, N> cs)
  {
    m_cols = move(cs);
    m_data.clear();
  }

  template <size_t N>
  const array<rol_col_data, N>& rol_fixed<N>::cols() const
  {
    return m_cols;
  }

  template <size_t N>
  size_t rol_fixed<N>::size_col() const
  {
    return m_cols.size();
  }

  template <size_t N>
  void rol_fixed<N>::cols_name(array<string, N> names)
  {
    for (size_t i = 0; i < names.size() && i < m_cols.size(); i++)
      m_cols[i].name(move(names[i]));
  }

  template <size_t N>
  void rol_fixed<N>::rows(vector<array<rol_data, N>> data)
  {
    // Valida
    if (!data.empty())
      validar(this->cols(), span { data.data()->data(), data.size() * this->size_col() });

    // Seta
    m_data = move(data);
  }

  template <size_t N>
  span<array<rol_data, N>> rol_fixed<N>::rows()
  {
    return m_data;
  }

  template <size_t N>
  const vector<array<rol_data, N>>& rol_fixed<N>::rows() const
  {
    return m_data;
  }

  template <size_t N>
  rol_fixed<N>::rol_fixed_linhas rol_fixed<N>::named_rows()
  {
    return rol_fixed<N>::rol_fixed_linhas { *this };
  }

  template <size_t N>
  const rol_fixed<N>::rol_const_fixed_linhas rol_fixed<N>::named_rows() const
  {
    return rol_fixed<N>::rol_const_fixed_linhas { *this };
  }

  template <size_t N>
  void rol_fixed<N>::insert_row(size_t i, array<rol_data, N> row)
  {
    // Validações
    // Índice
    if (i > this->size_row())
      throw runtime_exc { "Problema no índice do insert!" };

    // Tipos
    for (size_t j = 0; j < this->size_col(); j++) {
      switch (this->col(j).type())
      {
        case rol_col_t::str:
        {
          if (!row[j].is_nullopt() && !row[j].is_str())
            throw runtime_exc { "Esperava dados do tipo str para a coluna!" };
          break;
        }
        case rol_col_t::int_value:
        {
          if (!row[j].is_nullopt() && !row[j].is_int_value())
            throw runtime_exc { "Esperava dados do tipo int para a coluna!" };
          break;
        }
        case rol_col_t::float_value:
        case rol_col_t::currency:
        {
          if (!row[j].is_nullopt() && !row[j].is_float_value())
            throw runtime_exc { "Esperava dados do tipo float para a coluna!" };
          break;
        }
        case rol_col_t::unknown:
          break;
      }
    }

    // Por fim insere
    m_data.insert(next(m_data.begin(), static_cast<vec_diff_type>(i)), move(row));
  }

  template <size_t N>
  void rol_fixed<N>::push_back_row(array<rol_data, N> row)
  {
    this->insert_row(this->size_row(), move(row));
  }

  template <size_t N>
  size_t rol_fixed<N>::size_row() const
  {
    return m_data.size();
  }

  template <size_t N>
  rol_data& rol_fixed<N>::at(size_t i, string_view col_name)
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return m_data[i][static_cast<size_t>(it - m_cols.begin())];
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  template <size_t N>
  const rol_data& rol_fixed<N>::at(size_t i, string_view col_name) const
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return m_data[i][static_cast<size_t>(it - m_cols.begin())];
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  /*
  template <size_t N>
  array<rol_data, N>& rol_fixed<N>::operator[](size_t i)
  {
    return m_data[i];
  }

  template <size_t N>
  const array<rol_data, N>& rol_fixed<N>::operator[](size_t i) const
  {
    return m_data[i];
  }
  */

  template <size_t N>
  rol_data& rol_fixed<N>::operator[](pair<size_t, size_t> index)
  {
    auto [i, j] = index;
    return m_data[i][j];
  }

  template <size_t N>
  const rol_data& rol_fixed<N>::operator[](pair<size_t, size_t> index) const
  {
    auto [i, j] = index;
    return m_data[i][j];
  }

  // rol
  // Auxiliar
  rol::rol_const_linha::rol_const_linha(const rol& rol_ref, size_t no_linha)
    : m_rol_ref { &rol_ref }
    , m_no_linha { no_linha }
  {

  }

  const rol_data& rol::rol_const_linha::operator[](string_view col_name) const
  {
    if (m_rol_ref)
      return m_rol_ref->at(m_no_linha, col_name);

    throw runtime_exc { "rol_const_linha nullprt!" };
  }

  rol::rol_const_linhas::rol_const_linha_iterator::rol_const_linha_iterator(const rol& rol_ref, size_t no_linha)
    : m_rol_ref { &rol_ref }
    , m_no_linha { no_linha }
  {

  }

  rol::rol_const_linhas::rol_const_linha_iterator& rol::rol_const_linhas::rol_const_linha_iterator::operator++()
  {
    m_no_linha++;

    return *this;
  }

  rol::rol_const_linhas::rol_const_linha_iterator rol::rol_const_linhas::rol_const_linha_iterator::operator++(int)
  {
    rol_const_linha_iterator it { *this };
    ++*this;
    return it;
  }

  rol::rol_const_linhas::rol_const_linha_iterator& rol::rol_const_linhas::rol_const_linha_iterator::operator--()
  {
    m_no_linha--;

    return *this;
  }

  rol::rol_const_linhas::rol_const_linha_iterator rol::rol_const_linhas::rol_const_linha_iterator::operator--(int)
  {
    rol_const_linha_iterator it { *this };
    --*this;
    return it;
  }

  rol::rol_const_linha rol::rol_const_linhas::rol_const_linha_iterator::operator*() const
  {
    return rol::rol_const_linha { *m_rol_ref, m_no_linha };
  }

  bool rol::rol_const_linhas::rol_const_linha_iterator::operator==(const rol_const_linha_iterator& outro) const
  {
    return m_rol_ref == outro.m_rol_ref && m_no_linha == outro.m_no_linha;
  }

  bool rol::rol_const_linhas::rol_const_linha_iterator::operator!=(const rol_const_linha_iterator& outro) const
  {
    return !(*this == outro);
  }

  rol::rol_const_linhas::rol_const_linhas(const rol& rol_ref)
    : m_rol_ref { &rol_ref }
  {

  }

  rol::rol_const_linhas::rol_const_linha_iterator rol::rol_const_linhas::begin() const
  {
    if (!m_rol_ref)
      throw runtime_exc { "rol_const_linhas::begin nullptr!" };

    return rol::rol_const_linhas::rol_const_linha_iterator { *m_rol_ref, 0 };
  }

  rol::rol_const_linhas::rol_const_linha_iterator rol::rol_const_linhas::end() const
  {
    if (!m_rol_ref)
      throw runtime_exc { "rol_const_linhas::end nullptr!" };

    return rol::rol_const_linhas::rol_const_linha_iterator { *m_rol_ref, this->size_row() };
  }

  size_t rol::rol_const_linhas::size_row() const
  {
    return m_rol_ref->size_row();
  }

  rol::rol_linha::rol_linha(rol& rol_ref, size_t no_linha)
    : m_rol_ref { &rol_ref }
    , m_no_linha { no_linha }
  {

  }

  rol_data& rol::rol_linha::operator[](string_view col_name) const
  {
    if (m_rol_ref)
      return m_rol_ref->at(m_no_linha, col_name);

    throw runtime_exc { "rol_linha nullprt!" };
  }

  rol::rol_linhas::rol_linha_iterator::rol_linha_iterator(rol& rol_ref, size_t no_linha)
    : m_rol_ref { &rol_ref }
    , m_no_linha { no_linha }
  {

  }

  rol::rol_linhas::rol_linha_iterator& rol::rol_linhas::rol_linha_iterator::operator++()
  {
    m_no_linha++;

    return *this;
  }

  rol::rol_linhas::rol_linha_iterator rol::rol_linhas::rol_linha_iterator::operator++(int)
  {
    rol_linha_iterator it { *this };
    ++*this;
    return it;
  }

  rol::rol_linhas::rol_linha_iterator& rol::rol_linhas::rol_linha_iterator::operator--()
  {
    m_no_linha--;

    return *this;
  }

  rol::rol_linhas::rol_linha_iterator rol::rol_linhas::rol_linha_iterator::operator--(int)
  {
    rol_linha_iterator it { *this };
    --*this;
    return it;
  }

  rol::rol_linha rol::rol_linhas::rol_linha_iterator::operator*() const
  {
    return rol::rol_linha { *m_rol_ref, m_no_linha };
  }

  bool rol::rol_linhas::rol_linha_iterator::operator==(const rol_linha_iterator& outro) const
  {
    return m_rol_ref == outro.m_rol_ref && m_no_linha == outro.m_no_linha;
  }

  bool rol::rol_linhas::rol_linha_iterator::operator!=(const rol_linha_iterator& outro) const
  {
    return !(*this == outro);
  }

  rol::rol_linhas::rol_linhas(rol& rol_ref)
    : m_rol_ref { &rol_ref }
  {

  }

  rol::rol_linhas::rol_linha_iterator rol::rol_linhas::begin() const
  {
    if (!m_rol_ref)
      throw runtime_exc { "rol_linhas::begin nullptr!" };

    return rol::rol_linhas::rol_linha_iterator { *m_rol_ref, 0 };
  }

  rol::rol_linhas::rol_linha_iterator rol::rol_linhas::end() const
  {
    if (!m_rol_ref)
      throw runtime_exc { "rol_linhas::end nullptr!" };

    return rol::rol_linhas::rol_linha_iterator { *m_rol_ref, this->size_row() };
  }

  size_t rol::rol_linhas::size_row() const
  {
    return m_rol_ref->size_row();
  }

  // Construtor
  rol::rol(vector<rol_col_data> cols, vector<rol_data> data /*= {} */)
  {
    // Valida
    validar(cols, data);

    // Seta
    m_cols = move(cols);
    m_data = move(data);
  }

  template <size_t N>
  rol::rol(const rol_fixed<N>& rf)
  {
    vector<rol_data> data;

    data.reserve(rf.size_row() * rf.size_col());
    for (const auto& r : rf.rows())
      data.insert(data.end(), r.begin(), r.end());

    // Seta
    m_cols.insert(m_cols.end(), rf.cols().begin(), rf.cols().end());
    m_data = move(data);
  }

  void rol::insert_col(size_t i, rol_col_data c)
  {
    // Valida o índice
    if (i > this->size_col())
      throw runtime_exc { "Problema no índice do insert!" };

    // Ajusta os dados
    if (this->size_row())
    {
      vector<rol_data> data;
      data.reserve((this->size_col() + 1) * this->size_row());

      for (size_t j = 0; j < m_data.size(); j++) {
        // Nova coluna
        if (j % this->size_col() == i)
          data.push_back(nullopt);

        // Copia o valor
        data.push_back(m_data[j]);

        // Nova coluna na última posição
        if (this->size_col() == i && (j % this->size_col()) + 1 == this->size_col())
          data.push_back(nullopt);
      }

      // Novos dados
      m_data = move(data);
    }

    m_cols.insert(next(m_cols.begin(), static_cast<vec_diff_type>(i)), move(c));
  }

  void rol::push_back_col(rol_col_data col)
  {
    this->insert_col(this->size_col(), move(col));
  }

  rol_col_data& rol::col(size_t i)
  {
    return m_cols[i];
  }

  const rol_col_data& rol::col(size_t i) const
  {
    return m_cols[i];
  }

  const vector<rol_col_data>& rol::cols() const
  {
    return m_cols;
  }

  void rol::cols(vector<rol_col_data> cs)
  {
    m_cols = move(cs);
    m_data.clear();
  }

  size_t rol::size_col() const
  {
    return m_cols.size();
  }

  void rol::cols_name(vector<string> names)
  {
    for (size_t i = 0; i < names.size() && i < m_cols.size(); i++)
      m_cols[i].name(move(names[i]));
  }

  void rol::col_type(size_t i, rol_col_t type)
  {
    if (m_cols[i].type() != type)
    {
      m_cols[i].type(type);
      validar(m_cols, m_data);
    }
  }

  rol_col_t rol::col_type(size_t i) const
  {
    return m_cols[i].type();
  }

  void rol::col_name_type(string_view col_name, rol_col_t type)
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return this->col_type(static_cast<size_t>(it - m_cols.begin()), type);
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  rol_col_t rol::col_name_type(string_view col_name) const
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return this->col_type(rng::distance(m_cols.begin(), it));
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  void rol::rows(vector<rol_data> data)
  {
    // Valida
    validar(m_cols, data);

    // Seta
    m_data = move(data);
  }

  vector<span<rol_data>> rol::rows()
  {
    vector<span<rol_data>> ret;

    for (auto it = m_data.begin(); it != m_data.end(); it += static_cast<vec_diff_type>(this->size_col()))
      ret.push_back(span<rol_data> { it, it + static_cast<vec_diff_type>(this->size_col()) });

    return ret;
  }

  vector<span<const rol_data>> rol::rows() const
  {
    vector<span<const rol_data>> ret;

    for (auto it = m_data.cbegin(); it != m_data.cend(); it += static_cast<vec_diff_type>(this->size_col()))
      ret.push_back(span<const rol_data> { it, it + static_cast<vec_diff_type>(this->size_col()) });

    return ret;
  }

  rol::rol_linhas rol::named_rows()
  {
    return rol::rol_linhas { *this };
  }

  const rol::rol_const_linhas rol::named_rows() const
  {
    return rol::rol_const_linhas { *this };
  }

  void rol::insert_row(size_t i, const vector<rol_data>& row)
  {
    // Validações
    // Índice
    if (i > this->size_row())
      throw runtime_exc { "Problema no índice do insert!" };

    // Colunas
    if (row.size() != this->size_col())
      throw runtime_exc { "Quantidade de dados da linha ({}), não é o mesmo das colunas ({})!", row.size(), this->size_col() };

    // Tipos
    for (size_t j = 0; j < this->size_col(); j++) {
      switch (this->col(j).type())
      {
        case rol_col_t::str:
        {
          if (!row[j].is_nullopt() && !row[j].is_str())
            throw runtime_exc { "Esperava dados do tipo str para a coluna!" };
          break;
        }
        case rol_col_t::int_value:
        {
          if (!row[j].is_nullopt() && !row[j].is_int_value())
            throw runtime_exc { "Esperava dados do tipo int para a coluna!" };
          break;
        }
        case rol_col_t::float_value:
        case rol_col_t::currency:
        {
          if (!row[j].is_nullopt() && !row[j].is_float_value())
            throw runtime_exc { "Esperava dados do tipo float para a coluna!" };
          break;
        }
        case rol_col_t::unknown:
          break;
      }
    }

    // Por fim insere
    m_data.insert(next(m_data.begin(), static_cast<vec_diff_type>(i * this->size_col())), row.begin(), row.end());
  }

  void rol::push_back_row(const vector<rol_data>& row)
  {
    this->insert_row(this->size_row(), row);
  }

  size_t rol::size_row() const
  {
    if (this->size_col())
      return m_data.size() / this->size_col();
    else
      return 0;
  }

  rol_data& rol::at(size_t i, string_view col_name)
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return m_data[i * this->size_col() + static_cast<size_t>(it - m_cols.begin())];
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  const rol_data& rol::at(size_t i, string_view col_name) const
  {
    auto it = rng::find_if(m_cols, [&](const auto& e) { return e.name() == col_name; });
    if (it != m_cols.end())
      return m_data[i * this->size_col() + static_cast<size_t>(it - m_cols.begin())];
    else
      throw runtime_exc { "Coluna '{}' não encontrada!", col_name };
  }

  /*
  span<rol_data> rol::operator[](size_t i)
  {
    auto b = next(m_data.begin(), static_cast<vec_diff_type>(i * this->size_col()));
    auto e = b + static_cast<vec_diff_type>(this->size_col());
    return span<rol_data> { b, e };
  }

  span<const rol_data> rol::operator[](size_t i) const
  {
    auto b = next(m_data.cbegin(), static_cast<vec_diff_type>(i * this->size_col()));
    auto e = b + static_cast<vec_diff_type>(this->size_col());
    return span<const rol_data> { b, e };
  }
  */

  rol_data& rol::operator[](pair<size_t, size_t> index)
  {
    auto [i, j] = index;
    return m_data[i * this->size_col() + j];
  }

  const rol_data& rol::operator[](pair<size_t, size_t> index) const
  {
    auto [i, j] = index;
    return m_data[i * this->size_col() + j];
  }

  // rols usadas no sistema
  template class rol_fixed< 1>;
  template class rol_fixed< 2>;
  template class rol_fixed< 3>;
  template class rol_fixed< 4>;
  template class rol_fixed< 5>;
  template class rol_fixed< 6>;
  template class rol_fixed< 7>;
  template class rol_fixed< 8>;
  template class rol_fixed< 9>;
  template class rol_fixed<10>;

  template rol::rol(const rol_fixed< 1>&);
  template rol::rol(const rol_fixed< 2>&);
  template rol::rol(const rol_fixed< 3>&);
  template rol::rol(const rol_fixed< 4>&);
  template rol::rol(const rol_fixed< 5>&);

}

void validar(span<const rol_col_data> cols, span<const rol_data> data)
{
    // Tamanho
    if ((!cols.size() && data.size()) || (cols.size() && data.size() % cols.size()))
      throw runtime_exc { "Tamanho de linha incorreto! rol({} % {})", cols.size(), data.size() };

    // Consistência tipos
    for (size_t i = 0; i < cols.size(); i++) {
      switch (cols[i].type())
      {
        case rol_col_t::str:
        {
          for (size_t j = i; j < data.size(); j += cols.size())
            if (!data[j].is_nullopt() && !data[j].is_str())
              throw runtime_exc { "Esperava dados do tipo str para a coluna!" };
          break;
        }
        case rol_col_t::int_value:
        {
          for (size_t j = i; j < data.size(); j += cols.size())
            if (!data[j].is_nullopt() && !data[j].is_int_value())
              throw runtime_exc { "Esperava dados do tipo int para a coluna!" };
          break;
        }
        case rol_col_t::float_value:
        {
          for (size_t j = i; j < data.size(); j += cols.size())
            if (!data[j].is_nullopt() && !data[j].is_float_value())
              throw runtime_exc { "Esperava dados do tipo float para a coluna!" };
          break;
        }

        case rol_col_t::currency:
        case rol_col_t::unknown:
          break;
      }
    }
}