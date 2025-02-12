#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
   int play()
{
    auto start = chrono::steady_clock::now();  // Засекаем время начала игры  

    if (is_replay)  // Если игра перезапускается  
    {
        logic = Logic(&board, &config);  // Пересоздаём логику игры  
        config.reload();  // Перезагружаем конфигурацию  
        board.redraw();  // Перерисовываем игровое поле  
    }
    else
    {
        board.start_draw();  // Отображаем начальную доску  
    }
    is_replay = false;  // Сбрасываем флаг перезапуска  

    int turn_num = -1;  // Номер текущего хода  
    bool is_quit = false;  // Флаг выхода из игры  
    const int Max_turns = config("Game", "MaxNumTurns");  // Получаем максимальное число ходов  

    while (++turn_num < Max_turns)  // Основной игровой цикл  
    {
        beat_series = 0;  // Сбрасываем серию взятий  
        logic.find_turns(turn_num % 2);  // Поиск возможных ходов для текущего игрока  
        
        if (logic.turns.empty())  // Если ходов нет — игра заканчивается  
            break;

        // Устанавливаем глубину анализа хода для бота  
        logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
        
        if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))  // Если игрок - человек  
        {
            auto resp = player_turn(turn_num % 2);  // Запрашиваем ход у игрока  
            if (resp == Response::QUIT)  // Если игрок решил выйти  
            {
                is_quit = true;
                break;
            }
            else if (resp == Response::REPLAY)  // Если игрок хочет перезапустить игру  
            {
                is_replay = true;
                break;
            }
            else if (resp == Response::BACK)  // Если игрок хочет откатить ход  
            {
                // Проверяем, можно ли откатить ход, если противник - бот  
                if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                    !beat_series && board.history_mtx.size() > 2)
                {
                    board.rollback();  // Откатываем ход  
                    --turn_num;
                }
                if (!beat_series)
                    --turn_num;

                board.rollback();  // Откатываем ещё один ход  
                --turn_num;
                beat_series = 0;  // Сбрасываем серию взятий  
            }
        }
        else
            bot_turn(turn_num % 2);  // Если игрок - бот, вызываем его ход  
    }

    auto end = chrono::steady_clock::now();  // Засекаем время окончания игры  
    ofstream fout(project_path + "log.txt", ios_base::app);
    fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";  // Логируем время игры  
    fout.close();

    if (is_replay)  // Если игрок выбрал перезапуск, запускаем игру заново  
        return play();

    if (is_quit)  // Если игрок вышел, завершаем игру  
        return 0;

    int res = 2;  // По умолчанию ничья  

    if (turn_num == Max_turns)  // Если достигли максимального количества ходов  
    {
        res = 0;  // Ничья  
    }
    else if (turn_num % 2)  // Определяем победителя (если не ничья)  
    {
        res = 1;  // Победа чёрных  
    }

    board.show_final(res);  // Отображаем финальный результат  
    auto resp = hand.wait();  // Ждём реакцию игрока (например, нажатие кнопки)  

    if (resp == Response::REPLAY)  // Если игрок хочет сыграть заново  
    {
        is_replay = true;
        return play();  // Перезапускаем игру  
    }

    return res;  // Возвращаем результат игры (0 - ничья, 1 - победа чёрных, 2 - победа белых)  
}


  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
{
    // Создаём список возможных ходов
    vector<pair<POS_T, POS_T>> cells;
    for (auto turn : logic.turns)  
    {
        cells.emplace_back(turn.x, turn.y);
    }

    // Подсвечиваем клетки, с которых возможен ход  
    board.highlight_cells(cells);
    
    move_pos pos = {-1, -1, -1, -1};  // Переменная для хранения выбранного хода  
    POS_T x = -1, y = -1;  // Координаты выбранной клетки  

    // Ожидание первого хода игрока  
    while (true)  
    {
        auto resp = hand.get_cell();  // Получаем выбор игрока  
        
        if (get<0>(resp) != Response::CELL)  // Если игрок не выбрал клетку, возвращаем ответ (например, выход)  
            return get<0>(resp);

        pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};  // Получаем координаты выбранной клетки  

        bool is_correct = false;  
        for (auto turn : logic.turns)  
        {
            if (turn.x == cell.first && turn.y == cell.second)  // Проверяем, является ли клетка корректной для хода  
            {
                is_correct = true;
                break;
            }
            if (turn == move_pos{x, y, cell.first, cell.second})  // Проверяем, является ли ход завершённым  
            {
                pos = turn;
                break;
            }
        }

        if (pos.x != -1)  // Если ход найден, выходим из цикла  
            break;

        if (!is_correct)  // Если клетка выбрана некорректно, сбрасываем выбор  
        {
            if (x != -1)  
            {
                board.clear_active();  // Сбрасываем активную клетку  
                board.clear_highlight();  // Убираем подсветку возможных ходов  
                board.highlight_cells(cells);  // Повторно подсвечиваем доступные клетки  
            }
            x = -1;
            y = -1;
            continue;
        }

        // Запоминаем выбранную клетку  
        x = cell.first;
        y = cell.second;
        board.clear_highlight();  // Убираем старую подсветку  
        board.set_active(x, y);  // Устанавливаем клетку активной  

        // Подсвечиваем возможные ходы с выбранной клетки  
        vector<pair<POS_T, POS_T>> cells2;
        for (auto turn : logic.turns)  
        {
            if (turn.x == x && turn.y == y)  
            {
                cells2.emplace_back(turn.x2, turn.y2);
            }
        }
        board.highlight_cells(cells2);
    }

    board.clear_highlight();  // Убираем все подсветки  
    board.clear_active();  // Сбрасываем активную клетку  
    board.move_piece(pos, pos.xb != -1);  // Делаем ход  

    if (pos.xb == -1)  // Если ход не был ударом, завершаем ход  
        return Response::OK;

    // Если ход был ударом, проверяем возможность продолжить серию взятий  
    beat_series = 1;
    while (true)  
    {
        logic.find_turns(pos.x2, pos.y2);  // Ищем возможные взятия  

        if (!logic.have_beats)  // Если взятий больше нет, выходим из цикла  
            break;

        // Подсвечиваем возможные продолжения атаки  
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)  
        {
            cells.emplace_back(turn.x2, turn.y2);
        }
        board.highlight_cells(cells);
        board.set_active(pos.x2, pos.y2);

        // Ожидаем выбор игрока  
        while (true)  
        {
            auto resp = hand.get_cell();  // Получаем ход игрока  

            if (get<0>(resp) != Response::CELL)  // Если игрок выбрал выход, завершаем функцию  
                return get<0>(resp);

            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;
            for (auto turn : logic.turns)  
            {
                if (turn.x2 == cell.first && turn.y2 == cell.second)  
                {
                    is_correct = true;
                    pos = turn;
                    break;
                }
            }
            if (!is_correct)  // Если ход некорректен, запрашиваем повторный ввод  
                continue;

            // Делаем ход  
            board.clear_highlight();
            board.clear_active();
            beat_series += 1;
            board.move_piece(pos, beat_series);
            break;
        }
    }

    return Response::OK;  // Возвращаем успешное завершение хода  
}


  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
