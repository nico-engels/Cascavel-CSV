#ifndef NES_BD__SQLITE_API_H
#define NES_BD__SQLITE_API_H

#include <string_view>
#include <optional>
#include "rol.h"

// Protópipos de estruturas específicas do SQLite, (evita importar a biblioteca)
struct sqlite3;

namespace nes::bd {

  enum class sqlite_con_opts { normal, poup_bd_utils };

  class sqlite_api final
  {
    // Manipulador da conexão
    sqlite3* m_sqlite_con { nullptr };

  public:

    // Construtor
    sqlite_api() = default;
    ~sqlite_api();

    // Mover
    sqlite_api(sqlite_api&&) noexcept;
    sqlite_api& operator=(sqlite_api&&) noexcept;

    // Sem cópias
    sqlite_api(const sqlite_api&) = delete;
    sqlite_api& operator=(const sqlite_api&) = delete;

    // Conexão
    // Arquivo de Banco
    void conectar(std::string_view, sqlite_con_opts = sqlite_con_opts::poup_bd_utils);

    // Sobre o status da conexão
    bool conectado() const;

    // DQL
    std::optional<rol> exec_sql(std::string_view);
  };

}

#endif
// NES_BD__SQLITE_API_H