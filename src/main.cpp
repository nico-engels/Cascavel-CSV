#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <generator>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <spanstream>
#include <string>
#include <string_view>
#include <sstream>
#include <tuple>
#include <utility>
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
    bool sem_cabecalho;
};

extern generator<vector<string>>* aaa;

using gen_it_type = decltype(aaa->begin())&;

cmd_cfg_t extrair_cfg(const map<string, string>&, ifstream&);
generator<vector<string>> gerador_linhas_csv(ifstream&, char);
int bd_criar_estrutura(sqlite_api&, gen_it_type, bool);
void bd_processa_dados(sqlite_api&, gen_it_type, bool, int);
void bd_gravar_dados_tabela(sqlite_api&, string_view, const vector<string>&);
void gerar_saida(sqlite_api&, ostream&, char, bool, bool, string_view, bool);

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

    const auto [delim_ent, delim_saida, delim_fim_linha, must_quoted, sql_out, ign_err, sem_cabecalho] =
      extrair_cfg(args, arq_ent);

    // Criar o bd temporário para as manipulações
    sqlite_api con;

    //fs::remove("..\\examples\\test.bd");

    //con.conectar("..\\examples\\test.bd");
    con.conectar("");

    // Usando coroutines
    // Processamento
    // 1º - Criar gerador das linhas
    auto gn = gerador_linhas_csv(arq_ent, delim_ent);
    auto gn_it = gn.begin();

    // 2º - Estrutura
    auto num_cols = bd_criar_estrutura(con, gn_it, sem_cabecalho);

    // Se não tem cabeçalho devolve a linha consumida
    if (sem_cabecalho)
      arq_ent.seekg(0);
    gn_it++;

    // 3º - Verifica se a consulta funciona
    (void)con.exec_sql(sql_out.value_or("SELECT * FROM tab_0 t1")).value();

    // 4º - Dados
    bd_processa_dados(con, gn_it, ign_err, num_cols);

    // 5º - Gerar Saída
    gerar_saida(con, *arq_saida, delim_saida, delim_fim_linha, must_quoted,
      sql_out.value_or("SELECT * FROM tab_0 t1"), sem_cabecalho);

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
    bool sem_cabecalho = args.contains("-sem-cabecalho");

    return { *delim_ent, *delim_saida, *delim_fim_linha, *must_quoted, sql, ign, sem_cabecalho };
}

generator<vector<string>> gerador_linhas_csv(ifstream& arq_ent, char delim_ent)
{
    string linha;
    string linha_cont;
    string dado;

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
      vector<string> valores;

      while (getline(is, dado, delim_ent))
        valores.push_back(format("'{}'", replace_all(trim(trim(dado), '"'), "'", "''")));

      co_yield valores;
    }

    co_return;
}

int bd_criar_estrutura(sqlite_api& con, gen_it_type gni, bool sem_cabecalho)
{
    // Tabela auxiliar
    con.exec_sql("CREATE TABLE csv_col (TAB VARCHAR(10), ORD INTEGER, NAME VARCHAR(255))");

    // Processar
    string linha;
    string dado;
    vector<string> linhas_bd;

    // Processar cabeçalho
    auto cols = *gni;

    // Se for sem cabeçalho renomeia as colunas
    if (sem_cabecalho)
      for (size_t i = 0; i < cols.size(); i++)
        cols[i] = format("'a{}'", i);

    // Cria a tabela e metadados das colunas
    string sql_tab = "CREATE TABLE tab_0 (";
    for (const auto [i, c] : cols | vw::enumerate) {
      linhas_bd.push_back(format("('tab_0', {}, {})", i, c));

      if (i > 0)
        sql_tab += ", ";
      sql_tab += format("col_{} VARCHAR(255)", i);
    }
    sql_tab += ")";

    bd_gravar_dados_tabela(con, "csv_col (TAB, ORD, NAME)", linhas_bd);

    con.exec_sql(sql_tab);

    return cols.size();
}

void bd_processa_dados(sqlite_api& con, gen_it_type gni, bool ign_err, int num_cols)
{
    // Processar linhas
    size_t linha_atual = 1;
    vector<tuple<size_t, string>> erros_proc;

    for (auto grupo_linhas : rng::subrange { move(gni), default_sentinel } | vw::chunk(100)) {

      vector<string> csv_ins_sql;
      int max_cols = num_cols;

      for (auto linha : grupo_linhas) {

        // Se aumentou as colunas reajusta os sqls
        if (cmp_less(max_cols, linha.size()))
        {
          string null_apend;
          for (int i = max_cols; cmp_less(i, linha.size()); i++)
            null_apend += ", null";

          for (auto& ins_sql : csv_ins_sql)
            ins_sql += null_apend;

          max_cols = linha.size();
        }

        // Criação de colunas se necessário
        for (int i = linha.size(); i < max_cols; i++)
          linha.push_back("null");

        string linha_sql = "(" + (linha | vw::join_with(string { ", " }) | rng::to<string>());
        csv_ins_sql.push_back(linha_sql);
        linha_atual++;
      }

      // Gravação
      // Novas colunas
      {
        vector<string> novas_cols;
        for (; num_cols < max_cols; num_cols++) {
          novas_cols.push_back(format("('tab_0', {0}, 'a{0}')", num_cols));
          con.exec_sql(format("ALTER TABLE tab_0 ADD col_{} VARCHAR(255)", num_cols));
        }
        if (!novas_cols.empty())
          bd_gravar_dados_tabela(con, "csv_col (TAB, ORD, NAME)", novas_cols);
      }

      // Dados
      try {
        for (auto& ins_sql : csv_ins_sql)
          ins_sql += ")";

        bd_gravar_dados_tabela(con, "tab_0", csv_ins_sql);

      } catch (const exception& e) {
        erros_proc.push_back({ linha_atual, e.what() });

        if (!ign_err)
          break;
      }
    }

    if (!erros_proc.empty())
    {
      if (!ign_err)
        throw runtime_exc { "linha {}\nErro\n{}", get<0>(erros_proc.back()),  get<1>(erros_proc.back()) };
      println("{} erros ao processar linhas.", erros_proc.size());
      for (const auto& [linha, erro] : erros_proc)
        println("{} => {}", linha, erro);
    }
}

void bd_gravar_dados_tabela(sqlite_api& con, string_view tabela, const vector<string>& valores_linhas)
{
    string sql = "INSERT INTO " + string { tabela } + " VALUES ";

    sql += valores_linhas | vw::join_with(string { ", " }) | rng::to<string>();

    try {
      con.exec_sql(sql);
    } catch (const exception&) {
      for (const auto& valores : valores_linhas)
        con.exec_sql(format("INSERT INTO {} VALUES {}", tabela, valores));
      throw;
    }
}

void gerar_saida(sqlite_api& con, ostream& arq_saida, char delim_saida, bool delim_fim_linha, bool must_quoted,
  string_view sql_out, bool sem_cabecalho)
{
    // Gerar a saída
    // Primeiro faz a consulta
    // Dados
    auto rs = con.exec_sql(sql_out).value();

    // Segundo busca o nome das colunas para fazer o mapeamento de-para com as colunas de resultado
    if (!sem_cabecalho)
    {
      auto rs_col = con.exec_sql("SELECT cc.name FROM csv_col cc WHERE cc.tab = 'tab_0' ORDER BY cc.ord").value();

      // De-para nome colunas
      map<string, string> de_para_cols;
      for (auto [i, r] : rs_col.rows() | vw::join | vw::enumerate)
        de_para_cols[format("col_{}", i)] = r.to_str();

      // Cabeçalho
      for (bool primeiro = true; const auto& c : rs.cols()) {
        string col;
        if (de_para_cols.contains(c.name())) {
          col = de_para_cols[c.name()];
        }
        else {
          col = c.name();
        }

        if (!primeiro)
          print(arq_saida, "{}", delim_saida);

        if (must_quoted)
          print(arq_saida, "\"{}\"", col);
        else
          print(arq_saida, "{}", col);

        primeiro = false;
      }
      if (delim_fim_linha)
        print(arq_saida, "{}", delim_saida);
      println(arq_saida, "");
    }

    // Linhas
    for (const auto& r : rs.rows()) {
      print(arq_saida, "{}", r | vw::take_while([](const auto& rol_data) { return !rol_data.is_nullopt(); })
                               | vw::transform([&](const auto& rol_data)
                                 {
                                   if (must_quoted)
                                     return string { "\"" } + rol_data.to_str() + "\"";
                                   else
                                     return rol_data.to_str();
                                 })
                                | vw::join_with(delim_saida)
                                | rng::to<string>());

      if (delim_fim_linha)
        print(arq_saida, "{}", delim_saida);
      println(arq_saida, "");
    }

}