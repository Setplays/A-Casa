# Verificação de A Casa — agarramento e QTE

Verificado em 18/11/2026, Linux x86_64, GCC, FreeGLUT e Mesa por software.

- Compilação C11 de todos os fontes com `-Wall -Wextra -Wpedantic`: sem avisos.
- `make test`: passaram as três suítes de lógica (movimento/arma, zumbis e QTE).
- QTE: 12 pressionamentos, tecla segurada contando apenas uma vez, limite de tempo, um captor por vez, rejeição de agarrão através de parede e bloqueio de disparo.
- Sucesso: transição soco/empurrão, deslocamento do zumbi, proteção contra novo agarrão, atordoamento e retorno de rastejante à postura correta.
- Empurrão contra parede: o zumbi para antes de atravessar o collider.
- Falha: mordida, vida zero, queda animada antes da derrota, câmera a 0,24 m e sangue ativo.
- Cancelamento se o captor for removido e limpeza completa do estado ao reiniciar.
- AddressSanitizer e UndefinedBehaviorSanitizer sobre a suíte do QTE: passaram. LeakSanitizer desativado devido à limitação do ambiente de execução.
- Integração com contexto OpenGL: botão de spawn, tecla Z, headshot pelo clique, reset T, movimento bloqueado no agarrão, E reservado ao QTE sem abrir porta, recarga/tiro/mouse bloqueados, pausa sem avançar o cronômetro e sequência de sucesso pelos callbacks reais passaram.
- Renderização de agarrão, soco, empurrão, mordida, queda e derrota: cinco frames por fase, erro OpenGL zero em todas as fases.
- Imagens das fases conferidas visualmente: mãos segurando o zumbi, punho avançando, câmera inclinada durante a queda, sangue na tela e poça no chão.
- Título da janela, HUD, executável e projeto de compilação atualizados para A Casa.

Os testes de entrada invocam os callbacks reais com um contexto gráfico. Não foi feita uma sessão manual completa nem compilação nativa no Windows. O pacote contém fontes, documentação e testes, sem executáveis pré-compilados.

CMake é uma alternativa de compilação fornecida no pacote. Esta verificação usou Make/GCC, com bibliotecas gráficas disponibilizadas localmente para o ambiente de teste.
