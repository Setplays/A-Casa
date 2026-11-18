# A Casa

Pequeno FPS acadêmico em C11 com OpenGL clássico, GLU e FreeGLUT. O mapa contém a casa principal, jardim, floresta e três cabanas com interiores acessíveis. Os personagens e efeitos usam geometria simples, sem modelos ou texturas externas.

## Atualizar e executar no Windows

Extraia `A_Casa.zip`. A pasta `A_Casa` contém `Makefile`, `README.md` e `src`. Abra **MSYS2 UCRT64** e, se extraiu em Downloads, execute:

```sh
cd /c/Users/victor.AHERJ/Downloads/A_Casa
make
./a_casa.exe
```

Você também pode copiar o conteúdo da nova pasta para a pasta antiga do projeto, substituindo os fontes e o Makefile. Nesse caso, o comando continua usando seu caminho anterior:

```sh
cd /c/Users/victor.AHERJ/Downloads/FPS_DOOM_C
make
./a_casa.exe
```

O executável desta versão é **`a_casa.exe`**. Abrir o executável da versão anterior não aplica as atualizações. Este pacote contém fontes e testes; não inclui um `.exe` Windows pré-compilado.

Para uma instalação inicial, instale [MSYS2](https://www.msys2.org/), abra **MSYS2 UCRT64**, atualize com `pacman -Syu` (reabra o terminal se solicitado) e instale:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut make
```

Depois execute os comandos de compilação acima. O terminal UCRT64 encontra as DLLs necessárias; para abrir pelo Explorer ou distribuir o jogo, as DLLs de runtime da mesma arquitetura precisam estar acessíveis.

Compilação direta, sem Make:

```sh
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Isrc src/main.c src/zombie_render.c src/grapple_render.c src/game.c src/world.c src/zombies.c src/grapple.c -o a_casa.exe -lfreeglut -lopengl32 -lglu32 -lm
```

## Agarrão e quick time event

Quando um zumbi ativo chega a até 1,15 m, com passagem livre e o jogador no chão, ele inicia o agarrão. A câmera se volta para ele, seus braços avançam e as mãos do jogador aparecem segurando-o. Zumbis rastejantes também podem agarrar: erguem o tronco durante a luta, preservando o estado das pernas.

Após uma aproximação de 0,45 s, começa o desafio:

- Aperte e solte **E doze vezes em quatro segundos**.
- A barra amarela indica o esforço; a barra vermelha indica o tempo restante.
- Segurar E não conta como vários apertos. Pressões durante a aproximação ainda não somam esforço.
- Durante a luta, movimento, mouse, pulo, arma, recarga e interação com portas ficam bloqueados. E é reservado ao desafio.
- Esc pausa a luta e o cronômetro. Para continuar, pressione Esc ou clique fora do botão de spawn.
- Outros zumbis e novos spawns aguardam o desfecho do encontro, evitando agarrões simultâneos.

**Sucesso:** o jogador dá um soco, estende os braços e empurra o zumbi. O empurrão respeita paredes e outros inimigos; ele pode parar antes de percorrer toda a distância se houver obstáculo. O zumbi fica brevemente atordoado, e há uma proteção de 4 s contra um novo agarrão. Rastejantes voltam a rastejar. O soco serve para abrir espaço; a regra de três tiros no corpo continua contando apenas os disparos.

**Falha:** o zumbi avança a cabeça e morde. A vida vai a zero; a câmera cai e gira até ficar a aproximadamente 24 cm do chão. Há respingos e escorrimentos estilizados na tela, uma poça no chão e, após a animação, a tela de derrota. Pressione **T** para reiniciar.

O agarrão é o ataque de contato desta versão. Para vê-lo em jogo, ative os spawns com Z, espere um zumbi sair da terra e deixe-o se aproximar.

## Arma, dano e zumbis

| Região atingida | Resultado |
|---|---|
| Cabeça | Morte com um tiro, inclusive rastejando ou saindo do chão |
| Tronco, braços ou pernas | Morte no terceiro acerto acumulado fora da cabeça |
| Pernas | Primeiro tiro já conta como um acerto e inicia queda e rastejamento permanente |
| Pernas durante a saída da terra | O zumbi termina de emergir, cai e passa a rastejar |

O tiro é hitscan e atinge somente a primeira região visível do primeiro zumbi no caminho. Paredes, portas, móveis, troncos, chão e telhados bloqueiam o raio. As regiões de acerto acompanham as mesmas peças e transformações das poses desenhadas. Folhagem decorativa não bloqueia tiros.

A pistola tem 12 balas por carregador, 48 de reserva inicial, cadência de 0,23 s e recarga manual de 1,6 s. Cada clique dispara uma vez. Há uma caixa de 24 balas em cada construção, coletável uma única vez.

O spawn começa desligado. Use Z ou, após pausar com Esc, o botão no canto superior direito. Desligar os spawns impede novos inimigos, mantendo os existentes. O primeiro tenta surgir após 0,4 s; os próximos, a cada 3,2 s. Há 32 slots, compartilhados com cadáveres ainda visíveis.

Os pontos são sorteados entre 8 e 23 m do jogador, em solo externo livre e com caminho alcançável. O zumbi leva 3 s para emergir, com braços cavando, subida gradual, monte de terra, rachaduras e partículas. Apenas as partes já expostas podem ser atingidas. Depois de sair, persegue o jogador com navegação em grade e desvio de obstáculos. Caminha a 1,5 m/s ou rasteja a 0,62 m/s. Não abre portas.

Cadáveres ficam por alguns segundos e afundam; o slot é liberado após 6 s. T reinicia mapa, vida, arma, inimigos, QTE e sangue. O spawn volta a ficar desligado.

## Controles

| Entrada | Ação |
|---|---|
| WASD | Mover |
| Mouse | Olhar |
| Clique esquerdo | Atirar; durante a pausa, clicar no botão de spawn ou retomar |
| R | Recarregar |
| Espaço | Pular |
| E | Abrir/fechar a porta próxima; durante o agarrão, apertar repetidamente para escapar |
| Z | Ativar/desativar novos spawns |
| T | Reiniciar a partida |
| C | Reiniciar animação da chaminé |
| F | Emitir fumaça |
| H | Mostrar/ocultar ajuda |
| Esc | Pausar/liberar mouse ou retomar |
| Q / fechar janela | Sair |

## Linux — Debian/Ubuntu

```sh
sudo apt update
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev
make
./a_casa
```

Requer ambiente gráfico e suporte a OpenGL clássico. O protótipo usa modo imediato e GLU; não solicite um contexto exclusivo de perfil core.

## Testes

Testes da lógica sem depender de janela ou bibliotecas OpenGL:

```sh
make test
```

Incluem munição, movimento, dano regional, navegação, spawn e os dois desfechos do QTE. O desafio tem testes de tecla segurada, exclusividade do captor, paredes, empurrão, proteção após escapar, rastejante, câmera no chão e reset.

Teste dos callbacks reais e da renderização, com dependências gráficas e um display disponível:

```sh
make test-integration
```

Criação de contexto e cinco frames:

```sh
./a_casa --smoke-test
./a_casa --smoke-test --enemy-render-test
./a_casa --smoke-test --qte-preview=grab
```

Cenas estáticas para conferir as animações por fase:

```sh
./a_casa --qte-preview=grab
./a_casa --qte-preview=punch
./a_casa --qte-preview=push
./a_casa --qte-preview=bite
./a_casa --qte-preview=fall
./a_casa --qte-preview=dead
```

Estas prévias são estáticas. Para jogar, execute normalmente sem argumentos ou pressione T para iniciar a partida.

## CMake (alternativo)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

O executável é `a_casa`, com extensão `.exe` no Windows, na pasta de saída do gerador. Os testes do CTest não precisam de display; `test_integration` deve ser executado separadamente com display.

Para compilar apenas os testes de lógica:

```sh
cmake -S . -B build-tests -DBUILD_GAME=OFF
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

## Organização

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Janela, entrada, câmera, HUD e simulação integrada |
| `src/game.h` / `src/game.c` | Movimento, colisões, arma, portas e coleta |
| `src/world.c` | Construções, jardim, floresta, móveis e caminhos |
| `src/zombies.h` / `src/zombies.c` | Estados, dano, hitboxes, disparos, spawn e navegação |
| `src/zombie_render.h` / `src/zombie_render.c` | Desenho dos zumbis e efeitos de terra |
| `src/grapple.h` / `src/grapple.c` | Agarramento, entradas do QTE, sucesso, falha, empurrão e câmera |
| `src/grapple_render.h` / `src/grapple_render.c` | Mãos em primeira pessoa, barras, sangue e poça |
| `tests/test_game.c` | Lógica original |
| `tests/test_zombies.c` | Combate e navegação |
| `tests/test_grapple.c` | QTE e desfechos |
| `tests/test_integration.c` | Eventos e renderização com contexto OpenGL |

Ajuste `QTE_TAPS` e `QTE_SECONDS` em `grapple.h` para mudar a dificuldade. Outros tempos das fases estão no mesmo arquivo.

As animações são procedurais com blocos, adequadas ao escopo pequeno da apresentação. As colisões são horizontais com chão e teto simples: não há escadas, andares, terreno irregular ou animação esquelética. A navegação e o corpo rastejante usam aproximações que podem limitar o deslocamento em espaços apertados.

Referências de configuração: [FreeGLUT](https://freeglut.sourceforge.net/docs/api.php), [MSYS2](https://www.msys2.org/docs/environments/) e [pacote FreeGLUT UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-freeglut).
