#include "neurocube/crawl.hpp"
namespace nc {
static constexpr const char *first[] = {"Эпизод I",
                                        "ПРОБУЖДЕНИЕ КОДА",
                                        "",
                                        "В далёком репозитории",
                                        "снова погасли тесты.",
                                        "Ночной релиз ждал героя.",
                                        "",
                                        "Андрей открыл логи",
                                        "и нашёл пропущенную",
                                        "точку с запятой.",
                                        "",
                                        "Сборка вздохнула.",
                                        "Серверы улыбнулись.",
                                        "Можно немного отдохнуть."};
static constexpr const char *second[] = {"Эпизод II",
                                         "НОЧНОЙ ДОЗОР ЛОГОВ",
                                         "",
                                         "На краю инфраструктуры",
                                         "маленький контейнер",
                                         "мечтал о большом uptime.",
                                         "",
                                         "Он проверил DNS,",
                                         "сделал честный бэкап",
                                         "и перестал бояться",
                                         "пятничного деплоя.",
                                         "",
                                         "Логи были спокойны.",
                                         "Дежурный заварил чай."};
static constexpr const char *third[] = {
    "Эпизод III",           "ВОЗВРАЩЕНИЕ БЭКАПА",     "", "Последний баг спрятался",
    "между двумя строками", "безобидного конфига.",   "", "Андрей не стал спорить.",
    "Он прочитал diff",     "и вернул одну галочку.", "", "Прод снова работал.",
    "А бэкап тихо сказал:", "Я всё ещё здесь."};
const CrawlStory &crawl_story(int index) {
    static constexpr CrawlStory stories[] = {{first, sizeof(first) / sizeof(*first)},
                                             {second, sizeof(second) / sizeof(*second)},
                                             {third, sizeof(third) / sizeof(*third)}};
    return stories[(index % CrawlStoryCount + CrawlStoryCount) % CrawlStoryCount];
}
} // namespace nc
