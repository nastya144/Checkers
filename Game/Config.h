#pragma once  // Предотвращает повторное включение заголовочного файла  

#include <fstream>  // Для работы с файлами  
#include <nlohmann/json.hpp>  // Подключение библиотеки JSON  
using json = nlohmann::json;  // Упрощение использования библиотеки JSON  

#include "../Models/Project_path.h"  // Подключение пути к файлам проекта  

class Config  
{  
  public:  
    Config()  
    {  
        reload();  // При создании объекта сразу загружаем настройки из файла  
    }  

    /**  
     * @brief Загружает конфигурационные данные из JSON-файла.  
     *  
     * Открывает файл `settings.json`, считывает его содержимое в объект `config`  
     * и закрывает файл. Используется для обновления настроек без перезапуска программы.  
     */
    void reload()  
    {  
        std::ifstream fin(project_path + "settings.json");  // Открываем JSON-файл  
        fin >> config;  // Считываем данные в объект `config`  
        fin.close();  // Закрываем файл  
    }  

    /**  
     * @brief Оператор круглых скобок для удобного доступа к настройкам.  
     *  
     * Позволяет обращаться к значениям настроек так, как будто объект `Config` — это функция.  
     * Например: `config("WindowSize", "Width")` вернёт значение `Width` из `WindowSize`.  
     *  
     * @param setting_dir - Раздел настроек (например, "WindowSize").  
     * @param setting_name - Имя конкретного параметра (например, "Width").  
     * @return Значение запрашиваемого параметра.  
     */
    auto operator()(const string &setting_dir, const string &setting_name) const  
    {  
        return config[setting_dir][setting_name];  // Возвращает значение параметра  
    }  

  private:  
    json config;  // Объект для хранения загруженных данных из JSON  
};  
