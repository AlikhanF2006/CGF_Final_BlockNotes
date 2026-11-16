// BlockNotes - entry point, window callbacks and the automated smoke test.
// Module owner: Farkhad Imanbayev.
#include "app.hpp"
#include <chrono>
#include <iostream>

using namespace bn; // brings bn::fs (std::filesystem) into scope
namespace {
fs::path defaultPath() {
#ifdef _WIN32
    const char *base = std::getenv("APPDATA");
    return (base ? fs::u8path(base) : fs::current_path()) / "BlockNotes" / "notebook.json";
#elif defined(__APPLE__)
    const char *base = std::getenv("HOME");
    return (base ? fs::u8path(base) : fs::current_path()) / "Library" / "Application Support" /
           "BlockNotes" / "notebook.json";
#else
    const char *xdg = std::getenv("XDG_DATA_HOME"), *home = std::getenv("HOME");
    return (xdg ? fs::u8path(xdg)
                : (home ? fs::u8path(home) / ".local" / "share" : fs::current_path())) /
           "BlockNotes" / "notebook.json";
#endif
}
} // namespace
int main(int argc, char **argv) {
    fs::path assets = fs::absolute(fs::u8path(argv[0])).parent_path();
    if (!fs::exists(assets / "assets"))
        assets = fs::current_path();
    fs::path path = defaultPath();
    bool demo = false;
    int smoke = 0;
    std::string captures;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--file" && i + 1 < argc)
            path = fs::absolute(fs::u8path(argv[++i]));
        else if (arg == "--demo")
            demo = true;
        else if (arg == "--smoke" && i + 1 < argc) {
            smoke = 160;
            captures = argv[++i];
        } else if (arg == "--help") {
            std::cout << "BlockNotes [--file notebook.json] [--demo] [--smoke capture-directory]\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 2;
        }
    }
    if (smoke) {
        demo = true;
        path = fs::absolute(fs::u8path(captures)) / "smoke-notebook.json";
    }
    if (demo && path == defaultPath())
        path = defaultPath().parent_path() / "demo-notebook.json";
    glfwSetErrorCallback(
        [](int code, const char *msg) { std::cerr << "GLFW " << code << ": " << msg << "\n"; });
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow *window = glfwCreateWindow(1440, 900, "BlockNotes | SE-2408 | Faizrakhman, Imanbayev, Tarulin, Kenzhebek", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 1100, 700, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Cannot load OpenGL 3.3\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSwapInterval(1);
    int result = 0;
    try {
        App a;
        a.window = window;
        a.savePath = path;
        a.g.init((assets / "shaders").string());
        a.font.init((assets / "assets" / "DejaVuSansMono.ttf").string());
        a.pixelFont.init((assets / "assets" / "PressStart2P-Regular.ttf").string(), false);
        a.handFont.init((assets / "assets" / "VictorMono-Italic.ttf").string(), false);
        if (fs::exists(path)) {
            std::string e;
            if (!a.doc.load(path, e)) {
                a.loadBlocked = true;
                a.tell("Load failed (file preserved): " + e);
            }
        } else if (demo) {
            a.doc.title = U"The explorer's journal";
            a.doc.pages = {
                {U"A place for little discoveries.\n\nDay 01\n------\nBuilt a small cabin by the "
                 U"lake.\nThe light falls through the leaves\nand settles on this page.\n\nThings to "
                 U"remember:\n\n- Bring a map\n- Follow the river\n- Write something every day\n\nEvery "
                 U"adventure begins\nwith a blank page.",
                 0},
                {U"Tomorrow's plans\n\n[ ] Explore the northern hills\n[ ] Sketch the old stone "
                 U"bridge\n[ ] Collect a few good ideas\n\nThis book is yours.\nClick anywhere on the "
                 U"paper\nand begin writing.\n\nA thought worth keeping:\n\nStay curious. Keep "
                 U"exploring.\nThere is always another chapter.",
                 3},
                {U"Beyond the next page\n\nThe paper bends as it turns.\nThe light follows its "
                 U"surface.\n\nC++ / OpenGL 3.3\nGLSL lighting + shadow map\nTextured, animated page "
                 U"mesh\nUTF-8 notes saved as JSON",
                 2},
                {U"Make it your own.\n\nChoose an ink colour.\nAdd a new page.\nSave a thought for "
                 U"later.\n\nWhen the story is finished,\nsign the book.",
                 0}};
            a.doc.dirty = true;
        }
        glfwSetWindowUserPointer(window, &a);
        glfwSetCharCallback(window, [](GLFWwindow *w, unsigned int cp) {
            static_cast<App *>(glfwGetWindowUserPointer(w))->type({char32_t(cp)});
        });
        glfwSetKeyCallback(window, [](GLFWwindow *w, int k, int, int act, int mods) {
            static_cast<App *>(glfwGetWindowUserPointer(w))->key(k, act, mods);
        });
        glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y) {
            static_cast<App *>(glfwGetWindowUserPointer(w))->pointer(x, y);
        });
        glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int b, int act, int mods) {
            static_cast<App *>(glfwGetWindowUserPointer(w))->click(b, act, mods);
        });
        glfwSetScrollCallback(window, [](GLFWwindow *w, double, double y) {
            auto *a = static_cast<App *>(glfwGetWindowUserPointer(w));
            a->distance = std::clamp(a->distance - float(y) * .35f, 7.4f, 11.f);
        });
        glfwSetDropCallback(window, [](GLFWwindow *w, int count, const char **paths) {
            if (count > 0)
                static_cast<App *>(glfwGetWindowUserPointer(w))->loadBook(fs::u8path(paths[0]));
        });
        glfwSetWindowCloseCallback(window, [](GLFWwindow *w) {
            auto *a = static_cast<App *>(glfwGetWindowUserPointer(w));
            if (a->doc.dirty && !a->save())
                glfwSetWindowShouldClose(w, GLFW_FALSE);
        });
        std::cout << "OpenGL " << glGetString(GL_VERSION) << "\nSave file: " << path << "\n";
        int frame = 0;
        if (smoke)
            fs::create_directories(captures);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            a.frame();
            if (smoke && a.smokeStep(frame++, captures))
                break;
            glfwSwapBuffers(window);
        }
        if (a.doc.dirty && !a.loadBlocked && !a.save())
            result = 1;
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error " << error << "\n";
            result = 1;
        }
        a.release();
    } catch (const std::exception &e) {
        std::cerr << "BlockNotes: " << e.what() << "\n";
        result = 1;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
