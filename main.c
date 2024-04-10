#include <stdint.h>

	#define RCC_C 0X40021000U //DECLARA A LOS REGISTROS DEL RELOJ

	#define PORTA 0x50000000U //DECLARA EL PUERTO A

	#define PORTB 0X50000400U //DECLARA EL PUERTO B

	#define PORTC 0X50000800U //DECLARAR EL PUERTO C

	#define PERIPHERAL_BASE_ADDRESS 0X40000000U  // BASE DE LOS PERIFERICOS

	#define AHB_BASE_ADDRESS (PERIPHERAL_BASE_ADDRESS + 0X00020000U) // BASE DEL AHB

	#define APB1_BASE_ADDRESS (PERIPHERAL_BASE_ADDRESS + 0X00000000U) // BASE DEL APB1

    #define USART2_BASE_ADDRESS ( APB1_BASE_ADDRESS + 0x00004400U) // USART2
    #define USART_ISR_RXNE (1 << 5)  // RXNE bit en el registro ISR del USART2
    #define USART_ISR_TXE (1 << 7) //

	//CREAR ESTRUCTURA DEL GPIO

	typedef struct
	{
			uint32_t MODER;
			uint32_t OTYPER;
			uint32_t OSPEEDR;
			uint32_t PUPDR;
			uint32_t IDR;
			uint32_t ODR;
			uint32_t BSRR;
			uint32_t LCKR;
			uint32_t AFR[2];
			uint32_t BRR;

	}GPIO_RegDef_t;

	//CREAR ESTRUCTURA DEL RCC

	typedef struct{
			uint32_t CR;
			uint32_t ICSCR;
			uint32_t CRRCR;
			uint32_t CFGR;
			uint32_t CIER;
			uint32_t CIFR;
			uint32_t CICR;
			uint32_t IOPRSTR;
			uint32_t AHBRSTR;
			uint32_t APB2RSTR;
			uint32_t APB1RSTR;
			uint32_t IOPENR;
			uint32_t AHBENR;
			uint32_t APB2ENR;
			uint32_t APB1ENR;
			uint32_t IOPSMENR;
			uint32_t AHBSMENR;
			uint32_t APB2SMENR;
			uint32_t APB1SMENR;
			uint32_t CCIPR;
			uint32_t CSR;
	}RCC_RegDef_t;

	typedef struct{
		uint32_t CR1;
		uint32_t CR2;
		uint32_t CR3;
		uint32_t BRR;
		uint32_t GTPR;
		uint32_t RTOR;
		uint32_t RQR;
		uint32_t ISR;
		uint32_t ICR;
		uint32_t RDR;
		uint32_t TDR;
	}USART_RegDef_t;

	// ESTRUCTURA PARA LLEVAR CONTROL DE VARIBALES DE TIEMPO
	struct Time_t {

		int8_t hora_decimal;
		int8_t hora_unidad ;

		int8_t minuto_decimal ;
		int8_t minuto_unidad  ;

		int8_t segundo_decimal ;
		int8_t segundo_unidad ;


	};
	struct alarm {

		uint8_t hora_decimal;
		uint8_t hora_unidad ;

		uint8_t minuto_decimal ;
		uint8_t minuto_unidad  ;

		uint8_t segundo_decimal ;
		uint8_t segundo_unidad ;


	};
	struct temporizador{
			uint8_t hora_decimal ;
			uint8_t hora_unidad ;

			uint8_t minuto_decimal ;
			uint8_t minuto_unidad ;

			uint8_t segundo_decimal ;
			uint8_t segundo_unidad ;
	};

	#define GPIOA 	((GPIO_RegDef_t*)PORTA)
	#define GPIOB 	((GPIO_RegDef_t*)PORTB)
	#define GPIOC 	((GPIO_RegDef_t*)PORTC)
	#define RCC 	((RCC_RegDef_t*)RCC_C)
	#define USART2 	((USART_RegDef_t*)USART2_BASE_ADDRESS)


void ConsultaSaldo();
void Deposito();
void Retiro();
void USART_SendString(char *str);
void USART2_IRQHandler();
void delay_ms(uint16_t n);
uint16_t saldo = 980; // Saldo inicial

// Declaraciones de funciones
void USART_SendChar(char c);
void USART_SendInt(uint16_t num);
void procesarComando(char comando);

int main(void) {
    // Configuración de puertos GPIO

    RCC->IOPENR |= 1 << 0; // Habilitar el reloj para el puerto A

    GPIOA->MODER &= ~(1 << 11); // Configurar PA5 como entrada

    // Configuración de pines USART2
    // Alternar la función de los pines PA2 y PA3 al modo alternativo AF4
    GPIOA->MODER &= ~(1 << 4); // Limpiar el bit 4 (PA2)
    GPIOA->MODER &= ~(1 << 6); // Limpiar el bit 6 (PA3)
    GPIOA->AFR[0] |= 1 << 10;   // Mapear PA2 a AF4
    GPIOA->AFR[0] |= 1 << 14;   // Mapear PA3 a AF4

    // Configuración del periférico USART2
    RCC->APB1ENR |= 1 << 17;  // Habilitar el reloj para USART2
    USART2->BRR = 218;        // Establecer la velocidad de transmisión a 9600 baudios
    USART2->CR1 |= (1 << 2) | (1 << 3); // Habilitar transmisor y receptor
    USART2->CR1 |= 1 << 0;    // Habilitar el periférico USART2

    while (1) {
        // Esperar a que llegue un comando por USART
        while (!(USART2->ISR & USART_ISR_RXNE)); // Esperar a que llegue un byte
        char comando = USART2->RDR; // Leer el byte recibido

        procesarComando(comando);
    }
}

// Función para enviar un carácter a través de USART
void USART_SendChar(char c) {
    while (!(USART2->ISR & USART_ISR_TXE)); // Esperar a que el registro de transmisión esté vacío
    USART2->TDR = c; // Enviar carácter
}

// Función para enviar una cadena de caracteres a través de USART
void USART_SendString(char *str) {
    // Iterar sobre la cadena de caracteres hasta encontrar el carácter nulo '\0'
    while (*str != '\0') {
        // Enviar el carácter actual a través de USART
        USART_SendChar(*str++);
    }
}

// Función para enviar un entero a través de USART
void USART_SendInt(uint16_t num) {
    char buffer[10]; // Se reserva un espacio para el valor máximo de un uint16_t (5 dígitos) y el carácter nulo final
    sprintf(buffer, "%u\n", (unsigned int)num); // Se utiliza %u para enteros sin signo
    USART_SendString(buffer);
}

// Función para procesar el comando recibido por USART
void procesarComando(char comando) {
    switch(comando) {
        case 'S':
            ConsultaSaldo();
            break;
        case 'D':
            Deposito();
            break;
        case 'R':
            Retiro();
            break;
        default:
            USART_SendString("Comando Invalido.\n");
            break;
    }
}

// Función para realizar un depósito
void Deposito() {
    saldo += 100; // Monto fijo para el depósito
    USART_SendString("Deposito exitoso - Saldo actual: ");
    USART_SendInt(saldo);
}

// Función para realizar un retiro
void Retiro() {
    int montoRetiro = 250; // Monto fijo para el retiro

    if (saldo >= montoRetiro) {
        saldo -= montoRetiro; // Restar monto del saldo
        USART_SendString("Retiro exitoso - Saldo actual: ");
        USART_SendInt(saldo);
    } else {
        USART_SendString("Saldo insuficiente.\n");
    }
}

// Función para consultar saldo
void ConsultaSaldo() {
    USART_SendString("Saldo actual: ");
    USART_SendInt(saldo);
}

// Función de interrupción para USART2
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE) { // Hay un byte recibido
        char comando = USART2->RDR; // Leer el byte recibido
        procesarComando(comando); // Procesar el comando recibido
    }
}
