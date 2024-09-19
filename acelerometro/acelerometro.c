#include "acelerometro.h"
#include "mapa_endereco.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

// Ponteiros para os registradores I2C0 e SYSMGR (Gerenciador de Sistema)
static unsigned int * ponteiro_base_i2c, * ponteiro_gerenciador;
static void * base_i2c_vitural, * gerenciador_base_virtual;
static int pasta_i2c = -1, pasta_gerenciador = -1;

int main (){
    uint8_t devid;
    int16_t sensibilidade = 4;
    int16_t eixo_x;

    // Abrir e mapear o endereço físico do SYSMGR
    if ((pasta_gerenciador = Abrir_memoria_fisica(pasta_gerenciador)) == -1) {
		return(-1);
	}

	if ((gerenciador_base_virtual = Mapear_memoria_fisica(pasta_gerenciador, SYSMGR_BASE, SYSMGR_SPAN)) == NULL) {
		return(-1);
	}

	ponteiro_Gerenciador_sistema = (unsigned int *) gerenciador_base_virtual;

    // Abrir e mapear o endereço físico do I2C0
    if ((pasta_i2c = Abrir_memoria_fisica(pasta_i2c)) == -1) {
		return(-1);
	}

	if ((base_i2c_vitural = Mapear_memoria_fisica(pasta_i2c, I2C0_BASE, I2C0_SPAN)) == NULL) {
		return(-1);
	}

	ponteiro_base_i2c = (unsigned int *) base_i2c_vitural;

    // Configurar Mux para usar i2c
    Configurar_mux();
    printf("Mux Selecionado protocolo I2C\n");
    
    // Inicializar o Controlador I2C0
    Inicializar_I2C0();
    printf("Inicializando protocolo de comunicação I2C0\n");
    
    // 0xE5 é lido de DEVID(0x00) se o I2C estiver funcionando corretamente
    Ler_REG(0x00, &devid);
    
    // Verificar ID do dispositivo
    if (devid == 0xE5){
        printf("Foi inicializado corretamente\n");
        // Inicializar o acelerômetro
        Inicializar_acelerometro();

        Calibrar();
        
        while(1){
            if (Dados_prontos()){
                eixo_x = Ler_X();
                printf("Eixo X = %d mg\n", eixo_x * sensibilidade);
            }
        }
    } else {
        printf("ID do dispositivo incorreto\n");
    }
    
    return 0;
}

// Configurar pin muxing (no sysmgr) para conectar os fios do Acelerômetro ao I2C0
void Configurar_mux(){
    // Desabilita o uso da interface FPGA para I2C0
    *(ponteiro_Gerenciador_sistema + SYSMGR_I2C0USEFPGA) = 0;
    // Configura GENERALIO7 para I2C0.SDA
    *(ponteiro_Gerenciador_sistema + SYSMGR_GENERALIO7) = 1;
    // Configura GENERALIO8 para I2C0.SCL
    *(ponteiro_Gerenciador_sistema + SYSMGR_GENERALIO8) = 1;
}

// Inicializa o controlador I2C0 para uso com o acelerômetro
void Inicializar_I2C0(){

    // Aborta qualquer transmissão em andamento e desabilita I2C0.
    *(ponteiro_base_i2c + I2C0_ENABLE) = 2;
    
    // Espera até que o I2C0 esteja desabilitado
    while(((*(ponteiro_base_i2c + I2C0_ENABLE_STATUS))&0x1) == 1){
        //zona critica
    }
    
    // Configura o registrador de configuração com as configurações desejadas (atuar como
    // mestre, usa endereçamento de 7 bits, modo rápido (400kb/s)).
    *(ponteiro_base_i2c + I2C0_CON) = 0x65;
    
    // Define endereço alvo (desabilitar comandos especiais, usar endereçamento de 7 bits)
    *(ponteiro_base_i2c + I2C0_TAR) = 0x53;
    
    // Define contagens altas/baixas do SCL (Assumindo clock de entrada de 100MHz no Controlador I2C0).
    // O período mínimo alto do SCL é 0.6us, e o período mínimo baixo do SCL é 1.3us,
    // No entanto, o período combinado deve ser de 2.5us ou maior, então adiciona-se 0.3us a cada.
    *(ponteiro_base_i2c + I2C0_FS_SCL_HCNT) = 60 + 30; // 0.6us + 0.3us
    *(ponteiro_base_i2c + I2C0_FS_SCL_LCNT) = 130 + 30; // 1.3us + 0.3us
    
    // Habilita o controlador
    *(ponteiro_base_i2c + I2C0_ENABLE) = 1;
    
    // Espera até que o controlador esteja habilitado
    while(((*(ponteiro_base_i2c + I2C0_ENABLE_STATUS))&0x1) == 0){
        //zona critica
    }
}

// Escreve valor no registrador interno no endereço especificado
void Ler_REG(uint8_t endereco, uint8_t *valor){
    
    // Envia endereço do registrador (+0x400 para enviar sinal START)
    *(ponteiro_base_i2c + I2C0_DATA_CMD) = endereco + 0x400;
    
    // Envia sinal de leitura
    *(ponteiro_base_i2c + I2C0_DATA_CMD) = 0x100;
    
    // Ler a resposta (primeiro espera até que o buffer RX contenha dados)  
    while (*(ponteiro_base_i2c + I2C0_RXFLR) == 0){
        // zona critica 
    }

    *valor = *(ponteiro_base_i2c + I2C0_DATA_CMD);
}

// Inicializar o acelerômetro
void Inicializar_acelerometro(){

    // Faixa de +- 16g, resolução total
    Escreve_REG(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
    
    // Taxa de Dados de Saída: 200Hz
    Escreve_REG(ADXL345_REG_BW_RATE, ADXL345_RATE_200);

    // Habilitando interrupções para atividade e inatividade
    Escreve_REG(ADXL345_REG_THRESH_ACT, 0x04);	// limiar de atividade
    Escreve_REG(ADXL345_REG_THRESH_INACT, 0x02);	// limiar de inatividade
    Escreve_REG(ADXL345_REG_TIME_INACT, 0x02);	// tempo para inatividade
    Escreve_REG(ADXL345_REG_ACT_INACT_CTL, 0xFF);	// Habilita acoplamento AC para limiares
    Escreve_REG(ADXL345_REG_INT_ENABLE, XL345_ACTIVITY | XL345_INACTIVITY );	// habilitar interrupções
    
    // Parar medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // Iniciar medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

// Calibrar o acelerômetro. O dispositivo deve ser colocado em uma superfície plana,
// e deve permanecer estacionário durante a calibração.
void Calibrar(){
    
    int media = 0;
    int16_t X;
    int8_t compensacao;
    
    // Para medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // Obtém os offsets atuais
    Ler_REG(ADXL345_REG_OFSX, (uint8_t *)&compensacao);
    
    // Usa taxa de 100 Hz para calibração. Salva a taxa atual.
    uint8_t taxa_salva;
    Ler_REG(ADXL345_REG_BW_RATE, &taxa_salva);
    Escreve_REG(ADXL345_REG_BW_RATE, XL345_RATE_100);
    
    // Usa faixa de 16g, resolução total. Salva o formato atual.
    uint8_t formato_de_dados_salvos;
    Ler_REG(ADXL345_REG_DATA_FORMAT, &formato_de_dados_salvos);
    Escreve_REG(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
    
    // Inicia medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_MEASURE);
    
    // Obtém as médias das acelerações de x sobre 32 amostras (LSB 3.9 mg)
    int i = 0;
    while (i < 32){
		if (Dados_prontos()){
            X = Ler_X();
            media += X;
            i++;
        }
    }

    media = Divisao_aproximada(media, 32);
    
    // Para medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // Calcula os offsets (LSB 15.6 mg)
    compensacao += Divisao_aproximada(0 - media, 4);
    
    // Define o registrador de offset X
    Escreve_REG(ADXL345_REG_OFSX, compensacao);
    
    // Restaura a taxa de largura de banda original
    Escreve_REG(ADXL345_REG_BW_RATE, taxa_salva);
    
    // Restaura o formato de dados original
    Escreve_REG(ADXL345_REG_DATA_FORMAT, formato_de_dados_salvos);
    
    // Inicia medição
    Escreve_REG(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

// Retorna verdadeiro se houve atividade desde a última leitura (verifica o bit ATIVIDADE).
bool Dados_prontos(){
	bool pronto = falso;
    uint8_t dados;
    
    Ler_REG(ADXL345_REG_INT_SOURCE, &dados);
    if (dados & XL345_ACTIVITY)
        pronto = verdadeiro;
    
    return pronto;
}

// Ler dados de aceleração do eixo X
int16_t Ler_X(){
    uint8_t dados[2];
    int16_t eixo_x;
    //se der erro nessa função trocar ADXL345_REG_DATAX0 por 0x32
    Multileitura_REG(ADXL345_REG_DATAX0, dados, sizeof(dados));

    eixo_x = (dados[1] << 8) | dados[0];
    return eixo_x;
}

// Escreve o valor no registrador interno no endereço especificado
void Escreve_REG(uint8_t endereco, uint8_t valor){
    
    // Envia endereço do registrador (+0x400 para enviar sinal START)
    *(ponteiro_base_i2c + I2C0_DATA_CMD) = endereco + 0x400;
    
    // Envia valor
    *(ponteiro_base_i2c + I2C0_DATA_CMD) = valor;
}

// Ler múltiplos registradores internos consecutivos para o eixo X
void Multileitura_REG(uint8_t endereco, uint8_t valores[], uint8_t tamanho){
    
    // Envia endereço do registrador (+0x400 para enviar sinal START)
    *(ponteiro_base_i2c + I2C0_DATA_CMD) = endereco + 0x400;
    
    // Envia sinal de leitura "n" vezes
    int i;
    for (i = 0; i < tamanho; i++)
        *(ponteiro_base_i2c + I2C0_DATA_CMD) = 0x100;

    // Ler os bytes
    int indice = 0;
    while (tamanho){
        if ((*(ponteiro_base_i2c + I2C0_RXFLR)) > 0){
            valores[indice] = *(ponteiro_base_i2c + I2C0_DATA_CMD);
            indice++;
            tamanho--;
        }
    }
}

// Abrir /dev/mem, se ainda não estiver aberto, para dar acesso a endereços físicos
int Abrir_memoria_fisica (int pasta_de_arquivo)
{
	if (pasta_de_arquivo == -1)
		if ((pasta_de_arquivo = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
		{
			printf ("ERRO: não foi possível abrir \"/dev/mem\"...\n");
			return (-1);
		}
	return pasta_de_arquivo;
}

// Fechar /dev/mem para dar acesso a endereços físicos
void Fechar_memoria_fisica (int pasta_de_arquivo){
	close (pasta_de_arquivo);
}

/*
 * Estabelece um mapeamento de endereço virtual para os endereços físicos começando pela base, e
 * estendendo-se pelo tamanho em bytes.
 */
void* Mapear_memoria_fisica(int pasta_de_arquivo, unsigned int base, unsigned int tamanho){
	void *virtual_base;

	// Faz um mapeamento dos endereços físicos para endereços virtuais
	virtual_base = mmap (NULL, tamanho, (PROT_READ | PROT_WRITE), MAP_SHARED, pasta_de_arquivo, base);

	if (virtual_base == MAP_FAILED) {
		printf ("ERRO: mmap() falhou...\n");
		close (pasta_de_arquivo);
		return (NULL);
	}
	return virtual_base;
}

/*
 * Fecha o mapeamento de endereço virtual previamente aberto
 */
int Desmapear_memoria_fisica(void * virtual_base, unsigned int tamanho){
	if (munmap (virtual_base, tamanho) != 0){
		printf ("ERRO: munmap() falhou...\n");
		return (-1);
	}
	return 0;
}

