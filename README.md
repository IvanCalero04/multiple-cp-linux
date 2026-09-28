# mcp — multi-copy

Binario de Linux (escrito en C) que extiende la idea de `cp` para copiar
**un archivo en varios destinos** de una sola vez.

## Compilar

```sh
make
```

Esto genera el binario `./mcp`. También puedes compilarlo a mano:

```sh
cc -O2 -Wall -Wextra -o mcp mcp.c
```

## Uso

```
mcp [OPCIONES] FUENTE DESTINO...
```

El primer argumento es la **fuente** y el resto son los **destinos**. Si un
destino es un directorio, la fuente se copia dentro de él conservando su nombre.

### Ejemplo

```sh
# Crea 5 copias de text.txt con los nombres test1.txt .. test5.txt
mcp text.txt ./test/test{1..5}.txt
```

El shell expande la llave `{1..5}` antes de invocar el programa, de modo que la
línea anterior equivale a:

```sh
mcp text.txt ./test/test1.txt ./test/test2.txt ./test/test3.txt ./test/test4.txt ./test/test5.txt
```

Otros ejemplos:

```sh
# Copiar en varios nombres sueltos y además en un directorio
mcp informe.pdf copia1.pdf copia2.pdf respaldo/

# Copiar desde stdin
cat text.txt | mcp - out1.txt out2.txt
```

## Opciones

| Opción        | Descripción                                                |
|---------------|------------------------------------------------------------|
| `-v, --verbose`  | Imprime cada copia realizada.                            |
| `-p, --preserve` | Preserva permisos y fechas de la fuente.                 |
| `-f, --force`    | Si el destino no se puede abrir, lo borra y reintenta.   |
| `-h, --help`     | Muestra la ayuda.                                        |
| `-V, --version`  | Muestra la versión.                                      |

El programa continúa copiando el resto de destinos aunque alguno falle y, si
hubo algún error, termina con código de salida `1`.

## Instalar

```sh
sudo make install      # instala en /usr/local/bin
sudo make uninstall
```

Para instalarlo en otro lugar:

```sh
make install PREFIX=~/.local
```
