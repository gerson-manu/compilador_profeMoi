#pragma once
//Lexer.h - Analizador léxico

#include <string>
#include <set>
#include <unordered_set>   //Usaremos esta libreria para guardar las palabras reservadas
#include <cctype>
#include "Token.h"

/*LEXER
* Identificadores con nombres , que empeizan por "a-z"
* Reconocimiento de tokens como números enteros positivos, ident básicos simples, y operadores
* Tiene un manejo básico de errores
* Identifica las palabras reservadas y detecta si están mal escritas (esse, intt...)
* Maneja decimales, cadenas, caracteres y los espacios en blanco (token ESPACIO)
* NUEVO: reconoce && || % ++ -- += -= *= /= %= << >> y los dos puntos ':'
* NUEVO: reconoce cout, cin y endl
* NUEVO: reconoce listas de declaración como "int a, b;"
*/

class Lexer { //Entonces
private:
    //Lista de palabras reservadas
    const std::set<std::string> reservadas = {
        "if", "else", "while", "for", "do", "return", "int", "float", "double", "char", "void", "bool",
        "break", "continue", "true", "false", "switch", "case", "default"
    };

    //Palabras de la librería de entrada/salida. Son reservadas, pero NO se revisan por parecido,
    //porque "cin" se parecería a nombres normales como "min", "sin" o "bin"
    const std::unordered_set<std::string> biblioteca = { "cout", "cin", "endl" };

    //Devuelve TRUE si la palabra está en la lista de reservadas, sino, un FALSE
    bool esReservada(const std::string& palabra) {
        return reservadas.count(palabra) > 0 || biblioteca.count(palabra) > 0;
    }

    //Devuelve TRUE si las dos palabras se diferencian por alguna edición, principalmete en las reservadas
    //una letra de más, una letra de menos o una letra distinta, compara las dos palabras
    bool aUnaEdicion(const std::string& a, const std::string& b) {
        int dif = (int)a.length() - (int)b.length();
        if (dif > 1 || dif < -1) return false;   //Si el tamaño difiere en más de 1, ya no se parecen

        size_t i = 0, j = 0;
        int cambios = 0;

        while (i < a.length() && j < b.length()) {
            if (a[i] == b[j]) {
                i++;
                j++;
            } else {
                cambios++;
                if (cambios > 1) return false;

                if (a.length() > b.length()) i++;        //"a" tiene una letra de más
                else if (a.length() < b.length()) j++;   //"b" tiene una letra de más
                else { i++; j++; }                       //Misma longitud, una letra distinta
            }
        }

        //Si sobra una letra al final de alguna palabra, también cuenta como un cambio
        cambios += (a.length() - i) + (b.length() - j);
        return cambios == 1;
    }

    //Si la palabra se parece a una reservada, devuelve cuál es
    //Si no se parece a ninguna, devuelve una cadena vacía
    std::string reservadaParecida(const std::string& palabra) {
        if (palabra.length() < 3) return "";   //Las palabras muy cortas como a, x ó i no se revisan

        for (const std::string& r : reservadas) {
            if (r.length() >= 3 && aUnaEdicion(palabra, r)) {
                return r;
            }
        }
        return "";
    }

    std::string codigo;
    size_t pos;
    char caracterActual;

    void avanzar() {
        pos++;
        if (pos < codigo.length()) {
            caracterActual = codigo[pos];
        } else {
            caracterActual = '\0'; //Representa el fin de cadena (EOF)
        }
    }

    //Aquí, se "asomará" para ver si hay un punto después de un entero, para identificarlo como un decimal
    char verSiguiente() {
        if (pos + 1 < codigo.length()) {
            return codigo[pos + 1];
        }
        return '\0';
    }

    void saltarEspacios() {
        while (caracterActual != '\0' && std::isspace(caracterActual)) {
            avanzar();
        }
    }

    Token obtenerNumero() {
        std::string num = "";

        //Parte entera, lee todos los dígitos seguidos
        while (caracterActual != '\0' && std::isdigit(caracterActual)) {
            num += caracterActual;
            avanzar();
        }

        //Si hay un punto Y después del punto viene un dígito, es un decimal, sino hay un número después del punto,
        //lo agarrará como ERROR
        if (caracterActual == '.' && std::isdigit(verSiguiente())) {
            num += caracterActual;   //Agrega el punto al número
            avanzar();

            //Parte decimal, lee los dígitos después del punto
            while (caracterActual != '\0' && std::isdigit(caracterActual)) {
                num += caracterActual;
                avanzar();
            }
            return Token{TipoToken::NUMERO_DECIMAL, num};
        }

        //Si no hubo punto con dígito, es un entero normal
        return Token{TipoToken::NUMERO, num};
    }

    Token obtenerIdentificador() {
        std::string ident = "";
        while (caracterActual != '\0' && (std::isalnum(static_cast<unsigned char>(caracterActual)) || caracterActual == '_')) {
            ident += caracterActual;
            avanzar();
        }

        if (esReservada(ident)) {
            return Token{TipoToken::PALABRA_RESERVADA, ident, ""};
        }

        //El lexer reconoce cualquier nombre valido como identificador.
        //El analizador semantico determinara si fue declarado o no.
        std::string parecida = reservadaParecida(ident);
        if (!parecida.empty()) {
            return Token{TipoToken::ERROR, ident, parecida};
        }
        return Token{TipoToken::IDENTIFICADOR, ident, ""};
    }

    //Lee una cadena de texto entre comillas dobles, Se llama cuando caracterActual es la comilla de apertura.
    Token obtenerCadena() {
        std::string texto = "";
        avanzar();   //Salta la comilla de apertura

        //Lee hasta encontrar la comilla de cierre
        while (caracterActual != '\0' && caracterActual != '"' && caracterActual != '\n') {
            texto += caracterActual;
            avanzar();
        }

        //Si se llegó a la comilla de cierre, la cadena es válida
        if (caracterActual == '"') {
            avanzar();   //Salta la comilla de cierre
            return Token{TipoToken::CADENA, texto};
        }

        //Si se acabó el código o hubo salto de línea sin cerrar, se guarda como error
        return Token{TipoToken::ERROR, "\"" + texto};
    }

    //Lee un carácter entre comillas simples, se llama cuando caracterActual es la comilla simple de apertura.
    Token obtenerCaracter() {
        std::string texto = "";
        avanzar();   //Salta la comilla de apertura

        //Lee el contenido, un carácter, o dos si es secuencia de escape
        if (caracterActual != '\0' && caracterActual != '\n' && caracterActual != '\'') {
            if (caracterActual == '\\') {
                texto += caracterActual;
                avanzar();
                if (caracterActual != '\0' && caracterActual != '\n') {
                    texto += caracterActual;
                    avanzar();
                }
            } else {
                texto += caracterActual;
                avanzar();
            }
        }

        //Debe venir la comilla de cierre
        if (caracterActual == '\'') {
            avanzar();   //Salta la comilla de cierre
            if (!texto.empty()) {
                return Token{TipoToken::CARACTER, texto};
            }
            return Token{TipoToken::ERROR, "''"};   //Comillas vacías, marcará un ERROR
        }

        //No se cerró la comilla, marcará otro ERROR
        return Token{TipoToken::ERROR, "'" + texto};
    }

    //Lee un operador que puede ir solo o seguido de '=' (como < o <=)
    //Recibe el símbolo, el tipo si lleva '=' después, y el tipo si va solo.
    Token leerOperadorConIgual(char simbolo, TipoToken conIgual, TipoToken sinIgual) {
        std::string texto(1, simbolo);   //Empieza con el primer símbolo (<)
        avanzar();                       //Consume el primer símbolo

        //Si lo que sigue es un '=', es el operador de dos caracteres (<=)
        if (caracterActual == '=') {
            texto += '=';
            avanzar();
            return Token{conIgual, texto};
        }

        //Si no, es el operador de un solo carácter
        return Token{sinIgual, texto};
    }

    //Consume dos caracteres seguidos y devuelve un solo token (&&, ||, <<, >>)
    Token operadorDoble(TipoToken tipo, const std::string& texto) {
        avanzar();
        avanzar();
        return Token{tipo, texto};
    }

    //Operadores aritméticos: + - * / %
    //Pueden ser solos (+), dobles (++ --) o compuestos con '=' (+= -= *= /= %=)
    Token leerOperadorAritmetico(char simbolo, TipoToken simple) {
        char sig = verSiguiente();
        std::string s(1, simbolo);

        if ((simbolo == '+' || simbolo == '-') && sig == simbolo) {
            return operadorDoble(simbolo == '+' ? TipoToken::INCREMENTO : TipoToken::DECREMENTO, s + s);
        }
        if (sig == '=') {
            return operadorDoble(TipoToken::ASIGNACION_COMPUESTA, s + "=");
        }
        avanzar();
        return Token{simple, s};
    }

public:
    Lexer(const std::string& fuente) : codigo(fuente), pos(0) {
        caracterActual = codigo.empty() ? '\0' : codigo[0];
    }

    //Entrega el siguiente token sin intentar resolver declaraciones ni ambitos.
    Token obtenerSiguienteToken() {
        return leerToken();
    }

    Token leerToken() {
        while (caracterActual != '\0') {

            //Los espacios ahora se devuelven como token, uno por cada espacio
            if (std::isspace(caracterActual)) {
                char espacio = caracterActual;
                avanzar();
                //Si es un tabulador se muestra como \t, para que se vea en pantalla
                return Token{TipoToken::ESPACIO, (espacio == '\t') ? "\\t" : std::string(1, espacio)};
            }

            if (std::isdigit(caracterActual)) {
                return obtenerNumero();
            }

            if (std::isalpha(caracterActual) || caracterActual == '_') {
                return obtenerIdentificador();
            }

            //Cadenas de texto, deben empezar con comillas dobles
            if (caracterActual == '"') {
                return obtenerCadena();
            }

            //Caracteres que tienen comillas simples
            if (caracterActual == '\'') {
                return obtenerCaracter();
            }

            //Operadores aritméticos (+ - * / %), junto con ++ -- += -= *= /= %=
            if (caracterActual == '+') return leerOperadorAritmetico('+', TipoToken::SUMA);
            if (caracterActual == '-') return leerOperadorAritmetico('-', TipoToken::RESTA);
            if (caracterActual == '*') return leerOperadorAritmetico('*', TipoToken::MULT);
            if (caracterActual == '/') return leerOperadorAritmetico('/', TipoToken::DIV);
            if (caracterActual == '%') return leerOperadorAritmetico('%', TipoToken::MODULO);

            //Operadores de dos caracteres que se revisan ANTES que < > (porque << empieza igual que <)
            if (caracterActual == '<' && verSiguiente() == '<') return operadorDoble(TipoToken::DESPLAZ_IZQ, "<<");
            if (caracterActual == '>' && verSiguiente() == '>') return operadorDoble(TipoToken::DESPLAZ_DER, ">>");
            if (caracterActual == '&' && verSiguiente() == '&') return operadorDoble(TipoToken::AND, "&&");
            if (caracterActual == '|' && verSiguiente() == '|') return operadorDoble(TipoToken::OR, "||");

            //Operadores que pueden llevar un '=' después
            if (caracterActual == '=') { return leerOperadorConIgual('=', TipoToken::IGUAL_IGUAL, TipoToken::ASIGNACION); }
            if (caracterActual == '!') { return leerOperadorConIgual('!', TipoToken::DIFERENTE_DE, TipoToken::NEGACION); }
            if (caracterActual == '<') { return leerOperadorConIgual('<', TipoToken::MENOR_IGUAL, TipoToken::MENOR); }
            if (caracterActual == '>') { return leerOperadorConIgual('>', TipoToken::MAYOR_IGUAL, TipoToken::MAYOR); }

            //Símbolos y signos de puntuación
            if (caracterActual == ';') { avanzar(); return Token{TipoToken::PUNTO_COMA, ";"}; }
            if (caracterActual == ':') { avanzar(); return Token{TipoToken::DOS_PUNTOS, ":"}; }
            if (caracterActual == ',') { avanzar(); return Token{TipoToken::COMA, ","}; }
            if (caracterActual == '.') { avanzar(); return Token{TipoToken::PUNTO, "."}; }
            if (caracterActual == '(') { avanzar(); return Token{TipoToken::PARENTESIS_IZQ, "("}; }
            if (caracterActual == ')') { avanzar(); return Token{TipoToken::PARENTESIS_DER, ")"}; }
            if (caracterActual == '[') { avanzar(); return Token{TipoToken::CORCHETE_IZQ, "["}; }
            if (caracterActual == ']') { avanzar(); return Token{TipoToken::CORCHETE_DER, "]"}; }
            if (caracterActual == '{') { avanzar(); return Token{TipoToken::LLAVE_IZQ, "{"}; }
            if (caracterActual == '}') { avanzar(); return Token{TipoToken::LLAVE_DER, "}"}; }

            //Checa si hay carácteres desconocidos
            char desconocido = caracterActual;
            avanzar();
            return Token{TipoToken::DESCONOCIDO, std::string(1, desconocido)};
        }

        return Token{TipoToken::FIN, ""};
    }
};
