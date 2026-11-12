//Código del Jefe y Subjefe - 09 / Noviembre / 2026

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include "Token.h"
#include "Lexer.h"
#include "Nodo.h"
#include "Parser.h"
#include "AnalizadorSemantico.h"

int main() {
    std::string codigo;
    std::string respuesta;

    std::cout << "\t******** BIENVENIDO AL COMPILADOR DEL JEFE Y SUBJEFE *********\n";

    do {
        std::cout << "\nEscriba una cadena de texto que desea analizar: ";
        std::getline(std::cin, codigo);     //aqui el getline sirve para leer la linea completa, y con espacios, si es que tiene

        Lexer lexer(codigo);                //Se crea un Lexer nuevo en cada vuelta para que empiece desde cero y no se sobreescriban
        std::cout << codigo << "\n\n";

        std::vector<Token> listaTokens;
        Token token = lexer.obtenerSiguienteToken();
        while (token.tipo != TipoToken::FIN) {
            std::cout << "Token(" << tipoATexto(token.tipo) << ", \"" << token.valor << "\")";
            if (!token.sugerencia.empty()) {
                std::cout << "   <-- quisiste acaso decir '" << token.sugerencia << "'?";
            }
            std::cout << "\n";
            listaTokens.push_back(token);
            token = lexer.obtenerSiguienteToken();
        }

        std::cout << "\n************* ANALISIS SINTACTICO *******************\n";
        try {
            Parser parser(listaTokens);
            NodoPtr arbol = parser.parsear();
            imprimirArbol(arbol);

            const std::vector<std::string>& errores = parser.obtenerErrores();
            if (errores.empty()) {
                std::cout << "\nSintaxis correcta\n";
            } else {
                std::cout << "\nSe encontraron " << errores.size() << " error(es) sintactico(s):\n";
                for (size_t i = 0; i < errores.size(); i++) {
                    std::cout << "  " << (i + 1) << ". " << errores[i] << "\n";
                }
            }
            if (errores.empty()) {
                std::cout << "\n************* ANALISIS SEMANTICO *******************\n";
                AnalizadorSemantico semantico;
                semantico.analizar(arbol);
                const auto& erroresSemanticos = semantico.obtenerErrores();
                if (erroresSemanticos.empty()) {
                    std::cout << "Analisis semantico correcto.\n";
                    std::cout << "\nCOMPILACION CORRECTA: no se encontraron errores.\n";
                } else {
                    std::cout << "Se encontraron " << erroresSemanticos.size() << " de posibles errores semanticos:\n";
                    for (size_t i = 0; i < erroresSemanticos.size(); ++i)
                        std::cout << "  " << (i + 1) << ". " << erroresSemanticos[i] << "\n";
                    std::cout << "\nCOMPILACION CON ERRORES SEMANTICOS.\n";
                }
            } else {
                std::cout << "\nSe omite el analisis semantico porque hay errores sintacticos.\n";
            }
        } catch (const std::exception& e) {
            std::cout << "Error interno del analizador: " << e.what() << "\n";
        }

        std::cout << "\nDesea analizar otra cadena? (s/n): ";
        std::getline(std::cin, respuesta);

    } while (!respuesta.empty() && respuesta[0] == 's');

    std::cout << "\nFin del programa, gracias Profe Moi :)\n";
    return 0;
}

/*Pruebas

int x = 10; double y = x + 6767676; if(y > x) { cout<<"\nEs mayor"; } esse { cout<<"Es menor"; } intt = x * y;

Aqui debe marcar correcto
int a, b = 2; int arr[3] = {1, 2, 3}; for (int i = 0; i < 3; i++) { a += arr[i] % 2; } while (a > 0 && b != 1 || a == 7) { a--; } cout << a << endl;

Funcion, do whhile, switch
int suma(int p, int q) { return p + q; } int r = suma(1, 2); do { r--; } while (r > 0); switch (r) { case 0: cout << "cero"; break; default: r = 1; }

If, else, else if
int n; cin >> n; if (n > 0) { cout << "pos"; } else if (n < 0) { cout << "neg"; } else { cout << "cero"; }

* Pequeña prueba para el analizador semántico:
int edad = 18; int anios = 2; int resultado = edad + anios; cout << resultado;
int cantidad = 10; double precio = 15.5; bool activo = true; char letra = 'A';
int sumar(int a, int b) { return a + b; } int resultado = sumar(5, 10);
int numeros[3] = {10, 20, 30}; int indice = 1; cout << numeros[indice];
int edad = 18; total = edad + 5;
int edad = 18; int edad = 20;
int edad = "dieciocho";
int resultado = 5 + "hola";
int sumar(int a, int b) { return a + b; } int resultado = sumar(5);
int obtenerEdad() { return "dieciocho"; }

*/

//Salu2
