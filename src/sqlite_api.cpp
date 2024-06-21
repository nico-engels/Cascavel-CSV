#include "sqlite_api.h"

#include <chrono>
#include <format>
#include <iomanip>
#include <sqlite3.h>
#include <sstream>
#include <string_view>
#include <thread>
#include "runtime_exc.h"
#include "str_util.h"
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace nes;

namespace nes::bd {

  // Escopo para o sqlite3_stmt
  class sqlite3_stmt_raii {
    sqlite3_stmt *m_handle { nullptr };
  public:
    sqlite3_stmt_raii() = default;
    ~sqlite3_stmt_raii() { if (m_handle) sqlite3_finalize(m_handle); };
    sqlite3_stmt*& handle() { return m_handle; };
  };

  sqlite_api::~sqlite_api()
  {
    if (m_sqlite_con)
    {
      sqlite3_close(m_sqlite_con);
      m_sqlite_con = nullptr;
    }
  }

  sqlite_api::sqlite_api(sqlite_api&& outro) noexcept
    : m_sqlite_con { outro.m_sqlite_con }
  {
    outro.m_sqlite_con = nullptr;
  }

  sqlite_api& sqlite_api::operator=(sqlite_api&& outro) noexcept
  {
    swap(m_sqlite_con, outro.m_sqlite_con);

    return *this;
  }

  void sqlite_api::conectar(string_view nome_base, sqlite_con_opts opts /*= sqlite_con_opts::poup_bd_utils*/)
  {
    // Validação se já conectado
    if (m_sqlite_con)
      throw runtime_exc { "Já conectado ao SQLite!" };

    // Conexão ao arquivo de banco de dados
    sqlite3* db = nullptr;
    if (sqlite3_open(nome_base.data(), &db))
    {
      string msg(sqlite3_errmsg(db));
      sqlite3_close(db);
      throw runtime_exc { "Não foi possível conectar: {}", msg };
    }

    // Extensão de funções nativas utilizadas pelo Poup
    if (opts == sqlite_con_opts::poup_bd_utils)
    {
      // pp_dt_fmt chamada genérica para formatar data
      if (sqlite3_create_function_v2(db, "pp_dt_fmt", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr,
         [](sqlite3_context *context, int argc, sqlite3_value **argv) {
           if (argc == 2)
           {
             const char* argv1 = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
             const char* argv2 = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));
             if (argv1 && argv2)
             {
               istringstream in(string{ argv1 });
               ostringstream out;
               string_view sarg_2(argv2);

               int ano, mes, dia;
               char sep;
               if (in >> ano >> sep >> mes >> sep >> dia && sep == '-')
               {
                 if (sarg_2 == "%d/%m/%Y")
                 {
                   out << setfill('0') << setw(2) << dia << '/' << setw(2) << mes << '/' << setw(4) << ano;
                   sqlite3_result_text(context, out.str().c_str(), -1, SQLITE_TRANSIENT);
                   return;
                 }
               }
             }
           }

           sqlite3_result_null(context);
         }, nullptr, nullptr, nullptr) != SQLITE_OK)
      {
        sqlite3_close(db);
        throw runtime_exc { "Não foi configurar função 'pp_dt_fmt'" };
      }

      // pp_dt_month_add faz modificações mensais na data
      if (sqlite3_create_function_v2(db, "pp_dt_month_add", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr,
         [](sqlite3_context *context, int argc, sqlite3_value **argv) {
           if (argc == 2)
           {
             const char* argv1 = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
             const char* argv2 = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));
             if (argv1 && argv2)
             {
               istringstream in1(string{ argv1 });
               istringstream in2(string{ argv2 });
               string data;
               int count;

               if (in1 >> data && in2 >> count)
               {
                 try {
                   //  Se exiver no formato yyyy-mm ou yyyy-mm-dd
                   if (data.size() == 7 || data.size() == 10)
                   {
                     year_month_day dt;
                     if (data.size() == 7)
                       dt = str_para<year_month>(data) / day { 1 };
                     else if (data.size() == 10)
                       dt = str_para<year_month_day>(data);

                     dt += months { count };
                     if (!dt.ok())
                       dt = dt.year() / dt.month() / last;

                     string ret;
                     if (data.size() == 7)
                       ret = format("{:%Y-%m}", dt);
                     else
                       ret = format("{:%F}", dt);

                     sqlite3_result_text(context, ret.c_str(), -1, SQLITE_TRANSIENT);
                     return;
                   }

                 } catch (...) {

                 }
               }
             }
           }

           sqlite3_result_null(context);
         }, nullptr, nullptr, nullptr) != SQLITE_OK)
      {
        sqlite3_close(db);
        throw runtime_exc { "Não foi configurar função 'pp_dt_fmt'" };
      }

      // concat chamada genérica concatenar strings
      if (sqlite3_create_function_v2(db, "concat", -1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr,
         [](sqlite3_context *context, int argc, sqlite3_value **argv) {

           string ret;
           for (int i = 0; i < argc; i++) {
             const auto* pargv = reinterpret_cast<const char*>(sqlite3_value_text(argv[i]));
             if (pargv)
               ret.append(pargv);
           }
           sqlite3_result_text(context, ret.c_str(), -1, SQLITE_TRANSIENT);

         }, nullptr, nullptr, nullptr) != SQLITE_OK)
      {
        sqlite3_close(db);
        throw runtime_exc { "Não foi configurar função 'concat'" };
      }
    }

    // Seta o handle
    m_sqlite_con = db;
  }

  bool sqlite_api::conectado() const
  {
    return m_sqlite_con;
  }

  optional<rol> sqlite_api::exec_sql(string_view sql)
  {
    // Validações são feitas no bd_con_tmpl
    // Aqui pode assumir conectado e sql não vazia

    sqlite3_stmt_raii stmt;
    if (sqlite3_prepare_v2(m_sqlite_con, sql.data(), -1, &stmt.handle(), nullptr) != SQLITE_OK)
      throw runtime_exc { "Erro na preparação do sql {}", sqlite3_errmsg(m_sqlite_con) };

    // Loop de extração das informações
    std::vector<rol_col_data> colunas;
    std::vector<rol_data> dados;

    // Nome da coluna apenas se tiver linhas então deixa como unknown
    int num_col = sqlite3_column_count(stmt.handle());
    if (num_col)
      colunas.resize(static_cast<size_t>(num_col), { "unknown" });

    int ret;
    int num_rows = 0;
    do {
      // Bloqueia se caso esteja esperando algum lock
      while ((ret = sqlite3_step(stmt.handle())) == SQLITE_BUSY)
        this_thread::sleep_for(1ms);

      if (ret != SQLITE_ROW && ret != SQLITE_DONE)
        throw runtime_exc { "Erro ao recuperar os dados {}", sqlite3_errmsg(m_sqlite_con) };

      // Na primeira linha identifica o nome das colunas
      if (num_rows == 0)
        for (size_t i = 0; i < colunas.size(); i++)
          colunas[i].name(sqlite3_column_name(stmt.handle(), static_cast<int>(i)));

      // Nova linha de dados
      if (ret == SQLITE_ROW)
      {
        for (size_t i = 0; i < colunas.size(); i++) {

          int colt = sqlite3_column_type(stmt.handle(), static_cast<int>(i));

          // Primeiro verifica se é null, se sim adiciona o valor nulo
          if (colt == SQLITE_NULL)
            dados.push_back(nullopt);
          else
          {
            // Se a coluna ainda não tem um tipo define
            if (colunas[i].type() == rol_col_t::unknown)
            {
              if (colt == SQLITE_INTEGER)
                colunas[i].type(rol_col_t::int_value);
              else if (colt == SQLITE_FLOAT)
                colunas[i].type(rol_col_t::float_value);
              else if (colt == SQLITE_TEXT || colt == SQLITE_BLOB)
                colunas[i].type(rol_col_t::str);
            }
            else if (colunas[i].type() == rol_col_t::int_value && colt == SQLITE_FLOAT)
            {
              // Problema que ocorre quanto a coluna é de floats porém o sqlite retorna int quando é 0
              // Muda o tipo da coluna e ajustas os dados inseridos
              colunas[i].type(rol_col_t::float_value);
              for (size_t j = i; j < dados.size(); j += colunas.size())
                dados[j].float_value(static_cast<float>(dados[j].int_value()));
            }

            // Extrai o dado conforme o tipo
            switch (colunas[i].type())
            {
              case rol_col_t::int_value:
                dados.push_back(sqlite3_column_int(stmt.handle(), static_cast<int>(i)));
                break;
              case rol_col_t::float_value:
              case rol_col_t::currency:
                dados.push_back(static_cast<float>(sqlite3_column_double(stmt.handle(), static_cast<int>(i))));
                break;
              case rol_col_t::str:
              {
                const char* pstr = reinterpret_cast<const char*>(sqlite3_column_text(stmt.handle(), static_cast<int>(i)));
                if (pstr)
                  dados.push_back(string { pstr });
                else
                  dados.push_back(string {});
                break;
              }
              case rol_col_t::unknown: break;
            }
          }
        }
        num_rows++;
      }
    } while (ret != SQLITE_DONE);

    // Se tem retorno retorna os dados
    if (colunas.size())
      return rol { colunas, dados };
    else
      return nullopt;
  }

}