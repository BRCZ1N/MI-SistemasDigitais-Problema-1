/**
 * @file main.c
 * @date 2021-10-17
 * @brief Arquivo principal do jogo
*/

#include "prototype.h"

/**
 * Função para verificar se o jogador perdeu o jogo
 * @param peca Peca
 * @return 0 se o jogador perdeu, 1 caso contrário  
*/
int checkLose(Peca *peca) {
    if (ball->ballPositionY > (SCREEN_Y - 25))
        return 0;
    return 1;
}

/**
 * Função para posicionar os elementos no jogo ao iniciar uma partida
 * @param peca Peça
 * @param score Pontuação
 * @return void
*/
void reset_data (Peca *peca, int *score) {

    peca->ballPositionX = SCREEN_X / 2;
    peca->ballPositionY = SCREEN_Y - 120;
    peca->ballSpeedX = 1;
    peca->ballSpeedY = 1;
    peca->collision = -1;

    *score = 0;

}

int main() {

    /*Inicializar dos perifericos*/

    /*Botões*/
    int state_game, buttons;
    KEY_open    ();
    KEY_read    (&buttons);

    /*Acelerômetro*/
    int ready, tap, dtap, velX, velY, velZ, mg_per_lsb;
    accel_open      ();
    accel_init      ();
    accel_calibrate ();

    /*Vídeo*/
    video_open();

    /* Inicializar os elementos do jogo */
    int score;
    Block blocksList[QUANTITY_BLOCKS];
    Peca peca;

    /*Loop principal do jogo*/
    while (1) { 
    
        /*Posiciona elementos e iniciar a maquina de estado da tela*/
    	state_game = 0;         
    	reset_data (&ball, &bar, &score);
	    makeBlocks (blocksList);
    	
        /*Loop da patida do jogo*/
        while (checkLose(&ball)) {  // Enquanto o jogador não perdeu e não ganhou o jogo
        
            // Leitura dos botões e mudança de estado
            KEY_read        (&buttons);
            change_state    (&state_game, &buttons);

            video_clear     ();
            video_erase     ();
            
            /* switch para mudar a tela de acordo com o estado */
            if (state_game == 0) {          // Tela inicial

                /*Desenhar elementos do jogo*/ 
                reset_data  (&ball, &bar, &score);
                makeBlocks  (blocksList);
                create_menu ();
                video_show  ();
            
            } else if (state_game == 1) {       // Tela do jogo
            
                /*Desenhar elementos do jogo*/       
                game_field  (blocksList, score, state_game);
                bola9x9     (ball.ballPositionX, ball.ballPositionY, 0xffe0);
                video_box   (bar.coordX - BAR_SIZE, bar.coordY - BAR_WIDHT, bar.coordX + BAR_SIZE,bar.coordY + BAR_WIDHT , 0xfc18);
                video_show  ();

                /*Leitura do acelerômetro */
                accel_read  (&ready, &tap, &dtap, &velX, &velY, &velZ, &mg_per_lsb);

                /*Movimentação dos elementos do jogo*/
                moveBar     (&bar, velX); 
                moveBall    (&ball, blocksList, &bar);

                /*Verificação de colisão e remoção de blocos*/
                removeBlocks(blocksList, &score, &ball);
            
            } else if (state_game == 2) { // Estado de pausa

                /*Desenhar elementos do jogo*/ 
                game_field  (blocksList, score, state_game);
                bola9x9     (ball.ballPositionX, ball.ballPositionY, 0xFC18);
                video_box   (bar.coordX - BAR_SIZE, bar.coordY - BAR_WIDHT, bar.coordX + BAR_SIZE,bar.coordY + BAR_WIDHT , 0xFC18);
                video_show  ();
            
            } else if (state_game == 3) {   // Tela de pause/exit

                /*Desenhar elementos do jogo*/ 
                game_field  (blocksList, score, state_game);
                bola9x9     (ball.ballPositionX, ball.ballPositionY, 0xFC18);
                video_box   ( bar.coordX - BAR_SIZE, bar.coordY - BAR_WIDHT, bar.coordX + BAR_SIZE,bar.coordY + BAR_WIDHT , 0xFC18);
                video_show  ();
            }
        }
        /*Finalização do jogo*/

        if (checkLose(&ball) == 0) { // Se o jogador perdeu     
            do {
                KEY_read(&buttons);  // Aguarda o jogador apertar o botão 1 para voltar ao menu

                video_clear();
                video_erase();
                screen_defeat(score);
                video_show();
            } while (buttons != 1);
        }
        else {                  // Se o jogador ganhou
            screen_victory();
        }
    }
   
    return 0;
}

