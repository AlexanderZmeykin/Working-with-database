#include "client_manager.h"

#include <iostream>

static void printClient(const Client& c) 
{
    std::cout << "ID " << c.id << ": "
        << c.first_name << " " << c.last_name
        << " <" << (c.email.empty() ? "-" : c.email) << ">\n";
    if (c.phones.empty()) {
        std::cout << "   телефоны: (нет)\n";
    }
    else {
        for (auto& p : c.phones)
            std::cout << "   тел[phone_id=" << p.id << "]: " << p.phone << "\n";
    }
}

int main() 
{
    // Вообще, по хорошему, тут надо делать селектор на пользовательский ввод данных от его локалхоста, что будто бы несложно, но это надо будет ещё нагружать код защитой от дурака.
    // Я это к тому, что если неиронично интересно запустить, всегда можно сделать самого что ни на есть дефолтного юзера в HeidiSQL, которой я вынужден пользоваться.
    // Причина этого довольно проста: ОС Windows - это не просто ОС, а ОС, которая шлёт нафиг твои попытки ввода пароля в постгрес, ибо у тебя в винде стоит, о ужас, РУССКИЙ ЯЗЫК
    // И это не шутка: у меня из-за этого не работает самый простой и обычный ввод данных кирилицей в терминале: либо его просто не видно, либо вместо него тарабарщина
    // И все советы и про реестр, и про форсирование юникода, и далее по списку были перепробованы несколько раз и посоветованы тоже раз пять разными лицами.
    // Если посмотреть на мои старые репозитории, там очень часто менялся стиль ввода данных
    // А всё из-за того, что самый простой и адекватный у меня не работает. Я воевал с этим полгода. Безуспешно. Причём, не работает именно пользовательский ввод. 
    // Я это к чему? Постгрес у меня просто висит мёртвым грузом, не давая войти в свою учётку. В любую из. С дефолтными настройками, с не дефолтными, вообще любая учётка.
    // Итого, мне чтобы ПРОСТО начать работать с SQL пришлось искать иной софт, который у меня хотя бы работает. 
    DBConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 3306;
    cfg.user = "root";
    cfg.password = "";    
    cfg.database = "clients_db";

    try 
    {
        ClientManager mgr(cfg);
        mgr.createTables();

        long long id1 = mgr.addClient("Иван", "Иванов", "ivan@mail.ru");
        long long id2 = mgr.addClient("Мария", "Петрова", "maria@mail.ru");
        long long id3 = mgr.addClient("Пётр", "Сидоров", "");            
        std::cout << "Добавлены клиенты: " << id1 << ", " << id2 << ", " << id3 << "\n\n";

        mgr.addPhone(id1, "+7-900-111-22-33");
        mgr.addPhone(id1, "+7-900-444-55-66");
        mgr.addPhone(id2, "+7-495-777-88-99");

        std::cout << "=== Все клиенты ===\n";
        for (auto& c : mgr.getAllClients()) printClient(c);
        // Тесты удаления и изменения данных, при которых сам клиент остаётся
        std::cout << "\n=== Изменяем email Марии ===\n";
        mgr.updateClient(id2, "", "", "maria.new@mail.ru");
        if (auto c = mgr.getClient(id2)) printClient(*c);

        std::cout << "\n=== Удаляем один телефон у Ивана ===\n";
        if (auto ivan = mgr.getClient(id1); ivan && !ivan->phones.empty())
            mgr.deletePhone(ivan->phones[0].id);
        if (auto c = mgr.getClient(id1)) printClient(*c);
        // Тесты поиска
        std::cout << "\n=== Поиск: \"Петров\" ===\n";
        for (auto& c : mgr.findClients("Петров")) printClient(c);

        std::cout << "\n=== Поиск по телефону: \"777-88\" ===\n";
        for (auto& c : mgr.findClients("777-88")) printClient(c);

        std::cout << "\n=== Поиск по email: \"@mail.ru\" ===\n";
        for (auto& c : mgr.findClients("@mail.ru")) printClient(c);

        // Тест удаления клиента
        std::cout << "\n=== Удаляем клиента id=" << id3 << " ===\n";
        mgr.deleteClient(id3);

        std::cout << "\n=== Итоговый список ===\n";
        for (auto& c : mgr.getAllClients()) printClient(c);

    }
    catch (const std::exception& e) 
    {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}