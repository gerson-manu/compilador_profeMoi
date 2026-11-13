#pragma once
//CodigoIntermedio.h - Generador de código intermedio (código de tres direcciones)

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "Nodo.h"

/*CODIGO INTERMEDIO
* Recorre el arbol sintactico (ya validado por el semantico) y genera instrucciones de tres direcciones.
* Cada instruccion es una CUADRUPLA: (operador, argumento1, argumento2, resultado)
* - Usa variables temporales: t1, t2, t3...
* - Usa etiquetas para los saltos:  L1, L2, L3...
* - if / while / do-while / for / switch se convierten en saltos (goto, if, ifFalse)
* - && y || usan evaluacion en cortocircuito
* - El codigo que esta fuera de funciones se agrupa dentro de una funcion llamada "main"
*/

struct Cuadrupla {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string res;
};

class GeneradorCodigoIntermedio {
private:
    std::vector<Cuadrupla> codigo;
    int contadorTemporales = 0;
    int contadorEtiquetas = 0;
    std::vector<std::string> destinosBreak;      //Etiqueta a la que salta un 'break'
    std::vector<std::string> destinosContinue;   //Etiqueta a la que salta un 'continue'

    // ------------------ Utilidades ------------------
    bool inicia(const std::string& texto, const std::string& prefijo) const {
        return texto.rfind(prefijo, 0) == 0;
    }
    std::string dentroParentesis(const std::string& etiqueta) const {
        size_t a = etiqueta.find('('), b = etiqueta.rfind(')');
        if (a == std::string::npos || b == std::string::npos || b <= a) return "";
        return etiqueta.substr(a + 1, b - a - 1);
    }
    std::string nuevoTemporal() { return "t" + std::to_string(++contadorTemporales); }
    std::string nuevaEtiqueta() { return "L" + std::to_string(++contadorEtiquetas); }

    void emitir(const std::string& op, const std::string& a1 = "", const std::string& a2 = "", const std::string& res = "") {
        codigo.push_back({op, a1, a2, res});
    }
    void etiqueta(const std::string& l) { emitir("label", "", "", l); }
    void saltar(const std::string& l) { emitir("goto", "", "", l); }

    //Quita el '=' de un operador compuesto:  +=  ->  +
    std::string operadorBase(const std::string& op) const { return op.substr(0, op.size() - 1); }

    // ------------------ Direcciones (lados izquierdos) ------------------
    //Un destino puede ser una variable (x) o un elemento de arreglo (arr[i])
    struct Destino {
        bool esIndice = false;
        std::string nombre;     //x, o el nombre del arreglo
        std::string indice;     //solo si es arr[i]
    };

    Destino obtenerDestino(const NodoPtr& n) {
        Destino d;
        if (n->etiqueta == "Indice") {
            d.esIndice = true;
            d.nombre = dentroParentesis(n->hijos[0]->etiqueta);
            d.indice = generarExpresion(n->hijos[1]);
        } else {
            d.nombre = dentroParentesis(n->etiqueta);
        }
        return d;
    }
    std::string leerDestino(const Destino& d) {
        if (!d.esIndice) return d.nombre;
        std::string t = nuevoTemporal();
        emitir("=[]", d.nombre, d.indice, t);
        return t;
    }
    void escribirDestino(const Destino& d, const std::string& valor) {
        if (d.esIndice) emitir("[]=", d.nombre, d.indice, valor);
        else emitir("=", valor, "", d.nombre);
    }

    // ------------------ Expresiones ------------------
    //Genera el codigo de una expresion y devuelve el nombre donde queda su valor (variable, constante o temporal)
    std::string generarExpresion(const NodoPtr& n) {
        if (!n) return "";
        const std::string& e = n->etiqueta;

        if (inicia(e, "NUMERO_DECIMAL(")) return dentroParentesis(e);
        if (inicia(e, "NUMERO("))         return dentroParentesis(e);
        if (inicia(e, "CADENA("))         return "\"" + dentroParentesis(e) + "\"";
        if (inicia(e, "CARACTER("))       return "'" + dentroParentesis(e) + "'";
        if (inicia(e, "BOOL("))           return dentroParentesis(e) == "true" ? "1" : "0";
        if (e == "Endl")                  return "endl";
        if (inicia(e, "Variable("))       return dentroParentesis(e);

        if (e == "Indice") {
            Destino d = obtenerDestino(n);
            return leerDestino(d);
        }

        if (inicia(e, "Op(") || inicia(e, "Comparacion(")) {
            std::string a = generarExpresion(n->hijos[0]);
            std::string b = generarExpresion(n->hijos[1]);
            std::string t = nuevoTemporal();
            emitir(dentroParentesis(e), a, b, t);
            return t;
        }

        if (inicia(e, "Logico(")) {
            //Se materializa como 0/1 usando cortocircuito
            std::string t = nuevoTemporal();
            std::string lFalso = nuevaEtiqueta(), lFin = nuevaEtiqueta();
            saltarSi(n, lFalso, false);
            emitir("=", "1", "", t);
            saltar(lFin);
            etiqueta(lFalso);
            emitir("=", "0", "", t);
            etiqueta(lFin);
            return t;
        }

        if (inicia(e, "Unario(")) {
            std::string op = dentroParentesis(e);
            if (op == "++" || op == "--") {
                //Prefijo: primero se modifica y luego se usa
                Destino d = obtenerDestino(n->hijos[0]);
                std::string valor = leerDestino(d);
                std::string t = nuevoTemporal();
                emitir(op == "++" ? "+" : "-", valor, "1", t);
                escribirDestino(d, t);
                return t;
            }
            std::string a = generarExpresion(n->hijos[0]);
            std::string t = nuevoTemporal();
            emitir(op, a, "", t);   //Unario: - o !
            return t;
        }

        if (inicia(e, "Postfijo(")) {
            //Postfijo: se guarda el valor viejo, se modifica, y se devuelve el viejo
            std::string op = dentroParentesis(e);
            Destino d = obtenerDestino(n->hijos[0]);
            std::string viejo = nuevoTemporal();
            emitir("=", leerDestino(d), "", viejo);
            std::string nuevo = nuevoTemporal();
            emitir(op == "++" ? "+" : "-", viejo, "1", nuevo);
            escribirDestino(d, nuevo);
            return viejo;
        }

        if (inicia(e, "Asignacion(")) {
            std::string op = dentroParentesis(e);
            Destino d = obtenerDestino(n->hijos[0]);
            std::string valor = generarExpresion(n->hijos[1]);
            if (op != "=") {
                std::string actual = leerDestino(d);
                std::string t = nuevoTemporal();
                emitir(operadorBase(op), actual, valor, t);
                valor = t;
            }
            escribirDestino(d, valor);
            return d.esIndice ? valor : d.nombre;
        }

        if (e == "Llamada") {
            std::string nombre = dentroParentesis(n->hijos[0]->etiqueta);
            std::vector<std::string> argumentos;
            for (size_t i = 1; i < n->hijos.size(); ++i) argumentos.push_back(generarExpresion(n->hijos[i]));
            for (const auto& a : argumentos) emitir("param", a);
            std::string t = nuevoTemporal();
            emitir("call", nombre, std::to_string(argumentos.size()), t);
            return t;
        }

        return "?";
    }

    //Salta a 'destino' si la condicion vale 'cuando'; si no, sigue de largo (cortocircuito para && y ||)
    void saltarSi(const NodoPtr& n, const std::string& destino, bool cuando) {
        const std::string& e = n->etiqueta;

        if (inicia(e, "Unario(") && dentroParentesis(e) == "!") {
            saltarSi(n->hijos[0], destino, !cuando);
            return;
        }
        if (inicia(e, "Logico(")) {
            bool esAnd = dentroParentesis(e) == "&&";
            if (esAnd != cuando) {
                //(&& y salta si falso) o (|| y salta si verdadero): basta que uno de los dos lo cumpla
                saltarSi(n->hijos[0], destino, cuando);
                saltarSi(n->hijos[1], destino, cuando);
            } else {
                //(&& y salta si verdadero) o (|| y salta si falso): se necesitan ambos
                std::string omitir = nuevaEtiqueta();
                saltarSi(n->hijos[0], omitir, !cuando);
                saltarSi(n->hijos[1], destino, cuando);
                etiqueta(omitir);
            }
            return;
        }
        std::string v = generarExpresion(n);
        emitir(cuando ? "if" : "ifFalse", v, "", destino);
    }

    // ------------------ Sentencias ------------------
    void generarDeclaracion(const NodoPtr& n) {
        for (size_t i = 1; i < n->hijos.size(); ++i) {
            const NodoPtr& v = n->hijos[i];
            std::string nombre = dentroParentesis(v->etiqueta);
            std::string tipo = dentroParentesis(n->hijos[0]->etiqueta);
            bool arreglo = false;
            NodoPtr inicial = nullptr;
            std::string tam = "";

            for (const auto& h : v->hijos) {
                if (h->etiqueta == "Tamano") {
                    arreglo = true;
                    if (!h->hijos.empty()) tam = generarExpresion(h->hijos[0]);
                } else if (h->etiqueta == "Inicial") {
                    inicial = h;
                }
            }

            if (arreglo) {
                //Si no trae tamano, se toma de la cantidad de valores de la lista
                if (tam.empty() && inicial && !inicial->hijos.empty()) tam = std::to_string(inicial->hijos[0]->hijos.size());
                emitir("alloc", tipo, tam, nombre);
            }
            if (inicial && !inicial->hijos.empty()) {
                const NodoPtr& valor = inicial->hijos[0];
                if (valor->etiqueta == "Lista") {
                    for (size_t k = 0; k < valor->hijos.size(); ++k) {
                        std::string x = generarExpresion(valor->hijos[k]);
                        emitir("[]=", nombre, std::to_string(k), x);
                    }
                } else {
                    emitir("=", generarExpresion(valor), "", nombre);
                }
            }
        }
    }

    void generarFuncion(const NodoPtr& n) {
        std::string nombre = dentroParentesis(n->etiqueta);
        emitir("func_begin", "", "", nombre);
        for (const auto& p : n->hijos[1]->hijos) {
            emitir("param_in", dentroParentesis(p->hijos[0]->etiqueta), "", dentroParentesis(p->hijos[1]->etiqueta));
        }
        if (n->hijos.size() > 2) generarSentencia(n->hijos[2]);
        emitir("func_end", "", "", nombre);
    }

    void generarIf(const NodoPtr& n) {
        std::string lElse = nuevaEtiqueta();
        bool tieneElse = n->hijos.size() > 2;
        saltarSi(n->hijos[0]->hijos[0], lElse, false);
        generarSentencia(n->hijos[1]);
        if (tieneElse) {
            std::string lFin = nuevaEtiqueta();
            saltar(lFin);
            etiqueta(lElse);
            generarSentencia(n->hijos[2]);
            etiqueta(lFin);
        } else {
            etiqueta(lElse);
        }
    }

    void generarWhile(const NodoPtr& n) {
        std::string lInicio = nuevaEtiqueta(), lFin = nuevaEtiqueta();
        etiqueta(lInicio);
        saltarSi(n->hijos[0]->hijos[0], lFin, false);
        destinosBreak.push_back(lFin); destinosContinue.push_back(lInicio);
        generarSentencia(n->hijos[1]);
        destinosBreak.pop_back(); destinosContinue.pop_back();
        saltar(lInicio);
        etiqueta(lFin);
    }

    void generarDoWhile(const NodoPtr& n) {
        std::string lCuerpo = nuevaEtiqueta(), lCond = nuevaEtiqueta(), lFin = nuevaEtiqueta();
        etiqueta(lCuerpo);
        destinosBreak.push_back(lFin); destinosContinue.push_back(lCond);
        generarSentencia(n->hijos[0]);
        destinosBreak.pop_back(); destinosContinue.pop_back();
        etiqueta(lCond);
        saltarSi(n->hijos[1]->hijos[0], lCuerpo, true);
        etiqueta(lFin);
    }

    void generarFor(const NodoPtr& n) {
        //hijos: Inicio, Condicion, Incremento, Cuerpo
        std::string lCond = nuevaEtiqueta(), lInc = nuevaEtiqueta(), lFin = nuevaEtiqueta();
        for (const auto& h : n->hijos[0]->hijos) generarSentencia(h);
        etiqueta(lCond);
        if (!n->hijos[1]->hijos.empty()) saltarSi(n->hijos[1]->hijos[0], lFin, false);
        destinosBreak.push_back(lFin); destinosContinue.push_back(lInc);
        generarSentencia(n->hijos[3]);
        destinosBreak.pop_back(); destinosContinue.pop_back();
        etiqueta(lInc);
        for (const auto& h : n->hijos[2]->hijos) generarExpresion(h);
        saltar(lCond);
        etiqueta(lFin);
    }

    void generarSwitch(const NodoPtr& n) {
        std::string valor = generarExpresion(n->hijos[0]->hijos[0]);
        std::string lFin = nuevaEtiqueta();
        std::vector<std::string> etiquetasCasos;
        std::string lDefault = "";

        //Primero las comparaciones...
        for (size_t i = 1; i < n->hijos.size(); ++i) {
            std::string l = nuevaEtiqueta();
            etiquetasCasos.push_back(l);
            if (n->hijos[i]->etiqueta == "Case") {
                std::string v = generarExpresion(n->hijos[i]->hijos[0]->hijos[0]);
                std::string t = nuevoTemporal();
                emitir("==", valor, v, t);
                emitir("if", t, "", l);
            } else {
                lDefault = l;
            }
        }
        saltar(lDefault.empty() ? lFin : lDefault);

        //...y luego los cuerpos, en orden (asi funciona el paso al siguiente case si no hay break)
        destinosBreak.push_back(lFin);
        for (size_t i = 1; i < n->hijos.size(); ++i) {
            etiqueta(etiquetasCasos[i - 1]);
            size_t primero = (n->hijos[i]->etiqueta == "Case") ? 1 : 0;
            for (size_t k = primero; k < n->hijos[i]->hijos.size(); ++k) generarSentencia(n->hijos[i]->hijos[k]);
        }
        destinosBreak.pop_back();
        etiqueta(lFin);
    }

    void generarSentencia(const NodoPtr& n) {
        if (!n) return;
        const std::string& e = n->etiqueta;

        if (e == "Declaracion")  { generarDeclaracion(n); return; }
        if (inicia(e, "Funcion(")) { generarFuncion(n); return; }
        if (inicia(e, "Prototipo(")) return;    //Un prototipo no genera codigo
        if (e == "Bloque")       { for (const auto& h : n->hijos) generarSentencia(h); return; }
        if (e == "If")           { generarIf(n); return; }
        if (e == "While")        { generarWhile(n); return; }
        if (e == "DoWhile")      { generarDoWhile(n); return; }
        if (e == "For")          { generarFor(n); return; }
        if (e == "Switch")       { generarSwitch(n); return; }
        if (e == "Entonces" || e == "Else" || e == "Cuerpo") {
            for (const auto& h : n->hijos) generarSentencia(h);
            return;
        }
        if (e == "Return") {
            emitir("return", n->hijos.empty() ? "" : generarExpresion(n->hijos[0]));
            return;
        }
        if (e == "Break")    { if (!destinosBreak.empty()) saltar(destinosBreak.back()); return; }
        if (e == "Continue") { if (!destinosContinue.empty()) saltar(destinosContinue.back()); return; }
        if (e == "Salida(cout)") {
            for (const auto& h : n->hijos) emitir("print", generarExpresion(h));
            return;
        }
        if (e == "Entrada(cin)") {
            for (const auto& h : n->hijos) {
                Destino d = obtenerDestino(h);
                if (d.esIndice) {
                    std::string t = nuevoTemporal();
                    emitir("read", "", "", t);
                    escribirDestino(d, t);
                } else {
                    emitir("read", "", "", d.nombre);
                }
            }
            return;
        }
        if (e == "Expresion") { if (!n->hijos.empty()) generarExpresion(n->hijos[0]); return; }
        //Vacia y cualquier otro nodo no generan codigo
    }

    // ------------------ Impresion ------------------
    static bool esOperadorBinario(const std::string& op) {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
               op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=";
    }

    std::string aTexto(const Cuadrupla& q) const {
        if (q.op == "label")      return q.res + ":";
        if (q.op == "func_begin") return "funcion " + q.res + ":";
        if (q.op == "func_end")   return "fin_funcion " + q.res;
        if (q.op == "param_in")   return "    recibe " + q.arg1 + " " + q.res;
        if (q.op == "goto")       return "    goto " + q.res;
        if (q.op == "if")         return "    if " + q.arg1 + " goto " + q.res;
        if (q.op == "ifFalse")    return "    ifFalse " + q.arg1 + " goto " + q.res;
        if (q.op == "param")      return "    param " + q.arg1;
        if (q.op == "call")       return "    " + q.res + " = call " + q.arg1 + ", " + q.arg2;
        if (q.op == "return")     return q.arg1.empty() ? "    return" : "    return " + q.arg1;
        if (q.op == "print")      return "    print " + q.arg1;
        if (q.op == "read")       return "    read " + q.res;
        if (q.op == "alloc")      return "    " + q.res + " = arreglo de " + q.arg1 + "[" + q.arg2 + "]";
        if (q.op == "=")          return "    " + q.res + " = " + q.arg1;
        if (q.op == "=[]")        return "    " + q.res + " = " + q.arg1 + "[" + q.arg2 + "]";
        if (q.op == "[]=")        return "    " + q.arg1 + "[" + q.arg2 + "] = " + q.res;
        if (q.arg2.empty())       return "    " + q.res + " = " + q.op + q.arg1;   //Unario: -x  !x
        if (esOperadorBinario(q.op)) return "    " + q.res + " = " + q.arg1 + " " + q.op + " " + q.arg2;
        return "    " + q.op;
    }

public:
    void generar(const NodoPtr& raiz) {
        codigo.clear();
        contadorTemporales = 0;
        contadorEtiquetas = 0;
        destinosBreak.clear();
        destinosContinue.clear();

        //Primero las funciones; lo demas (codigo suelto) se agrupa dentro de "main"
        bool haySuelto = false;
        for (const auto& n : raiz->hijos) {
            if (!n) continue;
            if (inicia(n->etiqueta, "Funcion(")) generarFuncion(n);
            else if (!inicia(n->etiqueta, "Prototipo(")) haySuelto = true;
        }
        if (haySuelto) {
            emitir("func_begin", "", "", "main");
            for (const auto& n : raiz->hijos) {
                if (n && !inicia(n->etiqueta, "Funcion(")) generarSentencia(n);
            }
            emitir("func_end", "", "", "main");
        }
    }

    const std::vector<Cuadrupla>& obtenerCodigo() const { return codigo; }

    //Codigo de tres direcciones en forma de texto
    void imprimirCodigo() const {
        for (const auto& q : codigo) std::cout << aTexto(q) << "\n";
    }

    //Tabla de cuadruplas (operador, arg1, arg2, resultado)
    void imprimirCuadruplas() const {
        std::cout << std::left
                  << std::setw(5)  << "#"
                  << std::setw(13) << "Operador"
                  << std::setw(14) << "Arg1"
                  << std::setw(14) << "Arg2"
                  << "Resultado\n";
        std::cout << std::string(60, '-') << "\n";
        for (size_t i = 0; i < codigo.size(); ++i) {
            const auto& q = codigo[i];
            std::cout << std::setw(5)  << i
                      << std::setw(13) << q.op
                      << std::setw(14) << (q.arg1.empty() ? "-" : q.arg1)
                      << std::setw(14) << (q.arg2.empty() ? "-" : q.arg2)
                      << (q.res.empty() ? "-" : q.res) << "\n";
        }
    }
};
