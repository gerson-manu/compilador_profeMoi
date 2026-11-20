# compilador_profeMoi
Holas, este es un repositorio especialmente para nuestro compilador, por parte de José Y Manuel, salu2

## Como se usa
1. Compilar `main.cpp` (necesita C++14 o superior).
2. Ejecutar y escribir el nombre o la ruta de un archivo `.cpp`
3. Si el programa no tiene errores, el codigo ensamblador educativo se guarda en un `.txt` en la misma carpeta del `.cpp`:
   `Prueba1.cpp` -> `Prueba1_ensamblador.txt`
4. Si el programa tiene errores, se dentendrá en la fase donde se presente el error y, no se generará el archivo `.txt`

## Fases
1. Analizador Léxico
2. Analizador Sintáctico
3. Analizador Semántico
4. Código Intermedio
5. Optimización de Código
6. Generación de Código (Ensamblador Educativo)
7. Lectura de archivos .cpp y extracción de Generación de código en archivos .txt

## Adevertencias

Aún no es un compilador muy completo, así que en la lectura de algunas funciones o código se pueden presentar errores, en algún futuro se podrá hacer más implementaciones
