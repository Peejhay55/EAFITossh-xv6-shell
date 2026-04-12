#include "kernel/types.h"
#include "user/user.h"

#define MAX_CHARS 80

typedef struct {
	int char_count;
	int done;
	char buffer[MAX_CHARS];
} SharedData;

static void
print_char_info(const char *actor, int idx, int total, char *slot, int counter)
{
	unsigned char v;

	v = (unsigned char)*slot;
	printf("[%s] idx=%d/%d VA=%p valor='%c' (0x%x) contador=%d\n",
	       actor, idx + 1, total, slot, *slot, v, counter);
}

// Proceso principal: escribe el texto en memoria compartida caracter a caracter.
void
produce_chars(SharedData *shm, const char *text)
{
	int n;
	char *slot;

	n = strlen(text);
	if(n > MAX_CHARS)
		n = MAX_CHARS;

	printf("[Principal] Base SHM=%p | VA contador=%p | VA done=%p | VA buffer=%p\n",
	       shm, &shm->char_count, &shm->done, &shm->buffer[0]);

	for(int i = 0; i < n; i++){
		slot = &shm->buffer[i];
		*slot = text[i];
		shm->char_count++;
		print_char_info("Principal/Escritura", i, n, slot, shm->char_count);
		print_char_info("Principal/Lectura ", i, n, slot, shm->char_count);
		pause(15);
	}

	shm->done = 1;
	printf("[Principal] done=1 en VA=%p\n", &shm->done);
}

// Renderizador: consume lo que el principal va publicando en el buffer.
void
run_renderer(SharedData *shm)
{
	int rendered;
	char *slot;
	int available;

	rendered = 0;
	printf("[Renderizador] Esperando caracteres compartidos...\n");
	printf("[Renderizador] Base SHM=%p | VA contador=%p | VA done=%p | VA buffer=%p\n",
	       shm, &shm->char_count, &shm->done, &shm->buffer[0]);

	for(;;){
		while(rendered < shm->char_count){
			slot = &shm->buffer[rendered];
			print_char_info("Render/Lectura    ", rendered, MAX_CHARS, slot,
			                shm->char_count);
			rendered++;
			pause(20);
		}

		if(shm->done){
			printf("[Renderizador] El Principal finalizo, pero el Render sigue activo.\n");
			break;
		}

		pause(5);
	}

	available = shm->char_count;
	printf("[Renderizador] Relectura final desde SHM tras salida del Principal.\n");
	for(int i = 0; i < available; i++){
		slot = &shm->buffer[i];
		print_char_info("Render/Post-Exit  ", i, available, slot, shm->char_count);
		pause(10);
	}

	printf("\n[Renderizador] Render finalizado sin perder acceso a memoria.\n");
}

int
main(void)
{
	SharedData *shm;
	char input[MAX_CHARS + 2];
	int pid;
	int status;

	printf("\n=== Simulacion de Memoria Compartida (Shared Memory) ===\n");
	printf("Explicacion del flujo en ejecucion:\n");
	printf("1. El Padre (Renderizador) abre o crea la memoria compartida mediante shm_open().\n");
	printf("2. El Padre hace fork() para crear al Hijo (Principal). \n");
	printf("   Ambos procesos heredan la misma tabla de paginas inicial pero mantienen \n");
	printf("   su acceso a la direccion fisica compartida.\n");
	printf("3. El Principal (Hijo) escribe caracteres en la memoria y termina, cerrando su conexion.\n");
	printf("4. La memoria NO se libera porque el Renderizador (Padre) sigue conectado a ella.\n");
	printf("5. El Renderizador finaliza su trabajo, cierra la memoria y sale.\n\n");

	printf("Escribe un caracter o una cadena (max %d): ", MAX_CHARS);
	gets(input, sizeof(input));

	for(int i = 0; input[i] != 0; i++){
		if(input[i] == '\n'){
			input[i] = 0;
			break;
		}
	}

	if(input[0] == 0)
		strcpy(input, "SIN_ENTRADA");

	printf("\n[Sistema] Inicializando memoria compartida (shm_open)...\n");

	shm = (SharedData *)shm_open();
	if(shm == (void *)-1){
		printf("Error: shm_open fallo\n");
		exit(1);
	}

	printf("[Sistema] PID Padre=%d\n", getpid());
	printf("[Sistema] Memoria compartida asignada en VA=%p\n", shm);

	pid = fork();
	if(pid < 0){
		printf("Error: fork fallo\n");
		exit(1);
	}

	// Hijo = Principal. Padre = Renderizador.
	// Asi evitamos que aparezca el prompt "$" del shell en medio del render.
	if(pid == 0){
		// Como uvmcopy (al hacer fork) no clona direcciones altas por encima de p->sz,
		// el proceso hijo debe "vincularse" explícitamente a la memoria compartida.
		printf("[Principal] PID Hijo=%d\n", getpid());
		shm = (SharedData *)shm_open();
		if(shm == (void *)-1){
			printf("Error: Hijo fallo shm_open\n");
			exit(1);
		}

		shm->char_count = 0;
		shm->done = 0;

		printf("\n--- Fase 1: Proceso Principal escribe y comparte caracteres ---\n");
		printf("[Principal] Texto ingresado: %s\n", input);
		produce_chars(shm, input);

		printf("\n--- Fase 2: Principal termina operacion ---\n");
		printf("[Principal] Cerrando conexion a memoria compartida (shm_close) y finalizando.\n");
		shm_close();
		exit(0);
	}

	printf("\n--- Fase 3: Renderizador lee de memoria (aun sin que el Principal termine) ---\n");
	run_renderer(shm);

	// Recolecta al hijo para cerrar limpio y sin artefactos de consola.
	wait(&status);
	shm_close();

	printf("=== Flujo completado sin perdida de memoria compartida ===\n\n");
	exit(0);
}


