ESP8266 & DHT11 com MQTT
=====================================

1. [Instale o PlatformIO Core](https://docs.platformio.org/page/core.html)
2. Execute esses comandos:

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

Retire no nome do arquivo o '.disabled' para que o PlatformIO o detecte e envie para o seu dispositivo (mantenha somente 1 arquivo .cpp na pasta para testar os outros projetos)

No ícone do PlatformIO, clique na tarefa "Upload and Monitor" para além de 'buildar' e enviar seu código, também monitorá-lo.