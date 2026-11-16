#pragma once
// OptimizadorCodigo.h - Optimizacion de codigo

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <cerrno>

#include "CodigoIntermedio.h"

class OptimizadorCodigo {

private:

    int constantesPlegadas = 0;
    int expresionesSimplificadas = 0;

    // Comprueba si una cadena representa un numero
    bool esNumero(const std::string& texto, long double& valor) const {

        if (texto.empty()) return false;

        char* fin = nullptr;
        errno = 0;

        valor = std::strtold(texto.c_str(), &fin);

        return fin != texto.c_str()
            && *fin == '\0'
            && errno != ERANGE
            && std::isfinite(valor);
    }

    // Comprueba si un numero es entero
    bool esEntero(const std::string& texto) const {

        return texto.find('.') == std::string::npos
            && texto.find('e') == std::string::npos
            && texto.find('E') == std::string::npos;
    }

    // Convierte un resultado numerico a texto
    std::string numeroATexto(long double numero, bool decimal) const {

        std::ostringstream salida;

        salida << std::setprecision(15) << numero;

        std::string resultado = salida.str();

        // Conserva una representacion decimal cuando corresponde
        if (decimal &&
            resultado.find('.') == std::string::npos &&
            resultado.find('e') == std::string::npos &&
            resultado.find('E') == std::string::npos) {

            resultado += ".0";
        }

        return resultado;
    }

    // Sustituye una operacion por una asignacion
    void reemplazarPorAsignacion(
        Cuadrupla& q,
        const std::string& valor) {

        q.op = "=";
        q.arg1 = valor;
        q.arg2 = "";

        expresionesSimplificadas++;
    }

    // ---------------- CONSTANT FOLDING ----------------

    void plegarConstantes(Cuadrupla& q) {

        long double a, b, resultado;

        // Operaciones unarias: - y !
        if (q.arg2.empty() &&
            (q.op == "-" || q.op == "!")) {

            if (!esNumero(q.arg1, a)) return;

            if (q.op == "-") {
                resultado = -a;
            }
            else {
                resultado = (a == 0) ? 1 : 0;
            }

            reemplazarPorAsignacion(
                q,
                numeroATexto(resultado, false)
            );

            constantesPlegadas++;
            expresionesSimplificadas--;

            return;
        }

        // Necesitamos dos operandos numericos
        if (!esNumero(q.arg1, a) ||
            !esNumero(q.arg2, b)) {
            return;
        }

        bool decimal = !esEntero(q.arg1) ||
                       !esEntero(q.arg2);

        bool esAritmetica = true;

        // Operaciones aritmeticas
        if (q.op == "+") {
            resultado = a + b;
        }
        else if (q.op == "-") {
            resultado = a - b;
        }
        else if (q.op == "*") {
            resultado = a * b;
        }
        else if (q.op == "/") {

            if (b == 0) return;

            resultado = a / b;

            // Division entera: conserva la semantica de C++
            if (!decimal) {
                resultado = std::trunc(resultado);
            }
            else {
                decimal = true;
            }
        }
        else if (q.op == "%") {

            if (b == 0 || decimal) return;

            resultado = std::fmod(a, b);
        }

        // Operaciones de comparacion
        else if (q.op == "==") {
            resultado = (a == b) ? 1 : 0;
            esAritmetica = false;
        }
        else if (q.op == "!=") {
            resultado = (a != b) ? 1 : 0;
            esAritmetica = false;
        }
        else if (q.op == "<") {
            resultado = (a < b) ? 1 : 0;
            esAritmetica = false;
        }
        else if (q.op == ">") {
            resultado = (a > b) ? 1 : 0;
            esAritmetica = false;
        }
        else if (q.op == "<=") {
            resultado = (a <= b) ? 1 : 0;
            esAritmetica = false;
        }
        else if (q.op == ">=") {
            resultado = (a >= b) ? 1 : 0;
            esAritmetica = false;
        }
        else {
            return;
        }

        // Evita resultados no representables
        if (!std::isfinite(resultado)) return;

        // Evita perder precision al plegar enteros muy grandes
        if (esAritmetica && !decimal &&
            std::fabs(resultado) > 9007199254740991.0L) {
            return;
        }

        std::string valor = numeroATexto(
            resultado,
            esAritmetica && decimal
        );

        q.op = "=";
        q.arg1 = valor;
        q.arg2 = "";

        constantesPlegadas++;
    }

    // ---------------- SIMPLIFICACION ALGEBRAICA ----------------

    void simplificarExpresion(Cuadrupla& q) {

        long double valor;

        // x + 0 = x
        if (q.op == "+" &&
            esNumero(q.arg2, valor) && valor == 0) {

            reemplazarPorAsignacion(q, q.arg1);
        }

        // 0 + x = x
        else if (q.op == "+" &&
                 esNumero(q.arg1, valor) && valor == 0) {

            reemplazarPorAsignacion(q, q.arg2);
        }

        // x - 0 = x
        else if (q.op == "-" &&
                 esNumero(q.arg2, valor) && valor == 0) {

            reemplazarPorAsignacion(q, q.arg1);
        }

        // x * 1 = x
        else if (q.op == "*" &&
                 esNumero(q.arg2, valor) && valor == 1) {

            reemplazarPorAsignacion(q, q.arg1);
        }

        // 1 * x = x
        else if (q.op == "*" &&
                 esNumero(q.arg1, valor) && valor == 1) {

            reemplazarPorAsignacion(q, q.arg2);
        }

        // x / 1 = x
        else if (q.op == "/" &&
                 esNumero(q.arg2, valor) && valor == 1) {

            reemplazarPorAsignacion(q, q.arg1);
        }
    }

    // ---------------- IMPRESION ----------------

    std::string aTexto(const Cuadrupla& q) const {

        if (q.op == "label")
            return q.res + ":";

        if (q.op == "func_begin")
            return "funcion " + q.res + ":";

        if (q.op == "func_end")
            return "fin_funcion " + q.res;

        if (q.op == "param_in")
            return "    recibe " + q.arg1 + " " + q.res;

        if (q.op == "goto")
            return "    goto " + q.res;

        if (q.op == "if")
            return "    if " + q.arg1 + " goto " + q.res;

        if (q.op == "ifFalse")
            return "    ifFalse " + q.arg1 + " goto " + q.res;

        if (q.op == "param")
            return "    param " + q.arg1;

        if (q.op == "call")
            return "    " + q.res + " = call " +
                   q.arg1 + ", " + q.arg2;

        if (q.op == "return")
            return q.arg1.empty()
                ? "    return"
                : "    return " + q.arg1;

        if (q.op == "print")
            return "    print " + q.arg1;

        if (q.op == "read")
            return "    read " + q.res;

        if (q.op == "alloc")
            return "    " + q.res + " = arreglo de " +
                   q.arg1 + "[" + q.arg2 + "]";

        if (q.op == "=")
            return "    " + q.res + " = " + q.arg1;

        if (q.op == "=[]")
            return "    " + q.res + " = " +
                   q.arg1 + "[" + q.arg2 + "]";

        if (q.op == "[]=")
            return "    " + q.arg1 + "[" + q.arg2 +
                   "] = " + q.res;

        if (q.arg2.empty())
            return "    " + q.res + " = " + q.op + q.arg1;

        return "    " + q.res + " = " +
               q.arg1 + " " + q.op + " " + q.arg2;
    }

public:

    // Recibe las cuádruplas y aplica las optimizaciones
    void optimizar(std::vector<Cuadrupla>& codigo) {

        constantesPlegadas = 0;
        expresionesSimplificadas = 0;

        for (size_t i = 0; i < codigo.size(); i++) {

            // Primero intenta resolver constantes
            int anteriores = constantesPlegadas;

            plegarConstantes(codigo[i]);

            // Si no hubo plegado, intenta simplificar
            if (constantesPlegadas == anteriores) {
                simplificarExpresion(codigo[i]);
            }
        }
    }

    // Imprime el codigo optimizado
    void imprimirCodigo(
        const std::vector<Cuadrupla>& codigo) const {

        for (const auto& q : codigo) {
            std::cout << aTexto(q) << "\n";
        }
    }

    // Imprime las cuadruplas optimizadas
    void imprimirCuadruplas(
        const std::vector<Cuadrupla>& codigo) const {

        std::cout << std::left
                  << std::setw(5) << "#"
                  << std::setw(13) << "Operador"
                  << std::setw(14) << "Arg1"
                  << std::setw(14) << "Arg2"
                  << "Resultado\n";

        std::cout << std::string(60, '-') << "\n";

        for (size_t i = 0; i < codigo.size(); i++) {

            const auto& q = codigo[i];

            std::cout << std::setw(5) << i
                      << std::setw(13) << q.op
                      << std::setw(14)
                      << (q.arg1.empty() ? "-" : q.arg1)
                      << std::setw(14)
                      << (q.arg2.empty() ? "-" : q.arg2)
                      << (q.res.empty() ? "-" : q.res)
                      << "\n";
        }
    }

    // Muestra las optimizaciones realizadas
    void imprimirResumen() const {

        std::cout << "\n******** RESUMEN DE OPTIMIZACION ********\n";

        std::cout << "Constantes plegadas: "
                  << constantesPlegadas << "\n";

        std::cout << "Expresiones simplificadas: "
                  << expresionesSimplificadas << "\n";
    }
};
