#pragma once
// ManejadorArchivos.h - Fase 7: lectura de archivos de codigo fuente
// (sin acentos a proposito, para que se vea igual en Code::Blocks y en CLion)

#include <iostream>
#include <fstream>   // ifstream
#include <sstream>   // ostringstream
#include <string>
#include <cctype>

// Quita espacios y saltos de linea de los extremos.
// Tambien quita las comillas, porque al arrastrar un archivo a la consola
// de Windows la ruta puede venir como "C:\carpeta\programa.cpp"
inline std::string limpiarRuta(std::string ruta) {
    const std::string blancos = " \t\r\n";
    size_t inicio = ruta.find_first_not_of(blancos);
    if (inicio == std::string::npos) return "";
    size_t fin = ruta.find_last_not_of(blancos);
    ruta = ruta.substr(inicio, fin - inicio + 1);

    if (ruta.size() >= 2 && ruta.front() == '"' && ruta.back() == '"') {
        ruta = ruta.substr(1, ruta.size() - 2);
    }
    return ruta;
}

// Devuelve true si el nombre termina en .cpp (sin importar mayusculas)
inline bool terminaEnCpp(const std::string& ruta) {
    if (ruta.size() < 4) return false;
    std::string extension = ruta.substr(ruta.size() - 4);
    for (size_t i = 0; i < extension.size(); i++) {
        extension[i] = (char)std::tolower((unsigned char)extension[i]);
    }
    return extension == ".cpp";
}

// Lee un archivo .cpp completo y lo guarda en 'contenido'.
// Devuelve true si todo salio bien. Si falla, muestra el motivo y devuelve false.
inline bool leerArchivo(const std::string& rutaEscrita, std::string& contenido) {
    contenido.clear();
    std::string ruta = limpiarRuta(rutaEscrita);

    if (ruta.empty()) {
        std::cout << "Error: no se escribio ningun nombre de archivo.\n";
        return false;
    }

    if (!terminaEnCpp(ruta)) {
        std::cout << "Error: el archivo de entrada debe tener extension .cpp (se recibio \"" << ruta << "\").\n";
        return false;
    }

    std::ifstream archivo(ruta.c_str());
    if (!archivo.is_open()) {
        std::cout << "Error: no se pudo abrir el archivo \"" << ruta << "\".\n"
                  << "Revise que el nombre este bien escrito, que el archivo exista\n"
                  << "y que este en la carpeta desde donde se ejecuta el programa (o escriba la ruta completa).\n";
        return false;
    }

    // Se vuelca todo el contenido del archivo en un string
    std::ostringstream buffer;
    buffer << archivo.rdbuf();

    if (archivo.bad()) {
        std::cout << "Error: ocurrio un problema al leer el archivo \"" << ruta << "\".\n";
        return false;
    }

    contenido = buffer.str();

    // Algunos editores guardan los archivos UTF-8 con una marca invisible (BOM) al inicio.
    // Si no se quita, el lexer la tomaria como un simbolo desconocido.
    if (contenido.size() >= 3 &&
        (unsigned char)contenido[0] == 0xEF &&
        (unsigned char)contenido[1] == 0xBB &&
        (unsigned char)contenido[2] == 0xBF) {
        contenido.erase(0, 3);
    }

    if (contenido.empty()) {
        std::cout << "Error: el archivo \"" << ruta << "\" esta vacio.\n";
        return false;
    }

    std::cout << "Archivo \"" << ruta << "\" leido correctamente (" << contenido.size() << " caracteres).\n";
    return true;
}
