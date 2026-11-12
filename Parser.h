#pragma once
//Parser.h - Analizador sintáctico

#include <string>
#include <vector>
#include <stdexcept>
#include "Token.h"
#include "Nodo.h"

/*PARSER
* Declaraciones, asignaciones y expresiones con precedencia
* NUEVO: bloques { }, if / else if / else, while, do-while, for, switch/case/default
* NUEVO: return, break, continue, cout << ..., cin >> ...
* NUEVO: funciones (definición y prototipo), llamadas, arreglos, ++ y --
* NUEVO: operadores lógicos && || y el módulo %
* NUEVO: recuperación de errores, ahora muestra TODOS los errores, no solo el primero*/

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos = 0;
    std::vector<std::string> errores;   //Aquí se juntan todos los errores sintácticos encontrados

    const Token& actual() const { return tokens[pos]; }
    bool es(TipoToken t) const { return actual().tipo == t; }

    //TRUE si el token actual es esa palabra reservada en específico (if, else, while...)
    bool esPalabra(const std::string& p) const {
        return es(TipoToken::PALABRA_RESERVADA) && actual().valor == p;
    }

    //TRUE si el token actual es un tipo de dato (int, float...)
    bool esTipoActual() const {
        return es(TipoToken::PALABRA_RESERVADA) && esNombreDeTipo(actual().valor);
    }

    //Texto para mostrar en los mensajes de error
    std::string descripcionActual() const {
        if (es(TipoToken::FIN)) return "fin de la entrada";
        return "'" + actual().valor + "'";
    }

    //Si el token actual es del tipo esperado lo consume, si no, lanza un error sintáctico
    Token consumir(TipoToken t, const std::string& esperado) {
        if (!es(t)) {
            throw std::runtime_error("Se esperaba " + esperado + " pero se encontro " + descripcionActual());
        }
        return tokens[pos++];
    }

    // ================== EXPRESIONES (de mayor a menor precedencia) ==================

    //Valores simples: números, cadenas, nombres, true/false, endl y expresiones entre paréntesis
    NodoPtr primario() {
        if (es(TipoToken::NUMERO) || es(TipoToken::NUMERO_DECIMAL) ||
            es(TipoToken::CADENA) || es(TipoToken::CARACTER)) {
            Token t = tokens[pos++];
            return crearNodo(tipoATexto(t.tipo) + "(" + t.valor + ")");
        }

        if (es(TipoToken::IDENTIFICADOR)) {
            Token t = tokens[pos++];
            NodoPtr n = crearNodo("Variable(" + t.valor + ")");
            n->asignable = true;
            return n;
        }

        if (esPalabra("true") || esPalabra("false")) {
            Token t = tokens[pos++];
            return crearNodo("BOOL(" + t.valor + ")");
        }

        if (esPalabra("endl")) {
            pos++;
            return crearNodo("Endl");
        }

        if (es(TipoToken::PARENTESIS_IZQ)) {
            pos++;
            NodoPtr interno = expresion();
            consumir(TipoToken::PARENTESIS_DER, "')'");
            return interno;
        }

        //Tokens que el lexer ya marco como problema
        if (es(TipoToken::DESCONOCIDO)) {
            throw std::runtime_error(descripcionActual() + " no se reconoce (identificador sin declarar o simbolo invalido)");
        }
        if (es(TipoToken::ERROR)) {
            std::string msg = descripcionActual() + " es un token con error lexico";
            if (!actual().sugerencia.empty()) {
                msg += " (quiso decir '" + actual().sugerencia + "'?)";
            }
            throw std::runtime_error(msg);
        }

        throw std::runtime_error("Se esperaba un valor o expresion pero se encontro " + descripcionActual());
    }

    //Lo que va pegado a un valor: arr[i]   f(a, b)   x++   x--
    NodoPtr postfijo() {
        NodoPtr n = primario();

        while (true) {
            if (es(TipoToken::CORCHETE_IZQ)) {
                pos++;
                NodoPtr indice = crearNodo("Indice");
                indice->asignable = true;
                indice->hijos.push_back(n);
                indice->hijos.push_back(expresion());
                consumir(TipoToken::CORCHETE_DER, "']'");
                n = indice;
            }
            //Solo un nombre puede llamarse como función
            else if (es(TipoToken::PARENTESIS_IZQ) && n->etiqueta.rfind("Variable(", 0) == 0) {
                pos++;
                NodoPtr llamada = crearNodo("Llamada");
                llamada->hijos.push_back(n);
                if (!es(TipoToken::PARENTESIS_DER)) {
                    llamada->hijos.push_back(expresion());
                    while (es(TipoToken::COMA)) {
                        pos++;
                        llamada->hijos.push_back(expresion());
                    }
                }
                consumir(TipoToken::PARENTESIS_DER, "')' al cerrar la llamada");
                n = llamada;
            }
            else if (es(TipoToken::INCREMENTO) || es(TipoToken::DECREMENTO)) {
                Token op = tokens[pos++];
                if (!n->asignable) {
                    throw std::runtime_error("El operador '" + op.valor + "' necesita una variable");
                }
                NodoPtr p = crearNodo("Postfijo(" + op.valor + ")");
                p->hijos.push_back(n);
                n = p;
            }
            else {
                break;
            }
        }
        return n;
    }

    //Operadores unarios: -x   !x   ++x   --x
    NodoPtr unario() {
        if (es(TipoToken::RESTA) || es(TipoToken::NEGACION) ||
            es(TipoToken::INCREMENTO) || es(TipoToken::DECREMENTO)) {
            Token op = tokens[pos++];
            NodoPtr n = crearNodo("Unario(" + op.valor + ")");
            NodoPtr operando = unario();
            if ((op.tipo == TipoToken::INCREMENTO || op.tipo == TipoToken::DECREMENTO) && !operando->asignable) {
                throw std::runtime_error("El operador '" + op.valor + "' necesita una variable");
            }
            n->hijos.push_back(operando);
            return n;
        }
        return postfijo();
    }

    NodoPtr termino() {
        NodoPtr izq = unario();
        while (es(TipoToken::MULT) || es(TipoToken::DIV) || es(TipoToken::MODULO)) {
            Token op = tokens[pos++];
            NodoPtr der = unario();
            NodoPtr n = crearNodo("Op(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    NodoPtr suma() {
        NodoPtr izq = termino();
        while (es(TipoToken::SUMA) || es(TipoToken::RESTA)) {
            Token op = tokens[pos++];
            NodoPtr der = termino();
            NodoPtr n = crearNodo("Op(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    //  <  >  <=  >=
    NodoPtr relacional() {
        NodoPtr izq = suma();
        while (es(TipoToken::MENOR) || es(TipoToken::MAYOR) ||
               es(TipoToken::MENOR_IGUAL) || es(TipoToken::MAYOR_IGUAL)) {
            Token op = tokens[pos++];
            NodoPtr der = suma();
            NodoPtr n = crearNodo("Comparacion(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    //  ==  !=
    NodoPtr igualdad() {
        NodoPtr izq = relacional();
        while (es(TipoToken::IGUAL_IGUAL) || es(TipoToken::DIFERENTE_DE)) {
            Token op = tokens[pos++];
            NodoPtr der = relacional();
            NodoPtr n = crearNodo("Comparacion(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    //  &&
    NodoPtr logicoAnd() {
        NodoPtr izq = igualdad();
        while (es(TipoToken::AND)) {
            Token op = tokens[pos++];
            NodoPtr der = igualdad();
            NodoPtr n = crearNodo("Logico(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    //  ||
    NodoPtr logicoOr() {
        NodoPtr izq = logicoAnd();
        while (es(TipoToken::OR)) {
            Token op = tokens[pos++];
            NodoPtr der = logicoAnd();
            NodoPtr n = crearNodo("Logico(" + op.valor + ")");
            n->hijos = {izq, der};
            izq = n;
        }
        return izq;
    }

    //Asignaciones:  x = 5    x += 2    a[i] = 3    a = b = 0 (de derecha a izquierda)
    NodoPtr expresion() {
        NodoPtr izq = logicoOr();
        if (es(TipoToken::ASIGNACION) || es(TipoToken::ASIGNACION_COMPUESTA)) {
            Token op = tokens[pos++];
            if (!izq->asignable) {
                throw std::runtime_error("No se puede asignar con '" + op.valor + "' a eso, el lado izquierdo debe ser una variable");
            }
            NodoPtr der = expresion();
            NodoPtr n = crearNodo("Asignacion(" + op.valor + ")");
            n->hijos = {izq, der};
            return n;
        }
        return izq;
    }

    // ================== SENTENCIAS ==================

    //Después de "int nombre" lee el [tamaño] y el = valor, y los agrega a la declaración
    //Acepta:  x    x = 5    arr[3]    arr[3] = {1, 2, 3}
    void declarador(NodoPtr& decl, const Token& nombre) {
        NodoPtr v = crearNodo("Variable(" + nombre.valor + ")");

        if (es(TipoToken::CORCHETE_IZQ)) {
            pos++;
            NodoPtr tam = crearNodo("Tamano");
            if (!es(TipoToken::CORCHETE_DER)) tam->hijos.push_back(expresion());
            consumir(TipoToken::CORCHETE_DER, "']'");
            v->hijos.push_back(tam);
        }

        if (es(TipoToken::ASIGNACION)) {
            pos++;
            NodoPtr ini = crearNodo("Inicial");
            if (es(TipoToken::LLAVE_IZQ)) {
                pos++;
                NodoPtr lista = crearNodo("Lista");
                lista->hijos.push_back(expresion());
                while (es(TipoToken::COMA)) {
                    pos++;
                    lista->hijos.push_back(expresion());
                }
                consumir(TipoToken::LLAVE_DER, "'}' al cerrar la lista de valores");
                ini->hijos.push_back(lista);
            } else {
                ini->hijos.push_back(expresion());
            }
            v->hijos.push_back(ini);
        }
        decl->hijos.push_back(v);
    }

    //Función o prototipo:  int suma(int a, int b) { ... }     int suma(int a, int b);
    NodoPtr funcion(const Token& tipo, const Token& nombre) {
        pos++;   //Salta el '('
        NodoPtr n = crearNodo("Funcion(" + nombre.valor + ")");
        n->hijos.push_back(crearNodo("Tipo(" + tipo.valor + ")"));

        NodoPtr params = crearNodo("Parametros");
        if (esPalabra("void") && tokens[pos + 1].tipo == TipoToken::PARENTESIS_DER) {
            pos++;   //(void) significa sin parámetros
        } else if (!es(TipoToken::PARENTESIS_DER)) {
            while (true) {
                if (!esTipoActual()) {
                    throw std::runtime_error("Se esperaba el tipo de un parametro pero se encontro " + descripcionActual());
                }
                Token t = tokens[pos++];
                Token p = consumir(TipoToken::IDENTIFICADOR, "el nombre del parametro");
                NodoPtr par = crearNodo("Parametro");
                par->hijos.push_back(crearNodo("Tipo(" + t.valor + ")"));
                par->hijos.push_back(crearNodo("Variable(" + p.valor + ")"));
                params->hijos.push_back(par);

                if (es(TipoToken::COMA)) { pos++; continue; }
                break;
            }
        }
        consumir(TipoToken::PARENTESIS_DER, "')' al cerrar los parametros");
        n->hijos.push_back(params);

        //Si termina en ';' solo es el prototipo, si no, debe traer su cuerpo
        if (es(TipoToken::PUNTO_COMA)) {
            pos++;
            n->etiqueta = "Prototipo(" + nombre.valor + ")";
            return n;
        }
        n->hijos.push_back(bloque());
        return n;
    }

    //int x;    int x = 5;    int a, b = 2;    int arr[3];    int f(...) {...}
    NodoPtr declaracion() {
        Token tipo = tokens[pos++];
        Token nombre = consumir(TipoToken::IDENTIFICADOR, "un nombre de variable o funcion");

        if (es(TipoToken::PARENTESIS_IZQ)) return funcion(tipo, nombre);

        NodoPtr n = crearNodo("Declaracion");
        n->hijos.push_back(crearNodo("Tipo(" + tipo.valor + ")"));
        declarador(n, nombre);

        while (es(TipoToken::COMA)) {
            pos++;
            Token otro = consumir(TipoToken::IDENTIFICADOR, "un nombre de variable despues de ','");
            declarador(n, otro);
        }
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    //{ sentencia sentencia ... }
    NodoPtr bloque() {
        consumir(TipoToken::LLAVE_IZQ, "'{'");
        NodoPtr n = crearNodo("Bloque");
        while (!es(TipoToken::LLAVE_DER) && !es(TipoToken::FIN)) {
            if (NodoPtr h = sentenciaSegura()) n->hijos.push_back(h);
        }
        consumir(TipoToken::LLAVE_DER, "'}' para cerrar el bloque");
        return n;
    }

    //if (condicion) sentencia [else sentencia]      ("else if" sale solo porque el else lleva otra sentencia)
    NodoPtr sentenciaIf() {
        pos++;   //Salta el 'if'
        consumir(TipoToken::PARENTESIS_IZQ, "'(' despues de 'if'");

        NodoPtr n = crearNodo("If");
        NodoPtr cond = crearNodo("Condicion");
        cond->hijos.push_back(expresion());
        n->hijos.push_back(cond);
        consumir(TipoToken::PARENTESIS_DER, "')' al cerrar la condicion");

        NodoPtr entonces = crearNodo("Entonces");
        entonces->hijos.push_back(sentencia());
        n->hijos.push_back(entonces);

        if (esPalabra("else")) {
            pos++;
            NodoPtr sino = crearNodo("Else");
            sino->hijos.push_back(sentencia());
            n->hijos.push_back(sino);
        }
        return n;
    }

    //while (condicion) sentencia
    NodoPtr sentenciaWhile() {
        pos++;   //Salta el 'while'
        consumir(TipoToken::PARENTESIS_IZQ, "'(' despues de 'while'");

        NodoPtr n = crearNodo("While");
        NodoPtr cond = crearNodo("Condicion");
        cond->hijos.push_back(expresion());
        n->hijos.push_back(cond);
        consumir(TipoToken::PARENTESIS_DER, "')' al cerrar la condicion");

        NodoPtr cuerpo = crearNodo("Cuerpo");
        cuerpo->hijos.push_back(sentencia());
        n->hijos.push_back(cuerpo);
        return n;
    }

    //do sentencia while (condicion);
    NodoPtr sentenciaDoWhile() {
        pos++;   //Salta el 'do'
        NodoPtr n = crearNodo("DoWhile");

        NodoPtr cuerpo = crearNodo("Cuerpo");
        cuerpo->hijos.push_back(sentencia());
        n->hijos.push_back(cuerpo);

        if (!esPalabra("while")) {
            throw std::runtime_error("Se esperaba 'while' despues del cuerpo del 'do' pero se encontro " + descripcionActual());
        }
        pos++;
        consumir(TipoToken::PARENTESIS_IZQ, "'(' despues de 'while'");
        NodoPtr cond = crearNodo("Condicion");
        cond->hijos.push_back(expresion());
        n->hijos.push_back(cond);
        consumir(TipoToken::PARENTESIS_DER, "')' al cerrar la condicion");
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    //for (inicio; condicion; incremento) sentencia      (las tres partes pueden ir vacías)
    NodoPtr sentenciaFor() {
        pos++;   //Salta el 'for'
        consumir(TipoToken::PARENTESIS_IZQ, "'(' despues de 'for'");
        NodoPtr n = crearNodo("For");

        NodoPtr inicio = crearNodo("Inicio");
        if (es(TipoToken::PUNTO_COMA)) {
            pos++;
        } else if (esTipoActual()) {
            inicio->hijos.push_back(declaracion());     //Ya consume el ';'
        } else {
            inicio->hijos.push_back(expresion());
            consumir(TipoToken::PUNTO_COMA, "';' despues del inicio del for");
        }
        n->hijos.push_back(inicio);

        NodoPtr cond = crearNodo("Condicion");
        if (!es(TipoToken::PUNTO_COMA)) cond->hijos.push_back(expresion());
        consumir(TipoToken::PUNTO_COMA, "';' despues de la condicion del for");
        n->hijos.push_back(cond);

        NodoPtr inc = crearNodo("Incremento");
        if (!es(TipoToken::PARENTESIS_DER)) inc->hijos.push_back(expresion());
        consumir(TipoToken::PARENTESIS_DER, "')' al cerrar el for");
        n->hijos.push_back(inc);

        NodoPtr cuerpo = crearNodo("Cuerpo");
        cuerpo->hijos.push_back(sentencia());
        n->hijos.push_back(cuerpo);
        return n;
    }

    //TRUE si ya se acabaron las sentencias de un case (viene otro case, default, o se cierra el switch)
    bool esFinDeCaso() const {
        return es(TipoToken::LLAVE_DER) || es(TipoToken::FIN) || esPalabra("case") || esPalabra("default");
    }

    //switch (valor) { case 1: ... break;  default: ... }
    NodoPtr sentenciaSwitch() {
        pos++;   //Salta el 'switch'
        consumir(TipoToken::PARENTESIS_IZQ, "'(' despues de 'switch'");

        NodoPtr n = crearNodo("Switch");
        NodoPtr valor = crearNodo("Valor");
        valor->hijos.push_back(expresion());
        n->hijos.push_back(valor);
        consumir(TipoToken::PARENTESIS_DER, "')'");
        consumir(TipoToken::LLAVE_IZQ, "'{' despues del switch");

        while (!es(TipoToken::LLAVE_DER) && !es(TipoToken::FIN)) {
            NodoPtr caso;
            try { //comaEnDeclaracion
                if (esPalabra("case")) {
                    pos++;
                    caso = crearNodo("Case");
                    NodoPtr val = crearNodo("Valor");
                    val->hijos.push_back(expresion());
                    caso->hijos.push_back(val);
                } else if (esPalabra("default")) {
                    pos++;
                    caso = crearNodo("Default");
                } else {
                    throw std::runtime_error("Dentro de un switch se esperaba 'case' o 'default' pero se encontro " + descripcionActual());
                }
                consumir(TipoToken::DOS_PUNTOS, "':'");
            } catch (const std::runtime_error& e) {
                errores.push_back(e.what());
                sincronizarCaso();
                continue;
            }

            while (!esFinDeCaso()) {
                if (NodoPtr h = sentenciaSegura()) caso->hijos.push_back(h);
            }
            n->hijos.push_back(caso);
        }

        consumir(TipoToken::LLAVE_DER, "'}' para cerrar el switch");
        return n;
    }

    //return;    return expresion;
    NodoPtr sentenciaReturn() {
        pos++;   //Salta el 'return'
        NodoPtr n = crearNodo("Return");
        if (!es(TipoToken::PUNTO_COMA)) n->hijos.push_back(expresion());
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    //cout << algo << algo << endl;
    NodoPtr sentenciaSalida() {
        pos++;   //Salta el 'cout'
        NodoPtr n = crearNodo("Salida(cout)");
        if (!es(TipoToken::DESPLAZ_IZQ)) {
            throw std::runtime_error("Se esperaba '<<' despues de 'cout' pero se encontro " + descripcionActual());
        }
        while (es(TipoToken::DESPLAZ_IZQ)) {
            pos++;
            n->hijos.push_back(suma());
        }
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    NodoPtr sentenciaEntrada() {
        pos++;   //Salta el 'cin'
        NodoPtr n = crearNodo("Entrada(cin)");
        if (!es(TipoToken::DESPLAZ_DER)) {
            throw std::runtime_error("Se esperaba '>>' despues de 'cin' pero se encontro " + descripcionActual());
        }
        while (es(TipoToken::DESPLAZ_DER)) {
            pos++;
            NodoPtr destino = postfijo();
            if (!destino->asignable) {
                throw std::runtime_error("'cin >>' necesita una variable donde guardar el dato");
            }
            n->hijos.push_back(destino);
        }
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    NodoPtr sentencia() {
        if (es(TipoToken::PALABRA_RESERVADA)) {
            const std::string v = actual().valor;   //Copia, porque pos va a avanzar

            if (esNombreDeTipo(v)) return declaracion();
            if (v == "if")         return sentenciaIf();
            if (v == "while")      return sentenciaWhile();
            if (v == "do")         return sentenciaDoWhile();
            if (v == "for")        return sentenciaFor();
            if (v == "switch")     return sentenciaSwitch();
            if (v == "return")     return sentenciaReturn();
            if (v == "cout")       return sentenciaSalida();
            if (v == "cin")        return sentenciaEntrada();

            if (v == "break" || v == "continue") {
                pos++;
                consumir(TipoToken::PUNTO_COMA, "';'");
                return crearNodo(v == "break" ? "Break" : "Continue");
            }
            if (v == "else") {
                throw std::runtime_error("'else' sin un 'if' que lo preceda");
            }
            if (v == "case" || v == "default") {
                throw std::runtime_error("'" + v + "' solo puede usarse dentro de un switch");
            }
        }

        if (es(TipoToken::LLAVE_IZQ)) return bloque();

        if (es(TipoToken::PUNTO_COMA)) {
            pos++;
            return crearNodo("Vacia");
        }

        if (es(TipoToken::LLAVE_DER)) {
            throw std::runtime_error("'}' sin una '{' que la abra");
        }

        //Cualquier otra cosa se toma como una expresion, ya sea  x = 5;   i++;   suma(1, 2);
        NodoPtr n = crearNodo("Expresion");
        n->hijos.push_back(expresion());
        consumir(TipoToken::PUNTO_COMA, "';'");
        return n;
    }

    //****************** RECUPERA ERRORES *******************

    //Aqui despues de un error, salta tokens hasta el final de esa sentencia para poder seguir y encontrar mas errores
    //Se detiene al pasar un ';', o al terminar un bloque { } completo, o antes de un '}' que cierra el bloque de arriba
    void sincronizar() {
        int profundidad = 0;
        while (!es(TipoToken::FIN)) {
            if (es(TipoToken::LLAVE_IZQ)) {
                profundidad++;
            } else if (es(TipoToken::LLAVE_DER)) {
                if (profundidad == 0) return;               //Es la '}' del bloque que nos contiene, no se toca
                profundidad--;
                if (profundidad == 0) { pos++; return; }
            } else if (es(TipoToken::PUNTO_COMA) && profundidad == 0) {
                pos++;
                return;
            }
            pos++;
        }
    }

    //Después de un error en la cabecera de un case, salta hasta el siguiente case/default
    //o hasta la '}' que cierra el switch (ignorando las { } anidadas)
    void sincronizarCaso() {
        int nivel = 0;
        while (!es(TipoToken::FIN)) {
            if (es(TipoToken::LLAVE_IZQ)) {
                nivel++;
            } else if (es(TipoToken::LLAVE_DER)) {
                if (nivel == 0) return;
                nivel--;
            } else if (nivel == 0 && (esPalabra("case") || esPalabra("default"))) {
                return;
            }
            pos++;
        }
    }

    //Intenta leer una sentenci y si hay un error lo guarda, se recupera y devuelve nullptr
    NodoPtr sentenciaSegura() {
        try {
            return sentencia();
        } catch (const std::runtime_error& e) {
            errores.push_back(e.what());
            sincronizar();
            return nullptr;
        }
    }

public:
    //aqui recibe los tokens del lexer y los espacios se ignoran
    Parser(const std::vector<Token>& entrada) {
        for (const Token& t : entrada) {
            if (t.tipo != TipoToken::ESPACIO) tokens.push_back(t);
        }
        tokens.push_back(Token{TipoToken::FIN, ""});
    } //LLAVE_DER

    NodoPtr parsear() {
        NodoPtr raiz = crearNodo("Programa");
        while (!es(TipoToken::FIN)) {
            size_t inicio = pos;
            if (NodoPtr h = sentenciaSegura()) raiz->hijos.push_back(h);
            if (pos == inicio) pos++;   //Aqui hay un seguro para que no pueda hanber un ciclo infinito si esque corurre
        }
        return raiz;
    }

    const std::vector<std::string>& obtenerErrores() const { return errores; }
};
