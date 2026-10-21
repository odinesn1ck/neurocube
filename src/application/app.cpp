#include "neurocube/application.hpp"
#include "neurocube/ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
namespace nc {
App::App(RecordStore &s, GameRegistry r, bool splash, const char *recipient)
    : registry_(r), store_(s), recipient_(recipient) {
    store_.load(records_);
    gestures_.sensitivity = records_.sensitivity;
    if (records_.calibrated)
        gestures_.calibrate(records_.calibration_x, records_.calibration_y);
    screen_ = splash ? Screen::Splash : Screen::Menu;
}
void App::launch(int i, bool rules) {
    if (i < 0 || i >= registry_.count)
        return;
    idle_.reset_timer();
    game().stop();
    selected_ = i;
    gestures_.reset();
    phase_time_ = 0;
    ++seed_;
    maze_options_.variant = uint8_t(seed_ % 3);
    active_options_ = maze_options_;
    game().configure(active_options_);
    game().start(game().setup() ? Difficulty(active_options_.geometry) : difficulty_, seed_);
    screen_ = game().setup() ? Screen::MazeSetup : rules ? Screen::Rules : Screen::Playing;
}
void App::restart() {
    launch(selected_, true);
}
void App::pause() {
    if (screen_ == Screen::Playing) {
        game().pause();
        screen_ = Screen::Paused;
    }
}
void App::resume() {
    if (screen_ == Screen::Paused) {
        gestures_.reset();
        game().resume();
        screen_ = Screen::Playing;
    }
}
uint32_t App::current_record() const {
    if (registry_.games[selected_]->setup())
        return records_.maze_best[active_options_.geometry][active_options_.variant]
                                 [active_options_.contact_index];
    return records_.best[selected_][int(difficulty_)];
}
void App::finish() {
    auto result = game().result();
    if (game().metadata().scored) {
        uint32_t &best = game().setup()
                             ? records_.maze_best[active_options_.geometry][active_options_.variant]
                                                 [active_options_.contact_index]
                             : records_.best[selected_][int(difficulty_)];
        if (result.score > best) {
            best = result.score;
            dirty_ = true;
        }
    }
    flush();
}
void App::menu() {
    if (screen_ == Screen::Playing || screen_ == Screen::Paused || screen_ == Screen::Result)
        finish();
    game().stop();
    screen_ = Screen::Menu;
    idle_.reset_timer();
    gestures_.reset();
}
void App::next() {
    if (screen_ == Screen::Playing || screen_ == Screen::Paused || screen_ == Screen::Result)
        finish();
    launch((selected_ + 1) % registry_.count);
}
bool App::flush() {
    if (!dirty_)
        return save_ok_;
    auto copy = records_;
    copy.sequence++;
    save_ok_ = store_.save(copy);
    if (save_ok_) {
        records_ = copy;
        dirty_ = false;
    }
    return save_ok_;
}
void App::begin_play() {
    gestures_.reset();
    phase_time_ = 0;
    screen_ = game().countdown() ? Screen::Countdown : Screen::Playing;
}
void App::update_calibration(const InputFrame &i, float dt) {
    if (i.tap && ui::CalibrationBack.contains(i.x, i.y)) {
        screen_ = Screen::Settings;
        return;
    }
    if (i.tap && ui::CalibrationRetry.contains(i.x, i.y)) {
        calibration_ready_ = false;
        calibrating_ = false;
        return;
    }
    if (i.tap && ui::CalibrationConfirm.contains(i.x, i.y) && i.imu && !calibration_ready_) {
        calibrating_ = true;
        cal_time_ = cal_x_ = cal_y_ = 0;
        cal_samples_ = 0;
        cal_min_x_ = cal_max_x_ = i.ax;
        cal_min_y_ = cal_max_y_ = i.ay;
    }
    if (!calibrating_ || !i.imu)
        return;
    cal_min_x_ = std::min(cal_min_x_, i.ax);
    cal_max_x_ = std::max(cal_max_x_, i.ax);
    cal_min_y_ = std::min(cal_min_y_, i.ay);
    cal_max_y_ = std::max(cal_max_y_, i.ay);
    if (cal_max_x_ - cal_min_x_ > .12f || cal_max_y_ - cal_min_y_ > .12f) {
        cal_time_ = cal_x_ = cal_y_ = 0;
        cal_samples_ = 0;
        cal_min_x_ = cal_max_x_ = i.ax;
        cal_min_y_ = cal_max_y_ = i.ay;
        return;
    }
    cal_time_ += dt;
    cal_x_ += i.ax;
    cal_y_ += i.ay;
    cal_samples_++;
    if (cal_time_ >= .6f && cal_samples_ >= 6) {
        records_.calibration_x = cal_x_ / cal_samples_;
        records_.calibration_y = cal_y_ / cal_samples_;
        records_.calibrated = true;
        gestures_.calibrate(records_.calibration_x, records_.calibration_y);
        dirty_ = true;
        flush();
        calibrating_ = false;
        calibration_ready_ = true;
    }
}
void App::update(const InputFrame &i, float dt) {
    const float idle_dt = std::isfinite(dt) ? std::max(0.f, dt) : 0;
    dt = std::clamp(dt, 0.f, .1f);
    clock_ += dt;
    latest_ = i;
    auto a = gestures_.update(i, dt);
    bool user_action = i.tap || i.touch || i.button_down || i.short_button || i.long_button;
    auto event =
        idle_.update(screen_ == Screen::Menu || screen_ == Screen::Result, user_action, idle_dt);
    if (event == IdleEvent::Started) {
        idle_return_ = screen_;
        screen_ = Screen::Crawl;
    }
    if (event == IdleEvent::Woke) {
        screen_ = idle_return_;
        gestures_.reset();
    }
    if (idle_.phase() == IdlePhase::Dark)
        screen_ = Screen::DisplayIdle;
    if (event != IdleEvent::None)
        return;
    if (screen_ == Screen::Splash) {
        phase_time_ += dt;
        if (i.tap) {
            screen_ = Screen::ProjectQr;
            phase_time_ = 0;
        } else if (phase_time_ >= config::SplashSeconds || i.short_button || i.long_button) {
            screen_ = Screen::Menu;
            phase_time_ = 0;
        }
        return;
    }
    if (screen_ == Screen::ProjectQr) {
        if (i.tap || i.short_button || i.long_button)
            screen_ = Screen::Menu;
        return;
    }
    if (i.long_button) {
        menu();
        return;
    }
    if (i.short_button) {
        next();
        return;
    }
    if (screen_ == Screen::Menu) {
        if (i.tap) {
            if (i.y >= 210) {
                if (i.x >= 150 && registry_.count > 4)
                    menu_page_ = (menu_page_ + 1) % ((registry_.count + 3) / 4);
                else
                    screen_ = Screen::Settings;
                return;
            }
            int n = (i.y - 43) / 39;
            if (i.y >= 43 && n + menu_page_ * 4 < registry_.count)
                launch(n + menu_page_ * 4);
        }
        return;
    }
    if (screen_ == Screen::Settings) {
        if (i.tap) {
            if (ui::Rect{8, 48, 224, 36}.contains(i.x, i.y))
                difficulty_ = Difficulty((int(difficulty_) + 1) % 3);
            else if (ui::Rect{8, 94, 224, 36}.contains(i.x, i.y)) {
                gestures_.sensitivity = gestures_.sensitivity < 1.3f ? 1.5f : .8f;
                records_.sensitivity = gestures_.sensitivity;
                dirty_ = true;
                flush();
            } else if (ui::CalibrationItem.contains(i.x, i.y)) {
                screen_ = Screen::Calibration;
                calibrating_ = false;
                calibration_ready_ = false;
            } else if (ui::Rect{8, 186, 224, 36}.contains(i.x, i.y))
                screen_ = Screen::Menu;
        }
        return;
    }
    if (screen_ == Screen::Calibration) {
        update_calibration(i, dt);
        return;
    }
    if (i.tap && i.y >= 208) {
        int cmd = std::clamp(i.x / 60, 0, 3);
        if (cmd == 0) {
            if (screen_ == Screen::Paused)
                resume();
            else
                pause();
        } else if (cmd == 1)
            restart();
        else if (cmd == 2)
            next();
        else
            menu();
        return;
    }
    if (screen_ == Screen::MazeSetup) {
        if ((i.touch || i.tap) && ui::MazeGeometry.contains(i.x, i.y))
            maze_options_.geometry = uint8_t(std::clamp(((i.x - 24) * 2 + 96) / 192, 0, 2));
        else if ((i.touch || i.tap) && ui::MazeContacts.contains(i.x, i.y))
            maze_options_.contact_index = uint8_t(std::clamp(((i.x - 24) * 5 + 96) / 192, 0, 5));
        else if (i.tap && ui::MazePlay.contains(i.x, i.y)) {
            active_options_ = maze_options_;
            game().configure(active_options_);
            game().start(Difficulty(active_options_.geometry), seed_);
            begin_play();
        }
        return;
    }
    if (screen_ == Screen::Rules) {
        if (i.tap)
            begin_play();
        return;
    }
    if (screen_ == Screen::Countdown) {
        phase_time_ += dt;
        if (phase_time_ >= 3) {
            screen_ = Screen::Playing;
            gestures_.reset();
        }
        return;
    }
    if (screen_ == Screen::Paused)
        return;
    if (screen_ == Screen::Result) {
        if (i.tap)
            restart();
        return;
    }
    if (a.shake && game().metadata().restart_on_shake) {
        restart();
        return;
    }
    game().update(a, dt);
    if (game().result().finished) {
        finish();
        screen_ = Screen::Result;
    }
}
void App::render(Renderer &r) const {
    r.clear(Bg);
    char b[128];
    if (screen_ == Screen::Crawl) {
        render_crawl(r, idle_.progress(), idle_.episode());
        return;
    }
    if (screen_ == Screen::DisplayIdle) {
        r.clear(0);
        return;
    }
    if (screen_ == Screen::ProjectQr) {
        render_project_qr(r);
        return;
    }
    if (screen_ == Screen::Splash) {
        float fade = std::clamp(phase_time_ / .45f, 0.f, 1.f);
        uint16_t color = uint16_t((int(31 * fade) << 11) | (int(63 * fade) << 5) | int(31 * fade));
        std::snprintf(b, sizeof b, "Привет, %s", recipient_);
        ui::text_box(r, {4, 94 + int((1 - fade) * 8), 232, 44}, b, color, 2);
        return;
    }
    if (screen_ == Screen::Menu) {
        r.text(15, 10, "NeuroCube", Accent, 2);
        for (int j = 0; j < 4 && j + menu_page_ * 4 < registry_.count; j++) {
            int n = j + menu_page_ * 4;
            ui::button(r, {10, 43 + j * 39, 220, 34}, registry_.games[n]->metadata().title,
                       registry_.games[n]->metadata().scored ? Accent : Gold);
        }
        r.text(12, 212, "Настройки", Muted);
        if (registry_.count > 4)
            r.text(170, 212, "Ещё", Gold);
        return;
    }
    if (screen_ == Screen::Settings) {
        ui::text_box(r, {8, 8, 224, 36}, "Настройки", Accent, 2);
        const char *levels[] = {"Легко", "Обычно", "Сложно"};
        std::snprintf(b, sizeof b, "Уровень: %s", levels[int(difficulty_)]);
        ui::button(r, {8, 48, 224, 36}, b);
        std::snprintf(b, sizeof b, "Чувств.: %.1f", gestures_.sensitivity);
        ui::button(r, {8, 94, 224, 36}, b);
        ui::button(r, ui::CalibrationItem, "Калибровка");
        ui::button(r, {8, 186, 224, 36}, "В меню", Gold);
        return;
    }
    if (screen_ == Screen::Calibration) {
        ui::text_box(r, {8, 0, 224, 36}, "Калибровка", Accent, 2);
        ui::text_box(r, {12, 38, 216, 36}, "Держи куб удобно и неподвижно", Ink);
        std::snprintf(b, sizeof b, "X %.2f   Y %.2f", latest_.ax, latest_.ay);
        ui::text_box(r, {10, 75, 220, 22}, latest_.imu ? b : "Нет данных IMU", Muted);
        ui::button(r, ui::CalibrationConfirm,
                   calibrating_         ? "Не двигай куб..."
                   : calibration_ready_ ? (save_ok_ ? "Сохранено" : "Ошибка записи")
                                        : "Это нейтраль",
                   calibration_ready_ ? Gold : Accent);
        ui::button(r, ui::CalibrationRetry, "Повторить");
        ui::button(r, ui::CalibrationBack, "Назад", Gold);
        ui::text_box(r, {12, 188, 216, 42},
                     calibration_ready_ ? "Наклоны отсчитываются от этой позиции"
                                        : "Подтверди нейтральное положение",
                     Muted);
        return;
    }
    ui::text_box(r, {8, 2, 224, 24}, registry_.games[selected_]->metadata().title, Accent);
    if (screen_ == Screen::MazeSetup) {
        const char *levels[] = {"Лёгкая", "Средняя", "Сложная"};
        std::snprintf(b, sizeof b, "Сложность: %s", levels[maze_options_.geometry]);
        ui::text_box(r, {12, 28, 216, 28}, b, Ink);
        ui::slider(r, ui::MazeGeometry, maze_options_.geometry, 2);
        int limit = ContactLimits[maze_options_.contact_index];
        if (limit)
            std::snprintf(b, sizeof b, "Касания: %d", limit);
        else
            std::snprintf(b, sizeof b, "Касания: без лимита");
        ui::text_box(r, {12, 99, 216, 22}, b, Ink);
        ui::slider(r, ui::MazeContacts, maze_options_.contact_index, 5);
        ui::button(r, ui::MazePlay, "Играть", Gold);
    } else if (screen_ == Screen::Rules) {
        ui::text_box(r, {12, 30, 216, 38}, "Как играть", Gold, 2);
        ui::text_box(r, {12, 72, 216, 62}, registry_.games[selected_]->metadata().rule, Ink);
        if (registry_.games[selected_]->metadata().scored) {
            std::snprintf(b, sizeof b, "Рекорд: %lu", (unsigned long)current_record());
            ui::text_box(r, {12, 138, 216, 25}, b, Gold);
        }
        ui::text_box(r, {12, 170, 216, 26}, "Коснись для старта", Accent);
    } else if (screen_ == Screen::Countdown) {
        int n = std::max(1, 3 - int(phase_time_));
        std::snprintf(b, sizeof b, "%d", n);
        ui::text_box(r, {20, 44, 200, 38}, "Приготовься", Gold, 2);
        ui::text_box(r, {40, 92, 160, 60}, b, Ink, 3);
        ui::text_box(r, {12, 166, 216, 28}, "Ввод начнётся после 1", Muted);
    } else if (screen_ == Screen::Paused) {
        ui::text_box(r, {12, 64, 216, 40}, "Пауза", Gold, 2);
        ui::text_box(r, {12, 117, 216, 40}, "Отдохни и продолжай", Ink);
    } else if (screen_ == Screen::Result) {
        auto result = registry_.games[selected_]->result();
        ui::text_box(r, {12, 34, 216, 40}, result.won ? "Победа!" : "Попробуй ещё", Gold, 2);
        std::snprintf(b, sizeof b, "Счёт: %lu", (unsigned long)result.score);
        ui::text_box(r, {12, 84, 216, 24}, b, Ink);
        if (registry_.games[selected_]->setup()) {
            std::snprintf(b, sizeof b, "Касаний: %u", result.contacts);
            ui::text_box(r, {12, 111, 216, 24}, b, Muted);
        }
        ui::text_box(r, {12, 146, 216, 42}, save_ok_ ? "Коснись: ещё попытка" : "Ошибка сохранения",
                     save_ok_ ? Accent : Red);
    } else
        registry_.games[selected_]->render(r);
    const char *labels[] = {screen_ == Screen::Paused ? "Играть" : "Пауза", "Заново", "Далее",
                            "Меню"};
    for (int j = 0; j < 4; j++) {
        r.rect(j * 60, 208, 59, 32, 0x294A);
        ui::text_box(r, {j * 60 + 1, 209, 58, 30}, labels[j], Ink);
    }
}
} // namespace nc
