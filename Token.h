#pragma once
//Token.h - Tipos de tokens, estructura Token y conversión a texto

#include <string>

//Tipos de tokens
enum class TipoToken {
    NUMERO,                 //67
    NUMERO_DECIMAL,         //3.14
    CADENA,                 //"Hello, Prof. Moisés"
    CARACTER,               //'Hola'
    IGUAL_IGUAL,            //==
    DIFERENTE_DE,           // !=
    MENOR_IGUAL,            //<=
    MAYOR_IGUAL,            //>=
    MENOR,                  //<
    MAYOR,                  //>
    NEGACION,               // !
    AND,                    // &&
    OR,                     // ||
    IDENTIFICADOR,          //Variable
    PALABRA_RESERVADA,      //if - else
    ASIGNACION,             //=
    ASIGNACION_COMPUESTA,   //+=  -=  *=  /=  %=
    PUNTO_COMA,             //;
    DOS_PUNTOS,             //:
    COMA,                   //,
    PUNTO,                  //.
    PARENTESIS_IZQ,         //(
    PARENTESIS_DER,         //)
    CORCHETE_IZQ,           //[
    CORCHETE_DER,           //]
    LLAVE_IZQ,              //{
    LLAVE_DER,              //}
    SUMA,                   //+
    RESTA,                  //-
    MULT,                   //*
    DIV,                    // /
    MODULO,                 // %
    INCREMENTO,             // ++
    DECREMENTO,             // --
    DESPLAZ_IZQ,            // <<   (el de cout)
    DESPLAZ_DER,            // >>   (el de cin)
    ESPACIO,                //Identifica los espacios
    DESCONOCIDO,            //Identifica si hay una secuencia desconocida
    ERROR,                  //Identifica si hay un error
    FIN
};
//std::unordered_set
//Agruparemos varias variables de distinto nombre para identificarlos con un solo nombre
struct Token {
    TipoToken tipo;
    std::string valor;
    std::string sugerencia = ""; //Solo se llena en los ERROR de palabras reservadas mal escritas
};

//Aquí inprime
inline std::string tipoATexto(TipoToken tipo) {
    switch (tipo) {
        case TipoToken::NUMERO: return "NUMERO";
        case TipoToken::CADENA: return "CADENA";
        case TipoToken::CARACTER: return "CARACTER";
        case TipoToken::IGUAL_IGUAL: return "IGUAL_IGUAL";
        case TipoToken::DIFERENTE_DE: return "DIFERENTE";
        case TipoToken::MENOR_IGUAL: return "MENOR_IGUAL";
        case TipoToken::MAYOR_IGUAL: return "MAYOR_IGUAL";
        case TipoToken::MENOR: return "MENOR";
        case TipoToken::MAYOR: return "MAYOR";
        case TipoToken::NEGACION: return "NEGACION";
        case TipoToken::AND: return "AND";
        case TipoToken::OR: return "OR";
        case TipoToken::ASIGNACION: return "ASIGNACION";
        case TipoToken::ASIGNACION_COMPUESTA: return "ASIGNACION_COMPUESTA";
        case TipoToken::PUNTO_COMA: return "PUNTO_COMA";
        case TipoToken::DOS_PUNTOS: return "DOS_PUNTOS";
        case TipoToken::COMA: return "COMA";
        case TipoToken::PUNTO: return "PUNTO";
        case TipoToken::PARENTESIS_IZQ: return "PARENTESIS_IZQ";
        case TipoToken::PARENTESIS_DER: return "PARENTESIS_DER";
        case TipoToken::CORCHETE_IZQ: return "CORCHETE_IZQ";
        case TipoToken::CORCHETE_DER: return "CORCHETE_DER";
        case TipoToken::LLAVE_IZQ: return "LLAVE_IZQ";
        case TipoToken::LLAVE_DER: return "LLAVE_DER";
        case TipoToken::NUMERO_DECIMAL: return "NUMERO_DECIMAL";
        case TipoToken::IDENTIFICADOR: return "IDENTIFICADOR";
        case TipoToken::PALABRA_RESERVADA: return "PALABRA_RESERVADA";
        case TipoToken::SUMA: return "SUMA";
        case TipoToken::RESTA: return "RESTA";
        case TipoToken::MULT: return "MULT";
        case TipoToken::DIV: return "DIV";
        case TipoToken::MODULO: return "MODULO";
        case TipoToken::INCREMENTO: return "INCREMENTO";
        case TipoToken::DECREMENTO: return "DECREMENTO";
        case TipoToken::DESPLAZ_IZQ: return "DESPLAZ_IZQ";
        case TipoToken::DESPLAZ_DER: return "DESPLAZ_DER";
        case TipoToken::ESPACIO: return "ESPACIO";
        case TipoToken::DESCONOCIDO: return "DESCONOCIDO";
        case TipoToken::ERROR: return "ERROR";
        case TipoToken::FIN: return "FIN";
    }
    return "DESCONOCIDO";
}
