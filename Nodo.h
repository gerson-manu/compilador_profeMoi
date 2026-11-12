#pragma once
//Nodo.h - Nodo del árbol sintáctico y funciones auxiliares del árbol

#include <iostream>
#include <memory>
#include <string>
#include <vector>

//Nodo del arbol sintáctico
struct Nodo {
    std::string etiqueta;
    std::vector<std::shared_ptr<Nodo>> hijos;
    bool asignable = false;     //TRUE si es algo a lo que se le puede asignar un valor (x, arr[i])
};
using NodoPtr = std::shared_ptr<Nodo>;

inline NodoPtr crearNodo(const std::string& etiqueta) {
    auto n = std::make_shared<Nodo>();
    n->etiqueta = etiqueta;
    return n;
}

//Imprime el arbol
inline void imprimirArbol(const NodoPtr& nodo, int nivel = 0) {
    std::cout << std::string(nivel * 3, ' ') << nodo->etiqueta << "\n";
    for (const NodoPtr& h : nodo->hijos) {
        imprimirArbol(h, nivel + 1);
    }
}

//Devuelve TRUE si la palabra es int, float, char, etc
inline bool esNombreDeTipo(const std::string& p) {
    return p == "int" || p == "float" || p == "double" || p == "char" || p == "void" || p == "bool";
}
