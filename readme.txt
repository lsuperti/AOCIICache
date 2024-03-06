Compilação:
Execute `$ make` ou `$ make all` na pasta do trabalho.
Requer o gcc e uma implementação do make.
Se ocorrerem erros pode ser necessário executar `$ make clean` antes da compilação para limpar resultados de compilações anteriores.

Execução:
O arquivo executável será gerado na pasta `output` com o nome `cache_simulator` (`cache_simulator.exe` no Windows).
O executável é portátil e pode ser movido para qualquer diretório para execução.
Para executar este executável execute-o como um executável nativo no sistema operacional de destino com argumentos de linha de comando válidos.

Recursos adicionais:
O simulador tem uma série de recursos adicionais que afetam ou não o comportamento do programa nas entradas válidas de acordo com a especificação.
A disponibilidade desses recursos depende de uma flag de compilação, COMPLIANCE_LEVEL, relativa à compliance com a especificação setada no arquivo CacheSimulator.h
Os valores para esta flag são: 0, relaxado; 1, estrito e 2, muito estrito.
0, relaxado: recursos adicionais podem fazer o programa ter comportamento não compliant com a especificação em casos pontuais improváveis. Por exemplo, arquivos com a extenção .txt são tratados como arquivos de texto.
1, estrito: recursos adicionais só são aplicados se eles não influenciarem no comportamento do programa com entradas válidas de acordo com a especificação. Por exemplo, níveis inferiores de cache tomam mais parâmetros do que definido pela especificação, o que seria inválido, logo isso é aceitável no nível estrito.
2, muito estrito: todos os recursos adicionais são desativados, o programa se comporta exatamente como descrito na especificação, inclusive nos casos inválidos.
A flag de compliance foi setada como 1 por padrão para garantir a funcionalidade adequada do programa nos testes automatizados.

Os recursos adicionais:
- Suporte para arquivos de texto: arquivos com a extensão .txt são tratados como arquivos de texto. Nível de compliance: 0.
Exemplo: cache_simulator 16 2 8 R bin_100.txt

- Suporte para entradas de parâmetros numéricos em bases não decimais: números com sufixo "0x" são tratados como hexadecimais, "0b" são tratados como binários e números com prefixo "0" ou "0o" são tratados como octais no nível de compliance 0, no nível de compliance 1 apenas o prefixo "0o" é aceito para números octais.
Exemplo: cache_simulator 0x10 0b10 0o10 R 01 bin_100.bin

- Suporte para múltiplos níveis de cache: é possível especificar níveis inferiores de cache emendando à linha de comando sequências de configurações de cache no seguinte formato: -l<level> <nsets> <bsize> <assoc> <substituição>. Nível de compliance: 1 ou inferior.
Exemplo: cache_simulator 16 2 8 R 0 bin_100.bin -l2 256 4 1 R -l3 512 8 2 R
