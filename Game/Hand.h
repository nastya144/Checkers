#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку ввода от пользователя (нажатия мыши, закрытие окна и изменение размеров окна)
class Hand
{
  public:
    // Конструктор принимает указатель на объект Board, чтобы взаимодействовать с игровой доской
    Hand(Board *board) : board(board)
    {
    }

    // Функция get_cell() ожидает ввода от пользователя и возвращает кортеж:
    // (Response - действие пользователя, POS_T - координата x, POS_T - координата y)
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK; // Переменная для хранения ответа пользователя
        int x = -1, y = -1;   // Координаты курсора в пикселях
        int xc = -1, yc = -1; // Координаты ячейки на доске

        // Ожидание действий пользователя
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверка, есть ли события в очереди
            {
                switch (windowEvent.type) // Определение типа события
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Пользователь закрыл окно
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Получаем координаты клика в пикселях
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    
                    // Переводим пиксельные координаты в координаты ячеек на доске
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Если нажали на пустое поле, а есть ходы в истории - шаг назад
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    // Если нажали на кнопку "перезапустить игру" - перезапуск
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    // Если клик в пределах игрового поля, то запоминаем ячейку
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    // Если координаты вне поля - сбрасываем значения
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    // Если изменился размер окна - обновляем размер игрового поля
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                // Выход из цикла, если получен какой-то ответ
                if (resp != Response::OK)
                    break;
            }
        }
        return {resp, xc, yc}; // Возвращаем ответ и координаты
    }

    // Функция wait() ожидает действия пользователя (закрытие окна или перезапуск игры)
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK; // Переменная для хранения ответа

        // Бесконечный цикл ожидания
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверка событий
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT; // Если пользователь закрыл окно
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size(); // Изменение размера окна
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    // Определяем, нажал ли пользователь кнопку "перезапустить игру"
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK) // Если есть ответ - выходим из цикла
                    break;
            }
        }
        return resp; // Возвращаем ответ пользователя
    }

  private:
    Board *board; // Указатель на игровую доску для взаимодействия
};
