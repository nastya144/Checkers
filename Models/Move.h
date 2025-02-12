#pragma once
#include <stdlib.h>

typedef int8_t POS_T; // Определение типа POS_T как 8-битного целого числа (экономия памяти)

// Структура move_pos представляет собой ход в игре (например, в шашках или шахматах)
struct move_pos
{
    POS_T x, y;             // Начальная позиция фигуры (из какой клетки)
    POS_T x2, y2;           // Конечная позиция фигуры (в какую клетку)
    POS_T xb = -1, yb = -1; // Координаты сбитой фигуры (по умолчанию -1, если взятия нет)

    // Конструктор для обычного хода без взятия
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) 
        : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода с взятием (когда сбивается фигура)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения (две структуры считаются равными, если у них совпадают начальные и конечные координаты)
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Оператор неравенства (противоположность оператора `==`)
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
