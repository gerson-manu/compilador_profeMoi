#pragma once
// Librerias.h - Catalogo de librerias que reconoce el compilador y las funciones de cada una
// (sin acentos a proposito, para que se vea igual en Code::Blocks y en CLion)

#include <string>
#include <vector>
#include <set>
#include <map>

/*LIBRERIAS
* Aqui vive TODA la informacion de las librerias, asi que para agregar una nueva
* solo se toca este archivo:
*   - librerias conocidas  -> agregar su nombre en librerias()
*   - funciones            -> agregar una linea en funcionesDeLibrerias()
* El analizador semantico usa estas tablas cuando ve  #include <...>  o una llamada como sqrt(x)
*/

//Una funcion que ofrece una libreria: sqrt(x), pow(x, y), rand()...
struct FuncionLibreria {
    std::string nombre;          //sqrt
    std::string libreria;        //cmath
    std::string retorno;         //double
    std::string tipoParametro;   //tipo que se espera en TODOS sus parametros
    int numParametros;           //cuantos parametros recibe
};

//Nombre especial: incluir esta libreria equivale a incluir todas las demas
inline std::string libreriaTodo() { return "bits/stdc++.h"; }

//Librerias que el compilador reconoce al ver #include <...>
inline const std::set<std::string>& librerias() {
    static const std::set<std::string> lista = {
        "iostream", "cmath", "cstdlib", "cstdio", "cstring", "string", "vector", "iomanip",
        "algorithm", "ctime", "cctype", "climits", "cfloat", "cassert", "fstream", "sstream",
        "map", "set", "queue", "stack", "list", "array", "numeric", "limits", "bitset",
        "random", "chrono", "utility", "functional", "bits/stdc++.h"
    };
    return lista;
}

//Funciones de cada libreria que el compilador sabe revisar (nombre, libreria, retorno, tipo de parametros, cuantos)
inline const std::vector<FuncionLibreria>& funcionesDeLibrerias() {
    static const std::vector<FuncionLibreria> tabla = {
        //<cmath>
        {"sqrt",  "cmath", "double", "double", 1},
        {"pow",   "cmath", "double", "double", 2},
        {"sin",   "cmath", "double", "double", 1},
        {"cos",   "cmath", "double", "double", 1},
        {"tan",   "cmath", "double", "double", 1},
        {"log",   "cmath", "double", "double", 1},
        {"log10", "cmath", "double", "double", 1},
        {"exp",   "cmath", "double", "double", 1},
        {"floor", "cmath", "double", "double", 1},
        {"ceil",  "cmath", "double", "double", 1},
        {"round", "cmath", "double", "double", 1},
        {"fabs",  "cmath", "double", "double", 1},
        {"abs",   "cmath", "double", "double", 1},
        //<cstdlib>
        {"abs",   "cstdlib", "int",  "int", 1},
        {"rand",  "cstdlib", "int",  "int", 0},
        {"srand", "cstdlib", "void", "int", 1},
        {"exit",  "cstdlib", "void", "int", 1},
        //<ctime>
        {"time",  "ctime", "int", "int", 1}
    };
    return tabla;
}

//Convierte los nombres estilo C (math.h) al nombre de C++ (cmath)
inline std::string normalizarLibreria(const std::string& nombre) {
    static const std::map<std::string, std::string> alias = {
        {"math.h", "cmath"}, {"stdlib.h", "cstdlib"}, {"stdio.h", "cstdio"}, {"string.h", "cstring"},
        {"time.h", "ctime"}, {"ctype.h", "cctype"}, {"limits.h", "climits"}, {"assert.h", "cassert"},
        {"float.h", "cfloat"}
    };
    std::map<std::string, std::string>::const_iterator it = alias.find(nombre);
    return it == alias.end() ? nombre : it->second;
}

inline bool esLibreriaConocida(const std::string& nombre) {
    return librerias().count(nombre) > 0;
}

//Distancia de edicion entre dos palabras (cuantas letras hay que cambiar, quitar o poner)
inline int distanciaEdicion(const std::string& a, const std::string& b) {
    std::vector<int> anterior(b.size() + 1), actual(b.size() + 1);
    for (size_t j = 0; j <= b.size(); j++) anterior[j] = (int)j;
    for (size_t i = 1; i <= a.size(); i++) {
        actual[0] = (int)i;
        for (size_t j = 1; j <= b.size(); j++) {
            int costo = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int borrar = anterior[j] + 1, poner = actual[j - 1] + 1, cambiar = anterior[j - 1] + costo;
            int menor = borrar < poner ? borrar : poner;
            actual[j] = menor < cambiar ? menor : cambiar;
        }
        anterior = actual;
    }
    return anterior[b.size()];
}

//Si la libreria esta mal escrita (iostrem), devuelve la conocida mas parecida (iostream)
//Si ninguna se parece (a lo mucho 2 letras de diferencia) devuelve una cadena vacia
inline std::string sugerirLibreria(const std::string& nombre) {
    std::string mejor = "";
    int mejorDistancia = 3;
    for (const std::string& l : librerias()) {
        int d = distanciaEdicion(nombre, l);
        if (d < mejorDistancia) { mejorDistancia = d; mejor = l; }
    }
    return mejor;
}

//Todas las funciones de libreria que se llaman asi (abs existe en <cmath> y en <cstdlib>)
inline std::vector<FuncionLibreria> buscarFuncionesDeLibreria(const std::string& nombre) {
    std::vector<FuncionLibreria> encontradas;
    for (const FuncionLibreria& f : funcionesDeLibrerias()) {
        if (f.nombre == nombre) encontradas.push_back(f);
    }
    return encontradas;
}
