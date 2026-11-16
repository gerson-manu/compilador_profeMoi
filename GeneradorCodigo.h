#pragma once
// GeneradorCodigo.h - Generación de código

#include <iostream>
#include <string>
#include <vector>

#include "CodigoIntermedio.h"

class GeneradorCodigo {

private:

    std::vector<std::string> codigoEnsamblador;

    // Agrega una instruccion al codigo generado
    void emitir(const std::string& instruccion) {
        codigoEnsamblador.push_back(instruccion);
    }

    // Carga un valor en un registro
    void cargarValor(
        const std::string& registro,
        const std::string& valor) {

        emitir("    LOAD " + registro + ", " + valor);
    }

    // Guarda el contenido de un registro
    void guardarValor(
        const std::string& destino,
        const std::string& registro) {

        emitir("    STORE " + destino + ", " + registro);
    }

    // Traduce operaciones aritmeticas y comparaciones
    void traducirOperacion(const Cuadrupla& q) {

        cargarValor("R1", q.arg1);
        cargarValor("R2", q.arg2);

        if (q.op == "+")
            emitir("    ADD R1, R2");

        else if (q.op == "-")
            emitir("    SUB R1, R2");

        else if (q.op == "*")
            emitir("    MUL R1, R2");

        else if (q.op == "/")
            emitir("    DIV R1, R2");

        else if (q.op == "%")
            emitir("    MOD R1, R2");

        else if (q.op == "==") {
            emitir("    CMP R1, R2");
            emitir("    SET_EQ R1");
        }

        else if (q.op == "!=") {
            emitir("    CMP R1, R2");
            emitir("    SET_NE R1");
        }

        else if (q.op == "<") {
            emitir("    CMP R1, R2");
            emitir("    SET_LT R1");
        }

        else if (q.op == ">") {
            emitir("    CMP R1, R2");
            emitir("    SET_GT R1");
        }

        else if (q.op == "<=") {
            emitir("    CMP R1, R2");
            emitir("    SET_LE R1");
        }

        else if (q.op == ">=") {
            emitir("    CMP R1, R2");
            emitir("    SET_GE R1");
        }

        else {
            emitir("    ; Operacion no soportada: " + q.op);
            return;
        }

        guardarValor(q.res, "R1");
    }

    // Traduce operaciones unarias
    void traducirUnario(const Cuadrupla& q) {

        cargarValor("R1", q.arg1);

        if (q.op == "-")
            emitir("    NEG R1");

        else if (q.op == "!")
            emitir("    NOT R1");

        else {
            emitir("    ; Operacion unaria no soportada");
            return;
        }

        guardarValor(q.res, "R1");
    }

    // Traduce asignaciones
    void traducirAsignacion(const Cuadrupla& q) {

        cargarValor("R1", q.arg1);
        guardarValor(q.res, "R1");
    }

    // Traduce saltos condicionales
    void traducirSalto(const Cuadrupla& q) {

        cargarValor("R1", q.arg1);
        emitir("    CMP R1, 0");

        if (q.op == "if")
            emitir("    JNE " + q.res);

        else if (q.op == "ifFalse")
            emitir("    JE " + q.res);
    }

    // Traduce cada cuadrupla
    void traducirCuadrupla(const Cuadrupla& q) {

        // Etiquetas
        if (q.op == "label") {
            emitir(q.res + ":");
        }

        // Saltos
        else if (q.op == "goto") {
            emitir("    JMP " + q.res);
        }

        else if (q.op == "if" || q.op == "ifFalse") {
            traducirSalto(q);
        }

        // Asignaciones
        else if (q.op == "=") {
            traducirAsignacion(q);
        }

        // Operaciones unarias
        else if (q.arg2.empty() &&
                 (q.op == "-" || q.op == "!")) {

            traducirUnario(q);
        }

        // Operaciones binarias
        else if (q.op == "+" || q.op == "-" ||
                 q.op == "*" || q.op == "/" ||
                 q.op == "%" || q.op == "==" ||
                 q.op == "!=" || q.op == "<" ||
                 q.op == ">" || q.op == "<=" ||
                 q.op == ">=") {

            traducirOperacion(q);
        }

        // Arreglos
        else if (q.op == "alloc") {
            emitir("    ALLOC " + q.res + ", " +
                   q.arg2 + ", " + q.arg1);
        }

        else if (q.op == "=[]") {
            cargarValor("R1", q.arg2);
            emitir("    LOAD_INDEX R2, " + q.arg1 + ", R1");
            guardarValor(q.res, "R2");
        }

        else if (q.op == "[]=") {
            cargarValor("R1", q.arg2);
            cargarValor("R2", q.res);
            emitir("    STORE_INDEX " + q.arg1 + ", R1, R2");
        }

        // Entrada y salida
        else if (q.op == "print") {
            emitir("    PRINT " + q.arg1);
        }

        else if (q.op == "read") {
            emitir("    READ " + q.res);
        }

        // Funciones y parametros
        else if (q.op == "func_begin") {
            emitir("\nFUNC " + q.res);
        }

        else if (q.op == "func_end") {
            emitir("END_FUNC " + q.res + "\n");
        }

        else if (q.op == "param_in") {
            emitir("    RECEIVE " + q.res);
        }

        else if (q.op == "param") {
            emitir("    PUSH " + q.arg1);
        }

        else if (q.op == "call") {
            emitir("    CALL " + q.arg1 + ", " + q.arg2);
            guardarValor(q.res, "R0");
        }

        // Retorno de funciones
        else if (q.op == "return") {

            if (!q.arg1.empty())
                cargarValor("R0", q.arg1);

            emitir("    RET");
        }

        else {
            emitir("    ; Instruccion no reconocida: " + q.op);
        }
    }

public:

    // Recibe las cuadruplas optimizadas
    void generarCodigo(
        const std::vector<Cuadrupla>& codigo) {

        codigoEnsamblador.clear();

        emitir(" ---- CODIGO ENSAMBLADOR GENERADO ----");
        emitir(" ---- Arquitectura educativa de registros ----");

        for (const auto& q : codigo) {
            traducirCuadrupla(q);
        }
    }

    // Imprime el codigo ensamblador
    void imprimirCodigo() const {

        std::cout << "\n***** CODIGO ENSAMBLADOR *****\n\n";

        for (const auto& instruccion : codigoEnsamblador) {
            std::cout << instruccion << "\n";
        }
    }

    // Permite consultar el codigo generado
    const std::vector<std::string>& obtenerCodigo() const {
        return codigoEnsamblador;
    }
};
