#include "console.h"

#include <print>
using namespace std;

void uso_exe(string_view msg /*= {}*/)
{
    println("Cascavel CSV [arquivo_csv] [-c 'comando'] [-o arquivo_saida]\n"
            "  arquivo_csv:      O arquivo CSV para ler os dados\n"
            "                    - Ao interpretar tenta deduzir o delimitador, caso não seja possível usa o ';'.\n"
            "  -c \"comando\":     Processa o arquivo utilizando o comando (use --ajuda-csv-sql para detalhes).\n"
            "  -d  delim:        Delimitador do arquivo de entrada. Por padrão segue o arquivo de entrada.\n"
            "  -ds delim:        Delimitador do arquivo de saída. Por padrão é o mesmo da entrada.\n"
            "                    Valores para delim:\n"
            "                    - pv:  Ponto-e-vígula ';'.\n"
            "                    - v:   Vírgula ','.\n"
            "                    - tab: Tabulação '\\t'.\n"
            "  -f:               Adiciona o delimitador ao final da linha. Por padrão segue o arquivo de entrada.\n"
            "  -no-f:            Não adiciona o delimitador ao final da linha. Por padrão segue o arquivo de entrada.\n"
            "  -h:               Essa mensagem de ajuda.\n"
            "  --ajuda           Essa mensagem de ajuda.\n"
            "  --ajuda-csv-sql:  Ajuda sobre a forma de usar SQL para consultar dados no CSV.\n"
            "  -o arquivo_saida: Arquivo para gravar o resultado do processamento.\n"
            "  -q                Indica que deve colocar os itens com aspas. Por padrão segue o arquivo de entrada.\n"
            "  -no-q             Indica que não deve colocar os itens com aspas. Por padrão segue o arquivo de entrada.\n"
            "  -ig               Ignorar erros básicos de leiaute, linhas com problema.\n"
            "\n"
            "{}", msg);
    exit(1);
}

map<string, string> interpretar_args(span<const char*> args)
{
    static const string_view csv_sql_help =
      "Formato do csv-sql:\n"
      "  SELECT [colunas]\n"
      "   [FROM tabela-csv]\n"
      "  [WHERE condicao]\n"
      "Onde:\n"
      "  SELECT:       Literal caso insensitivo.\n"
      "  [colunas]:    Podem representar\n"
      "                - col(i)      : imprime os valores da coluna i (baseado em 0).\n"
      "  [tabela-csv]: O pseudo-nome do CSV carregado (sempre tab_1).\n"
      "  [condicao]:   Condição boleana para filtro.\n";

    if (args.size() < 2)
      uso_exe();

    map<string, string> ret {
      { "programa",    args[0] },
    };

    for (size_t i = 1; i < args.size(); i++) {
      string arg(args[i]);

      if (ret.contains(arg))
        uso_exe("Parâmetro '" + arg + "' duplicado.");

      if (arg == "-h" || arg == "--ajuda")
        uso_exe();

      if (arg == "--ajuda-csv-sql")
        uso_exe(csv_sql_help);

      if (arg.at(0) != '-')
        ret["arquivo_csv"] = arg;
      else if (arg == "-c" && i + 1 < args.size())
      {
        string param(args[++i]);
        ret[arg] = param;
      }
      else if (arg == "-o" && i + 1 < args.size())
      {
        string param(args[++i]);
        ret[arg] = param;
      }
      else if ((arg == "-d" || arg == "-ds") && i + 1 < args.size())
      {
        string param(args[++i]);

        if (param != "pv" && param != "v" && param != "tab")
          uso_exe("Parâmetro do -d \"" + param + "\" em formato inválido.");

        ret[arg] = param;
      }
      else if (arg == "-f")
        ret["-f"] = "sim";
      else if (arg == "-no-f")
        ret["-no-f"] = "sim";
      else if (arg == "-q")
        ret["-q"] = "sim";
      else if (arg == "-no-q")
        ret["-no-q"] = "sim";
      else if (arg == "-ig")
        ret["-ig"] = "sim";
      else
        uso_exe("Parâmetro '" + arg + "' não reconhecido ou necessita de parâmetro.");
    }

    if (ret.contains("-q") && ret.contains("-no-q"))
      uso_exe("Parâmetro -q e -no-q não podem ser usados juntos.");

    if (ret.contains("-f") && ret.contains("-no-f"))
      uso_exe("Parâmetro -f e -no-f não podem ser usados juntos.");

    return ret;
}