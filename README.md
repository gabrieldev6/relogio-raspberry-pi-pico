# Relógio Digital com Display OLED via I2C

Este código de exemplo mostra como interfacear o Raspberry Pi Pico com um display OLED 128x32 baseado no controlador de display SSD1306, cujo datasheet pode ser encontrado [aqui](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf).

O código exibe um contador de horas, que é incrementado a cada segundo até atingir o limite, então incrementa os minutos, e assim sucessivamente até o ano.

O SSD1306 é operado por meio de uma lista de comandos versáteis (consulte o datasheet) que permitem ao usuário acessar todas as capacidades do controlador. Após enviar um endereço de escravo, os dados que seguem podem ser um comando, flags para acompanhar um comando ou dados a serem escritos diretamente na RAM do display. Um byte de controle é necessário para cada escrita após o endereço do escravo, para que o controlador saiba que tipo de dado está sendo enviado.

## Lista de Arquivos

- **CMakeLists.txt**: Arquivo CMake para incorporar o exemplo na árvore de construção de exemplos.
- **ssd1306_i2c.c**: Todo o fluxo de código.
- **ssd1306_font.h**: Uma fonte simples usada no exemplo.
- **img_to_array.py**: Um auxiliar para converter um arquivo de imagem em um array que pode ser usado no exemplo.

## Lista de Materiais

| Item                           | Quantidade | Detalhes |
|--------------------------------|------------|---------|
| Breadboard                     | 1          | Peça genérica |
| Raspberry Pi Pico               | 1          | [Produto Oficial](https://www.raspberrypi.com/products/raspberry-pi-pico/) |
| Display OLED baseado em SSD1306 | 1          | [Peça da Adafruit](https://www.adafruit.com/product/4440) |
| Push button                     | 2          | Peça genérica |
| Jumpers M/M                     | 4          | Peça genérica |
