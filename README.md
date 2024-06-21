# Cascavel-CSV 🐍

Utilitário para processamento de dados em arquivos tabulares CSV. Tratando o CSV como uma
fonte de dados relacional. Permitindo manipular, analisar e transformar os arquivos CSV
usando a linguagem SQL.

# Tecnologias

C++ 23 mapeando o CSV em um Banco de Dados SQLite de forma temporária. Dependendo do tamanho do
arquivo CSV o arquivo é mapeado em memória, senão usa um arquivo temporário.

# Compilação

Na pasta ./build executar o cmake com as configurações de seu ambiente. Exemplos:

```
Windows/Mingw64
set PATH=%PATH%;C:\msys64\mingw64\bin\
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

Windows/MSVC++
set PATH=%PATH%;"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\"
cmake -G "Visual Studio 17 2022" ..

Ubuntu
cmake -DCMAKE_BUILD_TYPE=Release ..

Raspberry Pi OS
cmake -DCMAKE_BUILD_TYPE=Release .. -DCMAKE_C_COMPILER=/usr/bin/arm-linux-gnueabihf-gcc -DCMAKE_CXX_COMPILER=/usr/bin/arm-linux-gnueabihf-g++

Após configurado
cmake --build .
```

O executável cascavel_csv estará na pasta ./bin

# cascavel_csv -h

```
Cascavel CSV [arquivo_csv] [-c 'comando'] [-o arquivo_saida]
  arquivo_csv:      O arquivo CSV para ler os dados.
                    - Ao interpretar tenta deduzir o delimitador, caso não seja possível usa o ';'.
  -c "comando":     Processa o arquivo utilizando o comando (use --ajuda-csv-sql para detalhes).
  -d  delim:        Delimitador do arquivo de entrada. Por padrão segue o arquivo de entrada.
  -ds delim:        Delimitador do arquivo de saída. Por padrão é o mesmo da entrada.
                    Valores para delim:
                    - pv:  Ponto-e-vígula ';'.
                    - v:   Vírgula ','.
                    - tab: Tabulação '\t'.
  -f:               Adiciona o delimitador ao final da linha. Por padrão segue o arquivo de entrada.
  -no-f:            Não adiciona o delimitador ao final da linha. Por padrão segue o arquivo de entrada.
  -h:               Essa mensagem de ajuda.
  --ajuda:          Essa mensagem de ajuda.
  --ajuda-csv-sql:  Ajuda sobre a forma de usar SQL para consultar dados no CSV.
  -o arquivo_saida: Arquivo para gravar o resultado do processamento.
  -q                Indica que deve colocar os itens com aspas. Por padrão segue o arquivo de entrada.
  -no-q             Indica que não deve colocar os itens com aspas. Por padrão segue o arquivo de entrada.
```

## SQL

É possível executar comandos SQL como se o CSV interpretado fosse uma tabela e cada coluna.

O CSV é tratado pelo nome tab_1 e cada coluna do CSV segue o formato col_0, col_1, ..., col_n.

Ver abaixo exemplos de consultas.

# Exemplos

Acessando um arquivo com o estoque de uma mercearia:

```
arquivo estoque.csv

"categoria";"item";"quantidade";"preço"
"café da manhã";"leite";"200";"6.55"
"frutas";"banana";"100";"2.15"
"café da manhã";"pão";"5";"7.00"
"frutas";"maça";"150";"1.67"
"café da manhã";"queijo";"15";"8.00"
"legumes";"cenoura";"30";"1.44"
```

## Básico

Por padrão o Cascavel-CSV imprime o resultado na tela. Então lendo o arquivo e não
realizando nenhuma operação ele apenas lê o arquivo:

cascavel_csv estoque.csv

Saída terminal:

```
"categoria";"item";"quantidade";"preço"
"café da manhã";"leite";"200";"6.55"
"frutas";"banana";"100";"2.15"
"café da manhã";"pão";"5";"7.00"
"frutas";"maça";"150";"1.67"
"café da manhã";"queijo";"15";"8.00"
"legumes";"cenoura";"30";"1.44"
```

## Delimitador de saída

É possível trocar os delimitadores de saída:

`cascavel_csv estoque.csv -ds tab`

Saída terminal:

```
"categoria"	"item"	"quantidade"	"preço"
"café da manhã"	"leite"	"200"	"6.55"
"frutas"	"banana"	"100"	"2.15"
"café da manhã"	"pão"	"5"	"7.00"
"frutas"	"maça"	"150"	"1.67"
"café da manhã"	"queijo"	"15"	"8.00"
"legumes"	"cenoura"	"30"	"1.44"
```

## Consulta

Realizar consultas SQL. Todos os itens que sejam da categoria frutas:

`cascavel_csv estoque.csv -c "SELECT * FROM tab_1 WHERE col_0 = 'frutas'"`

Saída terminal:

```
"categoria";"item";"quantidade";"preço"
"frutas";"banana";"100";"2.15"
"frutas";"maça";"150";"1.67"
```

Todas as categorias

`cascavel_csv estoque.csv -no-q -c "SELECT distinct col_0 FROM tab_1 ORDER BY col_0"`

Saída:

```
categoria
café da manhã
frutas
legumes
```

Quantidade de itens do estoque:

`cascavel_csv estoque.csv -c "SELECT count(1) as qtde FROM tab_1"`

Saída:

```
"qtde"
"6"
```

Valor do estoque

`cascavel_csv estoque.csv -c "SELECT printf('%.2f', sum(col_2 * col_3)) as 'Valor Estoque' FROM tab_1"`

Saída:

```
"Valor Estoque"
"1973.70"
```