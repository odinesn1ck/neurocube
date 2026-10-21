#include "neurocube/application.hpp"
#include "neurocube/games.hpp"
namespace nc {
GameRegistry default_registry() {
    static SequenceGame s;
    static MazeGame m;
    static CommandsGame c;
    static MagicBall b;
    return {{{&s, &m, &c, &b}}};
}
} // namespace nc
