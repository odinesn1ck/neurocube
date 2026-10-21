#include "neurocube/application.hpp"
#include "neurocube/framebuffer.hpp"
#include "neurocube/games.hpp"
#include "neurocube/audio.hpp"
#include "neurocube/ui.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
using namespace nc;
struct FileStore : RecordStore {
    std::string path = "sim-records.bin";
    bool load(Records &r) override {
        std::ifstream f(path, std::ios::binary);
        RecordPage p{};
        f.read((char *)p.data(), p.size());
        return decode_records(p.data(), size_t(f.gcount()), r);
    }
    bool save(const Records &r) override {
        auto p = encode_records(r);
        std::ofstream f(path, std::ios::binary);
        f.write((char *)p.data(), p.size());
        return bool(f);
    }
};
static FileStore store;
static SimAudio sim_audio;
static App app(store, default_registry(), true, config::Recipient, &sim_audio);
static Framebuffer fb;
static void ppm(const char *path) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n240 240\n255\n";
    for (auto p : fb.pixels) {
        char rgb[] = {char(((p >> 11) & 31) * 255 / 31), char(((p >> 5) & 63) * 255 / 63),
                      char((p & 31) * 255 / 31)};
        f.write(rgb, 3);
    }
}
static int gallery(const std::string &directory) {
    std::filesystem::create_directories(directory);
    struct VolatileStore : RecordStore {
        bool load(Records &) override {
            return false;
        }
        bool save(const Records &) override {
            return true;
        }
    } memory;
    SimAudio gallery_audio;
    App scene(memory, default_registry(), true, config::Recipient, &gallery_audio);
    auto save = [&](const std::string &name) {
        scene.render(fb);
        ppm((directory + "/" + name + ".ppm").c_str());
    };
    auto tap = [&](int x, int y) {
        InputFrame i;
        i.tap = true;
        i.x = x;
        i.y = y;
        scene.update(i, .02f);
    };
    auto wait = [&](float seconds) {
        for (int i = 0; i < int(seconds * 100); i++)
            scene.update({}, .01f);
    };
    for (int game=0;game<4;++game) {
        scene.launch(game);
        auto box=game==1?ui::MazeSoundToggle:ui::SoundToggle;
        if (scene.records().sound_enabled) {
            tap(box.x+box.w/2,box.y+box.h/2);scene.update({},.02f);
        }
        save("audio-"+std::to_string(game)+"-off");
        tap(box.x+box.w/2,box.y+box.h/2);save("audio-"+std::to_string(game)+"-on");
        scene.update({},.02f);tap(box.x+box.w/2,box.y+box.h/2);scene.update({},.02f);
    }
    App splash_scene(memory,default_registry(),true);
    // Restore the original scene's splash using assignment is not permitted (RecordStore reference).
    scene.menu();
    wait(.6f);
    splash_scene.update({},.6f);splash_scene.render(fb);
    ppm((directory+"/00-splash.ppm").c_str());
    InputFrame splash_tap;splash_tap.tap=true;splash_tap.x=120;splash_tap.y=100;
    splash_scene.update(splash_tap,.02f);splash_scene.render(fb);ppm((directory+"/00-project-qr.ppm").c_str());
    save("01-menu");
    tap(100, 220);
    save("02-settings");
    tap(100, 155);
    save("03-calibration");
    InputFrame i;
    i.imu = i.tap = true;
    i.x = 100;
    i.y = 120;
    i.ax = .12f;
    i.ay = -.08f;
    scene.update(i, .02f);
    i.tap = false;
    for (int n = 0; n < 80; n++)
        scene.update(i, .01f);
    save("04-calibration-saved");
    for (int game = 0; game < 4; game++) {
        scene.launch(game);
        save(std::to_string(10 + game) + "-intro");
        tap(120, 180);
        if (game == 0 || game == 2) {
            save(std::to_string(20 + game) + "-countdown");
            wait(3.1f);
        }
        save(std::to_string(30 + game) + "-playing");
        if (game == 0) {
            wait(2.3f);
            save("40-sequence-recall");
        }
        if (game == 1) {
            tap(90, 220);
            save("41-maze-retry-setup");
        }
        if (game == 3) {
            tap(2, 35);
            wait(.3f);
            save("42-magic-classic");
            wait(.4f);
            save("43-magic-pair");
        }
    }
    scene.launch(1);
    save("60-maze-setup");
    tap(120, 185);
    save("61-maze-first");
    scene.restart();
    tap(120, 185);
    save("62-maze-retry");
    scene.pause();
    save("63-maze-pause");
    tap(120, 185);
    save("64-maze-new-immediate");
    tap(120, 185);
    save("65-maze-new");
    for (int command = 0; command < 6; ++command) {
        CommandsGame game;
        for (uint32_t seed = 1;; ++seed) {
            game.start(Difficulty::Normal, seed);
            if (game.command() == command) break;
        }
        Actions motion;
        motion.tilt_x = command == 3 ? -.3f : .3f;
        motion.tilt_y = command == 0 ? -.3f : .3f;
        motion.turn_rate = command == 4 ? 90.f : 0;
        for (int tick = 0; tick < 20; ++tick) game.update(motion, .02f);
        fb.clear(Bg);
        game.render(fb);
        ppm((directory + "/command-" + std::to_string(command) + ".ppm").c_str());
    }
    for (int index = 0; index < 24; index++) {
        MagicBall ball;
        bool found = false;
        for (uint32_t seed = 1; seed < 20000; seed++) {
            ball.start(Difficulty::Easy, seed);
            Actions a;
            a.tap = true;
            ball.update(a, .01f);
            if (ball.answer_index() == index) {
                found = true;
                break;
            }
        }
        if (!found)
            return 2;
        ball.update({}, 1);
        fb.clear(Bg);
        ball.render(fb);
        ppm((directory + "/pair-" + std::to_string(index) + ".ppm").c_str());
    }
    scene.menu();
    scene.update({},.1f);
    InputFrame power;power.power_button=true;
    for(int n=0;n<20;++n)scene.update(power,.1f);
    save("49-shutdown");
    scene.frame_presented();
    wait(1.1f);
    scene.power_cut_applied();wait(1.1f);scene.power_restored();
    save("49-external-power");
    tap(120,130);
    wait(120.1f);
    save("50-crawl-start");
    wait(20.f);
    save("51-crawl-middle");
    while (scene.idle().phase() == IdlePhase::Crawl) {
        scene.update({}, .02f);
        if (scene.idle().progress() > 1040 && scene.idle().progress() < 1041)
            save("52-crawl-end");
    }
    save("53-crawl-fade");
    wait(1.6f);
    save("54-idle-dark");
    tap(100, 60);
    save("55-wake-menu");
    std::puts("VERIFIED_IN_SIMULATOR: UI gallery and all 24 answer pairs captured; volatile "
              "records only");
    return 0;
}
#ifdef _WIN32
static InputFrame pending;

static uint32_t rgb[240 * 240];
LRESULT CALLBACK WindowProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN:
        pending.touch = true;
        SetCapture(w);
        pending.tap = true;
        pending.x = LOWORD(lp) / 2;
        pending.y = HIWORD(lp) / 2;
        return 0;
    case WM_LBUTTONUP:
        pending.touch = false;
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE:
        if (wp & MK_LBUTTON) {
            pending.x = LOWORD(lp) / 2;
            pending.y = HIWORD(lp) / 2;
        }
        return 0;
    case WM_KEYDOWN:
        if (!(lp & (1 << 30))) {
            if (wp == VK_TAB)
                pending.short_button = true;
            if (wp == VK_ESCAPE)
                pending.long_button = true;
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(w, &ps);
        for (int j = 0; j < 240 * 240; j++) {
            auto p = fb.pixels[j];
            rgb[j] = (((p >> 11) & 31) * 255 / 31 << 16) | (((p >> 5) & 63) * 255 / 63 << 8) |
                     ((p & 31) * 255 / 31);
        }
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = 240;
        bi.bmiHeader.biHeight = -240;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        StretchDIBits(dc, 0, 0, 480, 480, 0, 0, 240, 240, rgb, &bi, DIB_RGB_COLORS, SRCCOPY);
        EndPaint(w, &ps);
        app.frame_presented();
        return 0;
    }
    }
    return DefWindowProcW(w, msg, wp, lp);
}
#endif
int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "--gallery")
        return gallery(argc > 2 ? argv[2] : "artifacts/gallery");
    if (argc > 1 && std::string(argv[1]) == "--headless") {
        for (int i = 0; i < 600; i++)
            app.update({}, 1.f / 60);
        app.render(fb);
        ppm(argc > 2 ? argv[2] : "sim-menu.ppm");
        std::puts("VERIFIED_IN_SIMULATOR: 600 frames, screenshot captured");
        return 0;
    }
#ifdef _WIN32
    auto inst = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"NeuroCubeSimulator";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    RECT rect{0, 0, 480, 480};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND w = CreateWindowW(
        wc.lpszClassName, L"NeuroCube | arrows tilt, Q turn, Space shake, Tab next, Esc menu, P power (simulated)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, inst, nullptr);
    ShowWindow(w, SW_SHOW);
    MSG msg{};
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        pending.button_down =
            (GetAsyncKeyState(VK_TAB) & 0x8000) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
        pending.power_button = (GetAsyncKeyState('P') & 0x8000) != 0;
        pending.imu = true;
        pending.ax = ((GetAsyncKeyState(VK_RIGHT) & 0x8000) ? .7f : 0) -
                     ((GetAsyncKeyState(VK_LEFT) & 0x8000) ? .7f : 0);
        pending.ay = ((GetAsyncKeyState(VK_DOWN) & 0x8000) ? .7f : 0) -
                     ((GetAsyncKeyState(VK_UP) & 0x8000) ? .7f : 0);
        pending.gz = (GetAsyncKeyState('Q') & 0x8000) ? 200.f : 0;
        pending.az = (GetAsyncKeyState(VK_SPACE) & 0x8000) ? 3.f : 1.f;
        sim_audio.tick(.02f);
        app.update(pending, .02f);
        app.render(fb);
        if (app.power_cut_requested()) app.power_cut_applied();
        if (app.power_restore_requested()) app.power_restored();
        pending.tap = pending.short_button = pending.long_button = false;
        InvalidateRect(w, nullptr, FALSE);
        Sleep(20);
    }
#else
    app.render(fb);
    ppm("sim-menu.ppm");
    std::puts("Use --headless on non-Windows hosts.");
#endif
}
