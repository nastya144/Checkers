#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9; // Определяем большую константу для представления бесконечности (для эвристической оценки).

class Logic
{
  public:
    // Конструктор: Инициализация доски, конфигурации, генератора случайных чисел, режима оценки и оптимизации.
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0); // Инициализация генератора случайных чисел
        scoring_mode = (*config)("Bot", "BotScoringType"); // Устанавливаем режим оценки, основанный на конфигурации
        optimization = (*config)("Bot", "Optimization"); // Устанавливаем режим оптимизации (например, альфа-бета отсечение)
    }

    // Находит лучшие ходы для текущего состояния игры и цвета (игрок/бот)
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear(); // Очищаем список предыдущих состояний
        next_move.clear(); // Очищаем список предыдущих ходов

        // Начинаем поиск лучшего хода
        double best_score = -INF;
        vector<move_pos> best_moves;

        // Находим все возможные ходы для текущего игрока
        find_turns(color);

        // Для каждого возможного хода находим его "оценку"
        for (auto& turn : turns)
        {
            // Выполняем ход на доске
            vector<vector<POS_T>> new_board = make_turn(board->get_board(), turn);
            // Получаем оценку текущего состояния после хода
            double score = minimax(new_board, !color, 0, -INF, INF);

            // Обновляем список лучших ходов, если нашли лучший
            if (score > best_score)
            {
                best_score = score;
                best_moves.clear();
                best_moves.push_back(turn);
            }
            else if (score == best_score)
            {
                best_moves.push_back(turn);
            }
        }

        return best_moves;
    }

private:
    // Выполняет ход на доске (изменяет состояние)
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0; // Если был сбит какой-то фрагмент, удаляем его
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // Если пешка достигла противоположного края, она становится дамкой
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем фигуру
        mtx[turn.x][turn.y] = 0; // Освобождаем старую позицию
        return mtx;
    }

    // Минимакс алгоритм с альфа-бета отсечением для поиска наилучшего хода
    double minimax(vector<vector<POS_T>> mtx, bool color, int depth, double alpha, double beta)
    {
        if (depth == Max_depth) // Если достигнута максимальная глубина, оцениваем позицию
        {
            return calc_score(mtx, color);
        }

        find_turns(color, mtx); // Находим возможные ходы для текущего игрока

        // Если нет доступных ходов, то считаем, что это плохое состояние
        if (turns.empty())
        {
            return (color ? INF : -INF); 
        }

        double best_score = (color ? -INF : INF);

        for (auto& turn : turns)
        {
            // Выполняем ход и получаем новое состояние
            vector<vector<POS_T>> new_board = make_turn(mtx, turn);
            double score = minimax(new_board, !color, depth + 1, alpha, beta);

            if (color) // Максимизация для бота (играющего белыми)
            {
                best_score = std::max(best_score, score);
                alpha = std::max(alpha, best_score);
            }
            else // Минимизация для противника (играющего черными)
            {
                best_score = std::min(best_score, score);
                beta = std::min(beta, best_score);
            }

            // Альфа-бета отсечение
            if (beta <= alpha)
                break;
        }

        return best_score;
    }

    // Рассчитывает оценку позиции на доске в зависимости от текущего состояния
    double calc_score(const vector<vector<POS_T>> &mtx, const bool color) const
    {
        double w = 0, wq = 0, b = 0, bq = 0;
        // Подсчитываем количество пешек и дамок для белых и черных
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // Дополнительная оценка в зависимости от положения
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        // Меняем местами белых и черных, если бот играет черными
        if (!color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF; // Если у белых нет фигур, возвращаем бесконечность
        if (b + bq == 0)
            return 0; // Если у черных нет фигур, возвращаем 0
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // Меняем коэффициент для дамок в зависимости от режима
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // Оценка позиции
    }

    // Функция для поиска всех возможных ходов для текущего игрока
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // Поиск ходов для конкретной фигуры
    void find_turns(POS_T x, POS_T y, vector<vector<POS_T>> mtx) const
    {
        // Реализация поиска ходов для одной фигуры
    }

    // Поиск всех возможных ходов для игрока
    void find_turns(const bool color, vector<vector<POS_T>> mtx) const
    {
        // Реализация поиска ходов для всех фигур игрока
    }

  private:
    Board *board;  // Указатель на объект доски
    Config *config;  // Указатель на объект конфигурации
    std::default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode; // Режим оценки (например, по количеству фигур или позиции)
    string optimization; // Режим оптимизации (например, альфа-бета отсечение)
    vector<move_pos> turns; // Список возможных ходов
    vector<bool> have_beats; // Список возможных побитых фигур
    vector<move_pos> next_move; // Список следующих ходов
    vector<int> next_best_state; // Список следующих состояний
};
