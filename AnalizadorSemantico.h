#pragma once
//AnalizadorSemantico.h - Analizador semántico

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Nodo.h"

//==================== ANALISIS SEMANTICO ====================
//Esta etapa revisa declaraciones, ambitos, tipos, funciones y uso correcto de sentencias.
struct Simbolo {
    std::string tipo;
    bool arreglo = false;
    int tamano = -1;
};

struct FirmaFuncion {
    std::string retorno;
    std::vector<std::string> parametros;
};

class AnalizadorSemantico {
private:
    using Ambito = std::unordered_map<std::string, Simbolo>;
    std::vector<Ambito> ambitos;
    std::unordered_map<std::string, FirmaFuncion> funciones;
    std::unordered_set<std::string> funcionesDefinidas;
    std::vector<std::string> errores;
    std::string tipoRetornoActual = "";
    int profundidadCiclo = 0;
    int profundidadSwitch = 0;

    bool inicia(const std::string& texto, const std::string& prefijo) const {
        return texto.rfind(prefijo, 0) == 0;
    }
    std::string dentroParentesis(const std::string& etiqueta) const {
        size_t a = etiqueta.find('('), b = etiqueta.rfind(')');
        if (a == std::string::npos || b == std::string::npos || b <= a) return "";
        return etiqueta.substr(a + 1, b - a - 1);
    }
    void error(const std::string& mensaje) { errores.push_back(mensaje); }
    void abrirAmbito() { ambitos.emplace_back(); }
    void cerrarAmbito() { if (!ambitos.empty()) ambitos.pop_back(); }
    bool buscar(const std::string& nombre, Simbolo& salida) const {
        for (auto it = ambitos.rbegin(); it != ambitos.rend(); ++it) {
            auto encontrado = it->find(nombre);
            if (encontrado != it->end()) { salida = encontrado->second; return true; }
        }
        return false;
    }
    bool numerico(const std::string& t) const { return t == "int" || t == "float" || t == "double"; }
    bool compatible(const std::string& destino, const std::string& origen) const {
        if (destino == origen) return true;
        if (numerico(destino) && numerico(origen)) return true;
        return false;
    }
    std::string tipoVariable(const NodoPtr& n) {
        std::string nombre = dentroParentesis(n->etiqueta);
        Simbolo s;
        if (!buscar(nombre, s)) {
            error("La variable '" + nombre + "' se utiliza sin haber sido declarada.");
            return "error";
        }
        if (s.arreglo) {
            error("El arreglo '" + nombre + "' necesita un indice para utilizar un elemento.");
            return "error";
        }
        return s.tipo;
    }
    std::string analizarExpresion(const NodoPtr& n) {
        if (!n) return "error";
        const std::string& e = n->etiqueta;
        if (inicia(e, "NUMERO(")) return "int";
        if (inicia(e, "NUMERO_DECIMAL(")) return "double";
        if (inicia(e, "CADENA(")) return "string";
        if (inicia(e, "CARACTER(")) return "char";
        if (inicia(e, "BOOL(")) return "bool";
        if (e == "Endl") return "endl";
        if (inicia(e, "Variable(")) return tipoVariable(n);
        if (e == "Indice") {
            if (n->hijos.size() != 2) return "error";
            std::string nombre = dentroParentesis(n->hijos[0]->etiqueta);
            Simbolo s;
            if (!buscar(nombre, s)) { error("El arreglo '" + nombre + "' no ha sido declarado."); return "error"; }
            if (!s.arreglo) error("La variable '" + nombre + "' no es un arreglo.");
            std::string indice = analizarExpresion(n->hijos[1]);
            if (indice != "int" && indice != "error") error("El indice del arreglo '" + nombre + "' debe ser de tipo int.");
            return s.tipo;
        }
        if (inicia(e, "Llamada")) {
            if (n->hijos.empty()) return "error";
            std::string nombre = dentroParentesis(n->hijos[0]->etiqueta);
            auto f = funciones.find(nombre);
            std::vector<std::string> argumentos;
            for (size_t i = 1; i < n->hijos.size(); ++i) argumentos.push_back(analizarExpresion(n->hijos[i]));
            if (f == funciones.end()) { error("La funcion '" + nombre + "' no ha sido declarada."); return "error"; }
            if (argumentos.size() != f->second.parametros.size()) {
                error("La funcion '" + nombre + "' espera " + std::to_string(f->second.parametros.size()) + " argumento(s), pero recibio " + std::to_string(argumentos.size()) + ".");
            } else {
                for (size_t i = 0; i < argumentos.size(); ++i)
                    if (!compatible(f->second.parametros[i], argumentos[i]) && argumentos[i] != "error")
                        error("El argumento " + std::to_string(i + 1) + " de '" + nombre + "' tiene tipo " + argumentos[i] + " y se esperaba " + f->second.parametros[i] + ".");
            }
            return f->second.retorno;
        }
        if (inicia(e, "Asignacion(")) {
            if (n->hijos.size() != 2) return "error";
            std::string destino = analizarExpresion(n->hijos[0]);
            std::string origen = analizarExpresion(n->hijos[1]);
            std::string op = dentroParentesis(e);
            if (op != "=" && !numerico(destino)) error("El operador '" + op + "' requiere una variable numerica.");
            if (!compatible(destino, origen) && destino != "error" && origen != "error")
                error("No se puede asignar un valor de tipo " + origen + " a una variable de tipo " + destino + ".");
            return destino;
        }
        if (inicia(e, "Op(")) {
            std::string op = dentroParentesis(e);
            std::string a = n->hijos.size() > 0 ? analizarExpresion(n->hijos[0]) : "error";
            std::string b = n->hijos.size() > 1 ? analizarExpresion(n->hijos[1]) : "error";
            if (!numerico(a) || !numerico(b)) {
                if (a != "error" && b != "error") error("El operador aritmetico '" + op + "' solo admite operandos numericos.");
                return "error";
            }
            if (op == "%" && (a != "int" || b != "int")) error("El operador '%' requiere operandos de tipo int.");
            return (a == "double" || b == "double" || a == "float" || b == "float") ? "double" : "int";
        }
        if (inicia(e, "Comparacion(")) {
            std::string a = analizarExpresion(n->hijos[0]), b = analizarExpresion(n->hijos[1]);
            if (a != "error" && b != "error" && !compatible(a,b) && !compatible(b,a)) error("No se pueden comparar valores de tipo " + a + " y " + b + ".");
            return "bool";
        }
        if (inicia(e, "Logico(")) {
            std::string a = analizarExpresion(n->hijos[0]), b = analizarExpresion(n->hijos[1]);
            if ((a != "bool" && a != "error") || (b != "bool" && b != "error")) error("Los operadores logicos requieren expresiones bool.");
            return "bool";
        }
        if (inicia(e, "Unario(")) {
            std::string op = dentroParentesis(e), t = analizarExpresion(n->hijos[0]);
            if (op == "!") { if (t != "bool" && t != "error") error("El operador '!' requiere una expresion bool."); return "bool"; }
            if (!numerico(t) && t != "error") error("El operador unario '" + op + "' requiere un valor numerico.");
            return t;
        }
        if (inicia(e, "Postfijo(")) {
            std::string t = analizarExpresion(n->hijos[0]);
            if (!numerico(t) && t != "error") error("El incremento/decremento solo puede aplicarse a valores numericos.");
            return t;
        }
        return "error";
    }
    void registrarFunciones(const NodoPtr& raiz) {
        for (const auto& n : raiz->hijos) {
            if (!n || !(inicia(n->etiqueta,"Funcion(") || inicia(n->etiqueta,"Prototipo("))) continue;
            std::string nombre = dentroParentesis(n->etiqueta);
            FirmaFuncion firma;
            if (!n->hijos.empty()) firma.retorno = dentroParentesis(n->hijos[0]->etiqueta);
            if (n->hijos.size() > 1) for (const auto& p : n->hijos[1]->hijos)
                if (p->hijos.size() >= 2) firma.parametros.push_back(dentroParentesis(p->hijos[0]->etiqueta));
            auto it = funciones.find(nombre);
            if (it != funciones.end()) {
                if (it->second.retorno != firma.retorno || it->second.parametros != firma.parametros)
                    error("La funcion '" + nombre + "' tiene declaraciones con firmas incompatibles.");
                else if (inicia(n->etiqueta,"Funcion(")) {
                    if (funcionesDefinidas.count(nombre)) error("La funcion '" + nombre + "' esta definida mas de una vez.");
                    else funcionesDefinidas.insert(nombre);
                }
            } else {
                funciones[nombre] = firma;
                if (inicia(n->etiqueta,"Funcion(")) funcionesDefinidas.insert(nombre);
            }
        }
    }
    void analizarDeclaracion(const NodoPtr& n) {
        if (n->hijos.empty()) return;
        std::string tipo = dentroParentesis(n->hijos[0]->etiqueta);
        if (tipo == "void") error("No se puede declarar una variable de tipo void.");
        for (size_t i = 1; i < n->hijos.size(); ++i) {
            auto v = n->hijos[i];
            std::string nombre = dentroParentesis(v->etiqueta);
            if (ambitos.back().count(nombre)) { error("La variable '" + nombre + "' ya fue declarada en este ambito."); continue; }
            Simbolo sim; sim.tipo = tipo;
            NodoPtr inicial = nullptr;
            for (const auto& hijo : v->hijos) {
                if (hijo->etiqueta == "Tamano") {
                    sim.arreglo = true;
                    if (!hijo->hijos.empty()) {
                        std::string t = analizarExpresion(hijo->hijos[0]);
                        if (t != "int" && t != "error") error("El tamano del arreglo '" + nombre + "' debe ser un entero.");
                        try { sim.tamano = std::stoi(dentroParentesis(hijo->hijos[0]->etiqueta)); } catch (...) { sim.tamano = -1; }
                    }
                } else if (hijo->etiqueta == "Inicial") inicial = hijo;
            }
            ambitos.back()[nombre] = sim;
            if (inicial && !inicial->hijos.empty()) {
                NodoPtr valor = inicial->hijos[0];
                if (valor->etiqueta == "Lista") {
                    if (!sim.arreglo) error("La lista de inicializacion de '" + nombre + "' requiere que la variable sea un arreglo.");
                    if (sim.tamano >= 0 && static_cast<int>(valor->hijos.size()) > sim.tamano) error("La lista inicial del arreglo '" + nombre + "' supera su tamano declarado.");
                    for (const auto& elemento : valor->hijos) {
                        std::string t = analizarExpresion(elemento);
                        if (!compatible(tipo,t) && t != "error") error("Un elemento del arreglo '" + nombre + "' tiene tipo incompatible.");
                    }
                } else {
                    if (sim.arreglo) error("El arreglo '" + nombre + "' debe inicializarse con una lista o declararse sin valor inicial.");
                    std::string t = analizarExpresion(valor);
                    if (!compatible(tipo,t) && t != "error") error("No se puede inicializar '" + nombre + "' de tipo " + tipo + " con un valor de tipo " + t + ".");
                }
            }
        }
    }
    void analizarFuncion(const NodoPtr& n) {
        std::string nombre = dentroParentesis(n->etiqueta);
        std::string retorno = n->hijos.empty() ? "void" : dentroParentesis(n->hijos[0]->etiqueta);
        abrirAmbito();
        tipoRetornoActual = retorno;
        if (n->hijos.size() > 1) for (const auto& p : n->hijos[1]->hijos) {
            if (p->hijos.size() < 2) continue;
            std::string tipo = dentroParentesis(p->hijos[0]->etiqueta);
            std::string var = dentroParentesis(p->hijos[1]->etiqueta);
            if (ambitos.back().count(var)) error("Parametro duplicado '" + var + "' en la funcion '" + nombre + "'.");
            else ambitos.back()[var] = Simbolo{tipo,false,-1};
        }
        if (inicia(n->etiqueta,"Funcion(") && n->hijos.size() > 2) analizarSentencia(n->hijos[2]);
        cerrarAmbito();
        tipoRetornoActual.clear();
    }
    void analizarSentencia(const NodoPtr& n) {
        if (!n) return;
        const std::string& e = n->etiqueta;
        if (e == "Declaracion") { analizarDeclaracion(n); return; }
        if (inicia(e,"Funcion(") || inicia(e,"Prototipo(")) { analizarFuncion(n); return; }
        if (e == "Bloque") {
            abrirAmbito(); for (const auto& h : n->hijos) analizarSentencia(h); cerrarAmbito(); return;
        }
        if (e == "If" || e == "While" || e == "DoWhile" || e == "For") {
            abrirAmbito();
            if (e == "For") {
                for (const auto& h : n->hijos) {
                    if (h->etiqueta == "Inicio" || h->etiqueta == "Incremento" || h->etiqueta == "Condicion") {
                        for (const auto& x : h->hijos) {
                            if (x->etiqueta == "Declaracion") analizarDeclaracion(x);
                            else { std::string t=analizarExpresion(x); if (h->etiqueta=="Condicion" && t!="bool" && t!="error") error("La condicion del for debe ser bool."); }
                        }
                    } else if (h->etiqueta == "Cuerpo") {
                        ++profundidadCiclo; analizarSentencia(h); --profundidadCiclo;
                    } else analizarSentencia(h);
                }
            } else {
                for (const auto& h : n->hijos) {
                    if (h->etiqueta == "Condicion") { for (const auto& x : h->hijos) { std::string t=analizarExpresion(x); if (t!="bool" && t!="error") error("La condicion de '"+e+"' debe ser bool."); } }
                    else { if (e=="While" || e=="DoWhile") ++profundidadCiclo; analizarSentencia(h); if (e=="While" || e=="DoWhile") --profundidadCiclo; }
                }
            }
            cerrarAmbito(); return;
        }
        if (e == "Switch") {
            if (!n->hijos.empty() && !n->hijos[0]->hijos.empty()) analizarExpresion(n->hijos[0]->hijos[0]);
            ++profundidadSwitch;
            for (size_t i=1;i<n->hijos.size();++i) analizarSentencia(n->hijos[i]);
            --profundidadSwitch; return;
        }
        if (e == "Case") {
            if (!n->hijos.empty() && n->hijos[0]->etiqueta == "Valor")
                for (const auto& x : n->hijos[0]->hijos) analizarExpresion(x);
            for (size_t i = 1; i < n->hijos.size(); ++i) analizarSentencia(n->hijos[i]);
            return;
        }
        if (e == "Default" || e == "Entonces" || e == "Else" || e == "Cuerpo" || e == "Inicio" || e == "Incremento" || e == "Parametros" || e == "Lista" || e == "Inicial") {
            for (const auto& h : n->hijos) analizarSentencia(h);
            return;
        }
        if (e == "Valor") { for (const auto& h : n->hijos) analizarExpresion(h); return; }
        if (e == "Return") {
            if (tipoRetornoActual.empty()) error("La sentencia return solo puede aparecer dentro de una funcion.");
            if (n->hijos.empty()) { if (!tipoRetornoActual.empty() && tipoRetornoActual!="void") error("La funcion debe devolver un valor de tipo " + tipoRetornoActual + "."); }
            else {
                std::string t=analizarExpresion(n->hijos[0]);
                if (tipoRetornoActual=="void") error("Una funcion void no debe devolver un valor.");
                else if (!compatible(tipoRetornoActual,t) && t!="error") error("La funcion debe devolver " + tipoRetornoActual + " y esta devolviendo " + t + ".");
            }
            return;
        }
        if (e == "Break") { if (profundidadCiclo==0 && profundidadSwitch==0) error("break solo puede utilizarse dentro de un ciclo o switch."); return; }
        if (e == "Continue") { if (profundidadCiclo==0) error("continue solo puede utilizarse dentro de un ciclo."); return; }
        if (e == "Salida(cout)") { for (const auto& h:n->hijos) analizarExpresion(h); return; }
        if (e == "Entrada(cin)") { for (const auto& h:n->hijos) { std::string t=analizarExpresion(h); if (t=="string" || t=="endl") error("cin necesita una variable modificable."); } return; }
        if (e == "Expresion") { if (!n->hijos.empty()) analizarExpresion(n->hijos[0]); return; }
        //Case contiene Valor como primer hijo; se revisa como expresion y luego sus sentencias.
        for (const auto& h : n->hijos) {
            if (h->etiqueta == "Valor" || h->etiqueta == "Condicion") for (const auto& x:h->hijos) analizarExpresion(x);
            else analizarSentencia(h);
        }
    }
public:
    void analizar(const NodoPtr& raiz) {
        errores.clear(); ambitos.clear(); funciones.clear(); funcionesDefinidas.clear(); tipoRetornoActual.clear();
        profundidadCiclo=0; profundidadSwitch=0;
        abrirAmbito();
        registrarFunciones(raiz);
        for (const auto& n : raiz->hijos) {
            if (n && (inicia(n->etiqueta,"Funcion(") || inicia(n->etiqueta,"Prototipo("))) analizarFuncion(n);
            else analizarSentencia(n);
        }
    }
    const std::vector<std::string>& obtenerErrores() const { return errores; }
};
