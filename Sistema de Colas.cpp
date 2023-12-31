/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h" // Encabezado para el generador de números aleatorios
#include <iostream>

#define LIMITE_COLA 0      // Capacidad máxima de la cola
#define LIMITE_SERVIDORES 5
#define OCUPADO 1          // Indicador de Servidor Ocupado
#define LIBRE 0            // Indicador de Servidor Libre

int sig_tipo_evento, num_esperas_requerido, num_eventos,
    num_entra_cola, num_servidores_ocupados, total_clientes, perdidas;
float area_num_entra_cola, area_estado_servidor, media_entre_llegadas, media_atencion,
      tiempo_simulacion, tiempo_ultimo_evento, tiempo_sig_evento[3],
      total_de_esperas, tiempo_salida[LIMITE_SERVIDORES + 1], area_clientes_sistema, total_de_atencion;
bool estado_servidor[LIMITE_SERVIDORES + 1];

FILE *parametros, *resultados;

// Funciones principales
void inicializar(void); // 1 de 6
void controltiempo(void); // 2 de 6
void llegada(void); // 3 de 6: evento 1
void salida(void); // 3 de 6: evento 2
void actualizar_estad_prom_tiempo(void); // 3 de 6: actualización estado
float expon(float mean); // 4 de 6: percentil
void reportes(void); // 5 de 6
int main(void); // 6 de 6

// Funciones auxiliares y de medidas de desempeño
double factorial(int n);
double mu_n(int n);
double p_0(void);
double p_n(int n);
double tp(void);
double mean_n(void);
double erlang_b(void);

double factorial(int n) {
    double f = 1;
    for (int k = 1; k <= n; k++) {
        f *= k;
    }
    return f;
}

double mu_n(int n) {
    return n * media_atencion;
}

double p_0(void) {
    double a = 1.0;
    for (int n = 1; n <= LIMITE_SERVIDORES; n++) {
        double b = 1.0 / factorial(n);
        b *= std::pow(media_entre_llegadas / media_atencion, n);
        a += b;
    }
    return 1.0 / a;
}

double p_n(int n) {
    double b = 1.0 / factorial(n);
    b *= std::pow(media_entre_llegadas / media_atencion, n);
    return b * p_0();
}

double tp(void) {
    double y = 0;
    for (int n = 1; n <= LIMITE_SERVIDORES; n++) {
        y += mu_n(n) * p_n(n);
    }
    return y;
}

double mean_n(void) {
    double mean_n = 0;
    for (int n = 1; n <= LIMITE_SERVIDORES; n++) {
        mean_n += n * p_n(n);
    }
    return mean_n;
}

double erlang_b(void) {
    double a = 1.0 / factorial(LIMITE_SERVIDORES);
    a *= std::pow(media_entre_llegadas / media_atencion, LIMITE_SERVIDORES);
    return a * p_0();
}


int main(void) /* Función Principal */
{
    /* Abre los archivos de entrada y salida */

    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");

    /* Especifica el numero de eventos para la función controltiempo. */

    num_eventos = 2;

    /* Lee los parametros de entrada. */

    fscanf(parametros, "%f %f %d", &media_entre_llegadas, &media_atencion, &num_esperas_requerido);

    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n", media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atención%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "Número de clientes%14d\n\n\n", num_esperas_requerido);

    /* Inicializa la simulación. */

    inicializar();

    /* Corre la simulación mientras no se llegue al número de clientes especificado en el archivo de entrada */

    while (total_clientes < num_esperas_requerido) {

        /* Determina el siguiente evento */

        controltiempo();

        /* Actualiza los acumuladores estadísticos de tiempo promedio */

        actualizar_estad_prom_tiempo();

        /* Invoca la función del evento adecuado. */

        switch (sig_tipo_evento) {
        case 1:
            llegada();
            break;
        case 2:
            salida();
            break;
        }
    }

    /* Invoca el generador de reportes y termina la simulación. */

    reportes();

    fclose(parametros);
    fclose(resultados);

    return 0;
}


void inicializar(void) /* Función de inicialización. */
{
    /* Inicializa el reloj de la simulación. */

    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */

    for (int i = 1; i <= LIMITE_SERVIDORES; i++)
        estado_servidor[i] = LIBRE;

    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    /* Inicializa los contadores estadísticos. */

    total_clientes = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;
    area_clientes_sistema = 0.0;
    total_de_atencion = 0.0;

    /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
       (terminación del servicio) no se tiene en cuenta */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;

    perdidas = 0;
}


void controltiempo(void) /* Función controltiempo */
{
    int i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    /* Determina el tipo de evento del evento que debe ocurrir. */

    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }

    /* Revisa si la lista de eventos está vacía. */

    if (sig_tipo_evento == 0) {

        /* La lista de eventos está vacía, se detiene la simulación. */

        fprintf(resultados, "\nLa lista de eventos está vacía %f", tiempo_simulacion);
        exit(1);
    }

    /* La lista de eventos no está vacía, adelanta el reloj de la simulación. */

    tiempo_simulacion = min_tiempo_sig_evento;
}


void llegada(void) /* Función de llegada */
{
    /* Programa la siguiente llegada. */
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

    /* Revisa si el servidor está OCUPADO. */

    if (num_servidores_ocupados == LIMITE_SERVIDORES) {
        ++perdidas;
    } else {
        /* El servidor está LIBRE, por lo tanto, el cliente que llega tiene tiempo de espera = 0
           (Las siguientes dos líneas del programa son para claridad y no afectan
           el resultado de la simulación) */

        /* Incrementa el número de clientes en espera y pasa el servidor a ocupado */
        ++total_clientes;
        ++num_servidores_ocupados;

        /* Programa una salida (servicio terminado). */

        float tiempo_atencion = expon(media_atencion);
        float tiempo_evento = tiempo_simulacion + tiempo_atencion;
        total_de_atencion += tiempo_atencion;

        tiempo_sig_evento[2] = std::min(tiempo_sig_evento[2], tiempo_evento);
        for (int i = 1; i <= LIMITE_SERVIDORES; i++) {
            if (estado_servidor[i] == 0) {

                /* Pasa el servidor i a ocupado */
                estado_servidor[i] = OCUPADO;

                /* Programa la salida del servidor i */
                tiempo_salida[i] = tiempo_evento;

                break;
            }
        }
    }
}


void salida(void) /* Función de Salida. */
{
    /* La cola está vacía, pasa el servidor a LIBRE y
       no considera el evento de salida */
    --num_servidores_ocupados;

    /* Pasa el servidor con menor tiempo a LIBRE */
    for (int i = 1; i <= LIMITE_SERVIDORES; i++) {
        if (tiempo_salida[i] == tiempo_sig_evento[2]) {
            estado_servidor[i] = LIBRE;
            tiempo_salida[i] = 1.0e+30;
            break;
        }
    }

    /* Busca el siguiente servidor a salir */
    tiempo_sig_evento[2] = 1.0e+30;
    for (int i = 1; i <= LIMITE_SERVIDORES; i++)
        tiempo_sig_evento[2] = std::min(tiempo_sig_evento[2], tiempo_salida[i]);
}


void reportes(void)  /* Función generadora de reportes. */
{
    /* Calcula y estima los estimados de las medidas deseadas de desempeño */
    fprintf(resultados, "Espera promedio en la cola%11.3f minutos\n\n", total_de_esperas / total_clientes);
    fprintf(resultados, "Número promedio en cola%10.3f\n\n", area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Tiempo de terminación de la simulación%12.3f minutos\n\n\n", tiempo_simulacion);

    double throughput_simulado = area_estado_servidor / tiempo_simulacion / LIMITE_SERVIDORES;
    double num_medio_clientes_sistema_simulado = area_clientes_sistema / tiempo_simulacion;
    double tiempo_medio_clientes_sistema_simulado = total_clientes / total_de_atencion;
    double utilizacion_servidor_simulado = area_estado_servidor / total_de_atencion;
    double erlang_b_simulado = (double)perdidas / (double)num_esperas_requerido;

    double throughput_teorico = tp();
    double num_medio_clientes_sistema_teorico = mean_n();
    double tiempo_medio_clientes_sistema_teorico = mean_n() / tp();
    double utilizacion_servidor_teorico = 1.0 - p_0();
    double erlang_b_teorico = erlang_b();

    double throughput_error = std::abs(throughput_simulado - throughput_teorico) / throughput_teorico;
    double num_medio_clientes_sistema_error = std::abs(num_medio_clientes_sistema_simulado - num_medio_clientes_sistema_teorico) / num_medio_clientes_sistema_teorico;
    double tiempo_medio_clientes_sistema_error_1 = std::abs(tiempo_medio_clientes_sistema_simulado - tiempo_medio_clientes_sistema_teorico) / tiempo_medio_clientes_sistema_teorico;
    double utilizacion_servidor_error = std::abs(utilizacion_servidor_simulado - utilizacion_servidor_teorico) / utilizacion_servidor_teorico;
    double erlang_b_error = std::abs(erlang_b_simulado - erlang_b_teorico) / erlang_b_teorico;

    double tiempo_medio_clientes_sistema_excel = 6.162095589;
    double tiempo_medio_clientes_sistema_error_2 = std::abs(tiempo_medio_clientes_sistema_simulado - tiempo_medio_clientes_sistema_excel) / tiempo_medio_clientes_sistema_excel;

    fprintf(resultados, "Throughput simulado%10.3f\n\n", throughput_simulado);
    fprintf(resultados, "Throughput teórico%10.3f\n\n", throughput_teorico);
    fprintf(resultados, "Throughput error%10.3f\n\n\n", throughput_error);

    fprintf(resultados, "Número medio clientes sistema simulado%10.3f\n\n", num_medio_clientes_sistema_simulado);
    fprintf(resultados, "Número medio clientes sistema teórico%10.3f\n\n", num_medio_clientes_sistema_teorico);
    fprintf(resultados, "Número medio clientes sistema error%10.3f\n\n\n", num_medio_clientes_sistema_error);

    fprintf(resultados, "Tiempo medio clientes sistema simulado%10.3f\n\n", tiempo_medio_clientes_sistema_simulado);
    fprintf(resultados, "Tiempo medio clientes sistema teórico%10.3f\n\n", tiempo_medio_clientes_sistema_teorico);
    fprintf(resultados, "Tiempo medio clientes sistema excel%10.3f\n\n", tiempo_medio_clientes_sistema_excel);
    fprintf(resultados, "Tiempo medio clientes sistema error 1%10.3f\n\n", tiempo_medio_clientes_sistema_error_1);
    fprintf(resultados, "Tiempo medio clientes sistema error 2%10.3f\n\n\n", tiempo_medio_clientes_sistema_error_2);

    fprintf(resultados, "Utilización servidor simulado%10.3f\n\n", utilizacion_servidor_simulado);
    fprintf(resultados, "Utilización servidor teórico%10.3f\n\n", utilizacion_servidor_teorico);
    fprintf(resultados, "Utilización servidor teórico error%10.3f\n\n\n", utilizacion_servidor_error);

    fprintf(resultados, "Erlang B simulado%10.3f\n\n", erlang_b_simulado);
    fprintf(resultados, "Erlang B teórico%10.3f\n\n", erlang_b_teorico);
    fprintf(resultados, "Erlang B error%10.3f\n\n\n", erlang_b_error);
}


/* Actualiza los acumuladores de área para las estadísticas de tiempo promedio. */
void actualizar_estad_prom_tiempo(void)  								
{
    float time_since_last_event;

    /* Calcula el tiempo desde el último evento y actualiza el marcador del último evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    /* Actualiza el área bajo la función de número_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    /* Verifica si un servidor está activo */
    int servidor_activo = 0;
    for (int i = 1; i <= LIMITE_SERVIDORES; i++)
        if (estado_servidor[i] == OCUPADO)
            servidor_activo = 1;

    /* Actualiza el área bajo la función de número de clientes en el sistema */
    area_clientes_sistema += (num_entra_cola + num_servidores_ocupados) * time_since_last_event;

    /* Actualiza el área bajo la función indicadora de servidor ocupado */
    area_estado_servidor += servidor_activo * time_since_last_event;
}

float expon(float media)  /* Función generadora de la exponencial */
{
    /* Retorna una variable aleatoria exponencial con media "media" */

    return -media * log(lcgrand(1));
}

