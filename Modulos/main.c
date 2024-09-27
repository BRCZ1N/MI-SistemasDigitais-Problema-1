#include "prototype.h"
int16_t axis_x;
pthread_mutex_t lock;

/*
     * Função principal que inicializa o ambiente do jogo Tetris.
     * Cria duas threads: 
     * - A primeira thread executa a função execAccel, que lida com o acelerômetro ou entrada de movimento.
     * - A segunda thread executa a função execTetris, que contém a lógica principal do jogo Tetris.
     * Um mutex é inicializado para garantir que o acesso a recursos compartilhados, como a variável axis_x, seja feito de maneira segura.
     * As threads são unidas ao final da execução, garantindo que o programa aguarde a conclusão de ambas antes de encerrar.
     */
int main()
{

    pthread_t thread1, thread2;

    pthread_mutex_init(&lock, NULL);

    pthread_create(&thread1, NULL, execAccel, NULL);

    pthread_create(&thread2, NULL, execTetris, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&lock);

    return 0;
}
/**
 * Função principal do jogo Tetris que controla a lógica do jogo.
 *
 * Esta função é executada em uma thread separada e gerencia o fluxo
 * do jogo Tetris, incluindo a inicialização do tabuleiro, controle
 * dos tetrominos, verificação de condições de jogo, e exibição
 * do estado do jogo na tela. O loop principal continua até que o
 * jogo esteja terminado, e ele permite alternar entre pausar e
 * retomar o jogo.
 *
 * @note A função utiliza variáveis globais para o controle do estado
 *       do jogo e acesso a recursos de vídeo.
 */
void execTetris()
{

    int state_game = 1, buttons, buttonValue, buttonValueRotate;
    int16_t mg_per_lsb = 4;

    srand(time(NULL));
    
    Tetromino currentTetromino;
    PartTetromino boardMatrix[LINES][COLUMNS];
    int dx = 0, dy = 1, moved = 1, score, hscore;
    char text_over[4] = "over";
    char text_paused[6] = "paused";
    char text_game[4] = "game";

    while (1)
    {
        video_open();
        video_clear();
        video_erase();
        video_close();

        score = 0;
        resetBoard(boardMatrix);
        initTetromino(&currentTetromino);
        int pointerStateGame = 1;

        while (!checkGameOver(boardMatrix, &currentTetromino))
        {
            buttonValue = buttonRead();

            changePauseState(&pointerStateGame, &buttonValue);

            if (pointerStateGame == 1)
            {

                pthread_mutex_lock(&lock);
                if (axis_x * mg_per_lsb >= 100)
                {

                    dx = 1;
                }
                else if (axis_x * mg_per_lsb <= -100)
                {

                    dx = -1;
                }
                else
                {

                    dx = 0;
                }
                pthread_mutex_unlock(&lock);
                moveTetromino(boardMatrix, &currentTetromino, dx, dy, &moved);
                dx = 0;
                if (!moved)
                {
                    removeFullLines(boardMatrix, &score);
                    initTetromino(&currentTetromino);
                }
                drawTetrominoTerminal(currentTetromino);
                video_open();
                video_clear();
                gameField(score, hscore);
                drawBoard(boardMatrix);
                video_show();
                video_close();
                usleep(150000);
            }
            else
            {
                video_open();
                video_clear();
                gameField(score, hscore);
                drawBoard(boardMatrix);
                generatePhrase(120, 2, text_paused, 6, COLOR_RED);
                video_show();
                video_close();
            }
            drawBoardTerminal(boardMatrix);
        }
        video_open();
        video_clear();
        gameField(score, hscore);
        drawBoard(boardMatrix);
        generatePhrase(2, 100, text_game, 4, COLOR_RED);
        generatePhrase(260, 100, text_over, 4, COLOR_RED);
        video_show();
        usleep(8000000);
        video_clear();
        video_close();
        if(score > hscore){

            hscore = score;

        }
    }
}
