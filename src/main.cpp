#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <spanstream>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include "console.h"
#include "runtime_exc.h"
#include "sqlite_api.h"
#include "str_util.h"
using namespace std;
using namespace std::chrono;
using namespace nes;
using namespace nes::bd;
namespace fs = std::filesystem;
namespace rng = std::ranges;
namespace vw = std::views;

struct cmd_cfg_t
{
    char delim_ent;
    char delim_saida;
    bool delim_fim_linha;
    bool must_quoted;
    optional<string> sql_out;
    bool ignorar_erros;
};

cmd_cfg_t extrair_cfg(const map<string, string>&, ifstream&);
void bd_criar_estrutura(sqlite_api&, ifstream&, char);
void bd_processa_dados(sqlite_api&, ifstream&, char, bool);
void bd_gravar_dados_tabela(sqlite_api&, string_view, const vector<string>&);
void gerar_saida(sqlite_api&, ostream&, char, bool, bool, string_view);

int main(int argc, const char* argv[])
try {
    const auto args = interpretar_args(span(argv, argc));

    // Arquivo de entrada
    ifstream arq_ent(args.at("arquivo_csv"));
    if (!arq_ent)
      throw runtime_exc { "Não foi possível abrir o arquivo '{}' para leitura!", args.at("arquivo_csv") };

    // Arquivo de saída (parâmetro -o ou saída padrão)
    ostream* arq_saida = nullptr;
    ofstream saida;
    if (args.contains("-o"))
    {
      // Valida se a entrada e saída não são o mesmo arquivo
      if (args.at("arquivo_csv") == args.at("-o"))
        throw runtime_exc { "Não é possível utilizar o arquivo de entrada como saída!" };

      saida.open(args.at("-o"));
      if (!saida)
        throw runtime_exc { "Não foi possível abrir o arquivo '{}' para escrita!", args.at("-o") };
      else
        arq_saida = &saida;
    }
    else
      arq_saida = &cout;

    const auto [delim_ent, delim_saida, delim_fim_linha, must_quoted, sql_out, ign_err] = extrair_cfg(args, arq_ent);

    // Criar o bd temporário para as manipulações
    sqlite_api con;

    //fs::remove("..\\examples\\test.bd");

    //con.conectar("..\\examples\\test.bd");
    con.conectar("");

    // Processamento
    // 1º - Estrutura
    bd_criar_estrutura(con, arq_ent, delim_ent);

    // 2º - Verifica se a consulta funciona
    (void)con.exec_sql(sql_out.value_or("SELECT * FROM tab_0 t1")).value();

    // 3º - Dados
    bd_processa_dados(con, arq_ent, delim_ent, ign_err);

    // 4º - Gerar Saída
    gerar_saida(con, *arq_saida, delim_saida, delim_fim_linha, must_quoted, sql_out.value_or("SELECT * FROM tab_0 t1"));


} catch (const exception& e) {
    println("{}", e.what());
}

cmd_cfg_t extrair_cfg(const map<string, string>& args, ifstream& arq_ent)
{
    // Delimitador de entrada
    optional<char> delim_ent;
    if (args.contains("-d"))
    {
      if (args.at("-d") == "v")
        delim_ent = ',';
      else if (args.at("-d") == "tab")
        delim_ent = '\t';
    }

    // Delimitador de saída
    optional<char> delim_saida = delim_ent;
    if (args.contains("-ds"))
    {
      if (args.at("-ds") == "v")
        delim_saida = ',';
      else if (args.at("-ds") == "tab")
        delim_saida = '\t';
      else if (args.at("-ds") == "pv")
        delim_saida = ';';
    }

    // Adicionar o delimitador no fim da linha
    optional<bool> delim_fim_linha;

    if (args.contains("-f"))
      delim_fim_linha = true;
    else if (args.contains("-no-f"))
      delim_fim_linha = false;

    // Se deve ser colocado entre aspas
    optional<bool> must_quoted;
    if (args.contains("-q"))
      must_quoted = true;
    else if (args.contains("-no-q"))
      must_quoted = false;

    // Se alguma configuração não foi definida pega do arquivo de entrada
    if (!delim_ent || !delim_saida || !delim_fim_linha || !must_quoted)
    {
      string linha;
      if (getline(arq_ent, linha))
      {
        auto pos = linha.find_first_not_of(' ');
        if (pos != string::npos)
        {
          if (!must_quoted)
            must_quoted = linha[pos] == '"';

          if (!delim_ent)
          {
            int num_v = 0;
            int num_pv = 0;
            int num_tab = 0;

            for (auto i = pos; i < linha.size(); i++) {
              if (linha[i] == ',') num_v++;
              else if (linha[i] == ';') num_pv++;
              else if (linha[i] == '\t') num_tab++;
            }

            if (num_v > num_pv && num_v > num_tab)
              delim_ent = ',';
            else if (num_pv > num_v && num_pv > num_tab)
              delim_ent = ';';
            else if (num_tab > num_v && num_tab > num_pv)
              delim_ent = '\t';
            else
              delim_ent = ';';
          }

          if (!delim_saida)
            delim_saida = delim_ent;

          if (!delim_fim_linha)
          {
            pos = linha.find_last_not_of(' ');
            delim_fim_linha = linha.at(pos) == delim_ent;
          }
        }
      }
      else
        throw runtime_exc { "Não foi possível ler o cabeçalho para definir os parâmetros!" };

      arq_ent.seekg(0);
    }

    // Comando sql
    optional<string> sql;
    if (args.contains("-c"))
      sql = args.at("-c");

    if (!delim_ent || !delim_saida || !delim_fim_linha || !must_quoted)
      throw runtime_exc { "Não foi possível definir os parâmetros!" };

    bool ign = args.contains("-ig");

    return { *delim_ent, *delim_saida, *delim_fim_linha, *must_quoted, sql, ign };
}

void bd_criar_estrutura(sqlite_api& con, ifstream& arq_ent, char delim_ent)
{
    // Tabela auxiliar
    con.exec_sql("CREATE TABLE csv_col (TAB VARCHAR(10), ORD INTEGER, NAME VARCHAR(255))");

    // Processar
    string linha;
    string dado;
    vector<string> linhas_bd;

    // Processar cabeçalho
    // Primeiro definir as colunas
    vector<string> cols;
    if (getline(arq_ent, linha))
    {
      ispanstream is(linha);
      while (getline(is, dado, delim_ent))
        cols.push_back(string { trim(trim(dado), '"') });
    }
    else
      throw runtime_exc { "Não foi possível ler o cabeçalho!" };

    // Cria a tabela e metadados das colunas
    string sql_tab = "CREATE TABLE tab_0 (";
    for (const auto [i, c] : cols | vw::enumerate) {
      linhas_bd.push_back(format("('tab_0', {}, '{}')", i, c));

      if (i > 0)
        sql_tab += ", ";
      sql_tab += format("col_{} VARCHAR(255)", i);
    }
    sql_tab += ")";

    bd_gravar_dados_tabela(con, "csv_col (TAB, ORD, NAME)", linhas_bd);

    con.exec_sql(sql_tab);
}

void bd_processa_dados(sqlite_api& con, ifstream& arq_ent, char delim_ent, bool ign_err)
{
    // Processar linhas
    size_t linha_atual = 1;
    string sql_ins;
    string linha_cont;

    string linha;
    string dado;
    vector<string> linhas_bd;

    vector<pair<size_t, string>> erros_proc;

    while (getline(arq_ent, linha))
    {
      if (!linha_cont.empty())
      {
        linha = linha_cont + linha;
        linha_cont.clear();
      }

      if (rng::count(linha, '"') % 2)
      {
        linha_cont = linha + "\\r\\n";
        continue;
      }

      ispanstream is(linha);
      sql_ins += "(";

      if (getline(is, dado, delim_ent))
      {
        sql_ins += format("'{}'", replace_all(trim(trim(dado), '"'), "'", "''"));
        while (getline(is, dado, delim_ent))
          sql_ins += format(", '{}'", replace_all(trim(trim(dado), '"'), "'", "''"));
        sql_ins += ")";
      }

      linhas_bd.push_back(move(sql_ins));
      sql_ins.clear();

      if (linhas_bd.size() > 100)
      try {
        bd_gravar_dados_tabela(con, "tab_0", linhas_bd);
        linhas_bd.clear();
      } catch (const exception& e) {
        erros_proc.push_back({ linha_atual, e.what() });
        linhas_bd.clear();
        if (!ign_err)
          break;
      }

      linha_atual++;
    }

    if (!linhas_bd.empty()) try {
      bd_gravar_dados_tabela(con, "tab_0", linhas_bd);
    } catch (const exception& e) {
      erros_proc.push_back({ linha_atual, e.what() });
    }

    if (!erros_proc.empty())
    {
      if (!ign_err)
        throw runtime_exc { "linha {}\n{}", erros_proc[0].first, erros_proc[0].second };
      println("{} erros ao processar linhas.", erros_proc.size());
    }
}

void bd_gravar_dados_tabela(sqlite_api& con, string_view tabela, const vector<string>& dados)
{
    string sql = "INSERT INTO " + string { tabela } + " VALUES ";

    for (auto ch : dados | vw::join_with(string { ", " }))
        sql += ch;

    try {
      con.exec_sql(sql);
    } catch (const exception& e) {
      throw runtime_exc { "Erro ao inserir: '{}'\nErro retornado pelo BD: '{}'", sql, e.what() };
    }
}

void gerar_saida(sqlite_api& con, ostream& arq_saida, char delim_saida, bool delim_fim_linha, bool must_quoted, string_view sql_out)
{
    // Gerar a saída
    // Primeiro busca o nome das colunas para fazer o mapeamento de-para com as colunas de resultado
    auto rs = con.exec_sql("SELECT cc.name FROM csv_col cc WHERE cc.tab = 'tab_0' ORDER BY cc.ord").value();

    // De-para nome colunas
    map<string, string> de_para_cols;
    for (auto [i, r] : rs.rows() | vw::join | vw::enumerate)
      de_para_cols[format("col_{}", i)] = r.to_str();

    // Dados
    rs = con.exec_sql(sql_out).value();

    // Cabeçalho
    for (bool primeiro = true; const auto& c : rs.cols()) {
      string col;
      if (de_para_cols.contains(c.name()))
        col = de_para_cols[c.name()];
      else
        col = c.name();

      if (!primeiro)
        arq_saida << delim_saida;

      if (must_quoted)
        arq_saida << "\"" << col << "\"";
      else
        arq_saida << col;

      primeiro = false;
    }
    if (delim_fim_linha)
      arq_saida << delim_saida;
    arq_saida << "\n";

    // Linhas
    for (const auto& r : rs.rows()) {
      for (auto ch : r | vw::transform([&](const auto& rol_data) {
                                         if (must_quoted)
                                           return string { "\"" } + rol_data.to_str() + "\"";
                                         else
                                           return rol_data.to_str(); })
                       | vw::join_with(delim_saida))
        arq_saida << ch;

      if (delim_fim_linha)
        arq_saida << delim_saida;
      arq_saida << '\n';
    }

}