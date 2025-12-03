⚠️ __ATENÇÃO__: Os computadores da UNIVALI não permitem a instalação dos drivers do ESP8266 nem a conexão do NodeRED e VS Code por causa que é necessário permissões de admin do Firewall.

O ESP-12E __NÃO__ tem suporte para o protocolo __WPA2-Enterprise com PEAP-MSCHAPv2__ utilizado pela UNIVALI, mas é possível contornar abrindo um hotspot no seu notebook ou smartphone.

Também __NÃO__ há suporte há frequência 5GHz, sendo obrigatória a utilização do 2.4GHz.⚠️

![](https://i.imgur.com/eHSMRzq.png)
![](https://i.imgur.com/SoaJFyy.jpeg)

Insira o nome e senha do WIFI hotspot/roteador no arquivo `env.h`, que é explicado em [Configurando credenciais (env.h)](#configurando-credenciais-envh)

# Instalação

1. Baixe o repositório em [zip](https://github.com/AirtonBorges/Faculdade.IoT.Mqtt/archive/refs/heads/main.zip) e descompacte / ou clone com GIT:
```bash
git clone https://github.com/AirtonBorges/Faculdade.IoT.Mqtt.git
```
2. [Baixe e instale o VS code](https://code.visualstudio.com/download)
3. Abra a pasta do projeto no VS code
4. Marque a caixa e o botão __Sim, confio nos autores__

![](https://i.imgur.com/mPUyMoG.png)

5. Aparecerá uma pequena janela no canto inferior direito. Instale as extensões recomendadas do __PlatformIO__ e __C/C++__

![](https://i.imgur.com/lVE597o.png)

6. Clique em __Confiar em publicador & Instalar__
7. Aguarde o PlatformIO instalar (demora alguns minutos e você também pode fechar as abas de __bem-vindo__, __release notes do PlatformIO__ e as __duas páginas de extensões abertas__)
8. Após finalizar, reinicie o VS Code. Você irá observar que o PlatformIO automaticamente começará a instalar todas as dependências necessárias automaticamente de acordo com o arquivo `platformio.ini`. Aguarde mais alguns minutos para ele configurar o projeto (você pode fechar a aba __PIO HOME__)
9. Com a configuração finalizada, no inferior da tela do VS Code clique em `Default (Faculdade.IoT.Mqtt-main)` e escolha `env:dht11_mqtt`. Esse é o projeto da apresentação final. Se quiser testar outros projetos, é só clicar em outra opção (exceto Default, que irá buildar todos de uma vez só)

![](https://i.imgur.com/5c7lSmZ.png)

10. Continue em [Configurando credenciais (env.h)](#configurando-credenciais-envh) logo abaixo:

# Configurando credenciais (env.h)

10.1. Na pasta `include`, copie o arquivo `env.h.example` e renomeie ele para `env.h`.

Se tudo ocorrer bem, o resultado ficará assim:

![](https://i.imgur.com/GRhMkqc.png)

10.2. Edite o seu `env.h` com seu SSID, senha do seu WIFI e configurações MQTT.

10.3. O arquivo `env.h` já está listado em `.gitignore`, portanto não será enviado ao repositório.

10.4. Se você acidentalmente comitou `env.h`, remova-o do índice e crie um commit:

```powershell
git rm --cached include/env.h
git commit -m "Remover env.h com credenciais"
```

O arquivo de exemplo `include/env.h.example` está mantido no repositório para que outros usuários possam copiar e configurar localmente. __Mas não o exclua do repositório se você pretende desenvolver e fazer commits, pois senão ele será deletado do remoto__.

# Instalando o driver
O Windows automaticamente vai instalar. Mas se for necessário baixe e instale o driver do chip CH340 [Aqui](https://www.usinainfo.com.br/index.php?controller=attachment&id_attachment=452)

# Pós env.h

11. Conecte os componentes na sua protoboard/breadboard de acordo com o diagrama:

![](https://i.imgur.com/kbVA6OZ.jpeg)

Se seu ESP12E não foi reconhecido, [Instale o driver manualmente](#instalando-o-driver)
12. Após configurar o `env.h`, clique no ícone do PlatformIO (que é uma cabeça de alienígena) na barra lateral esquerda do VS Code:

![](https://i.imgur.com/zu3qJMV.png)

13. Conecte o USB e clique em `Upload and Monitor` - Isso irá buildar, enviar o código para o chip e monitorar no terminal. _Automaticamente o PlatformIO irá detectar a porta serial.__

![](https://i.imgur.com/zG35YGN.png)

14. Observe o log terminal do VS Code para fazer o debug

![](https://i.imgur.com/GwukZJ3.png)

# Observações
1. Após o primeiro build, será muito mais rápido fazer alterações no código, já que não é necessário buildar todas as outras vezes. Sempre clique em "Upload" quando fizer alguma alteração.
2. Encerre o terminal que está monitorando antes de fazer outro upload, senão acontecerá um erro de __acesso negado__.

![](https://i.imgur.com/ZOJvdco.png)

__FIM!__ 🎉🎉

# Comandos úteis

```shell
# Somente 'buildar' o projeto
$ pio run

# Enviar o programa para a placa (upload)
$ pio run --target upload

# Monitorar a placa (a localidade deve estar na pasta do projeto)
$ pio run --target monitor

# Build um ambiente específico
$ pio run -e nodemcuv2

# Enviar o programa para a placa em um ambiente específico
$ pio run -e nodemcuv2 --target upload

# Limpar arquivos de build
$ pio run --target clean
```

# Outros projetos

## Projeto "dht_webserver"

Este projeto roda em um ESP8266 lendo um sensor DHT11 e oferecendo:
- Interface web em LittleFS (UI em `/index.html` e `/config.html`).
- Configuração de Wi‑Fi via AP (rede `ESP-DHT11-Setup`) e formulário em `/config.html`.

## Como usar

1. Instale o PlatformIO e abra este projeto.
2. Carregue a imagem do sistema de arquivos

```powershell
pio run -t uploadfs --environment dht11_webserver
```

3. Compile e envie o firmware para o ESP8266:

```powershell
pio run -t upload --environment dht11_webserver
```

4. Conecte seu computador/telefone à rede Wi‑Fi criada pelo ESP: `ESP-DHT11-Setup`.
5. Acesse `http://192.168.4.1/` e abra a página de configuração. No formulário, informe o `SSID` e `Senha` e clique em `Salvar e conectar`.

## Monitoramento
```powershell
pio device monitor --environment dht11_webserver
```

## Comportamento após enviar credenciais

- O ESP tenta conectar à rede fornecida. Após a conexão, encontre o novo IP do dispositivo conectado à sua rede.
- O servidor responde imediatamente com a página de leitura (`/index.html`) ou com um JSON contendo a leitura atual (`/read`) para que o usuário veja temperatura e umidade sem precisar reiniciar.

## Resetar Wi‑Fi / voltar ao modo de configuração

- A página `/index.html` tem um botão `Escolher outro Wi‑Fi` que faz um POST em `/clear` — isso desconecta o STA e reativa o modo AP para nova configuração.

## Endpoints úteis

- `/` ou `/index.html` — interface de leitura (temperatura/umidade).
- `/config.html` — interface de configuração Wi‑Fi (quando em AP).
- `/read` — retorna JSON com `{ "temp": <C>, "hum": <%> }`.
- `/mqtt/publish` — publica uma leitura manualmente (form POST com `topic`).
- `/fs` e `/fs/*` — gerência de arquivos em LittleFS (download/upload/delete/list).


