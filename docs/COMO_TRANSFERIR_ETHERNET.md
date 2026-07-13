# Guia de Transferência do Jogo via Ethernet (TCP) para a DE1-SoC

Devido ao tamanho do binário compilado (~3.5MB) e problemas comuns de corrompimento ao usar transferência via cabo Serial/UART, o método oficial para transferir o jogo para a placa FPGA DE1-SoC é via **Cabo de Rede (Ethernet)** utilizando o protocolo TCP.

Abaixo está o passo a passo completo de como realizar essa transferência do PC Host (Windows/Linux) para o Linux Embarcado da DE1-SoC.

---

## 1. Preparação da Placa (Receptor)

A DE1-SoC precisa estar conectada na rede e possuir um IP acessível. Nos nossos testes práticos, assumimos que a placa obteve ou foi configurada com o IP estático `164.41.179.80`.

1. Conecte-se ao terminal da placa via PuTTY (Serial COM).
2. Para preparar a placa para receber o arquivo, vamos abrir a porta `12345` (ou outra de sua escolha) usando a ferramenta nativa `nc` (Netcat). 
3. O comando abaixo escuta (`-l`) a porta e redireciona todo o tráfego recebido para um arquivo binário local:

```bash
nc -l 12345 > jogo_metalslug_novo
```

> **Atenção:** Assim que rodar esse comando, o terminal **deve ficar travado/piscando**. Isso significa que ele está escutando. Se ele pular de linha e voltar para o prompt (`root@de1soclinux:~#`) imediatamente, a porta está ocupada/travada. Mude o número da porta (ex: `12346`) e tente novamente.

---

## 2. Envio pelo Computador (Remetente)

Com a placa aguardando, agora vamos disparar o arquivo compilado a partir do seu PC. Em vez de lidar com firewalls complexos ou instalar o WinSCP, o método mais rápido é usar um script Python nativo (o Python costuma vir pré-instalado em muitos ambientes ou pelo MSYS2).

No terminal do seu PC, **dentro da pasta raiz do projeto** (onde fica a pasta `build_arm`), rode o seguinte comando em uma única linha:

```bash
python -c "import socket; sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM); sock.connect(('164.41.179.80', 12345)); f = open('build_arm/metalslug', 'rb'); sock.sendall(f.read()); f.close(); sock.close(); print('Transferência Concluída!')"
```

*Nota: Se você escolheu a porta `12346` no Passo 1, lembre-se de mudar de `12345` para `12346` neste comando.*

---

## 3. Validação e Execução

Se a transferência ocorrer com sucesso, o script Python imprimirá `Transferência Concluída!` e o terminal da placa DE1-SoC vai se **destravar automaticamente**, voltando para o prompt de comando (`root@...`).

Agora, basta dar permissão de execução e rodar o seu jogo!

1. Verifique se o arquivo chegou inteiro (deve ter um tamanho maior que `0` bytes, idealmente ~3.5MB):
   ```bash
   ls -l jogo_metalslug_novo
   ```

2. Dê a permissão de execução:
   ```bash
   chmod +x jogo_metalslug_novo
   ```

3. Inicie o jogo:
   ```bash
   ./jogo_metalslug_novo
   ```

### Resumo de Resolução de Problemas
- **Arquivo com 0 Bytes:** A transferência falhou, provavelmente porque a porta estava ocupada ou o Python acusou erro de conexão (Connection Refused). Use uma porta TCP diferente e garanta que o `nc` ficou travado esperando.
- **Python - Connection Refused:** O IP está errado, o cabo de rede está solto, ou você tentou enviar o arquivo **antes** de colocar o `nc` para rodar na placa. Sempre rode o comando da placa primeiro.
- **Segmentation Fault / Erro imediato:** Se o arquivo não tiver 0 bytes mas der erro instantâneo, você pode ter transferido um binário compilado incorretamente (ex: `.sof` no lugar do `metalslug`, ou esqueceu o `chmod +x`).
