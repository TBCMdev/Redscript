use lang;
use events;


method: void onPlayerMove(_player: player, _vel: dvec3)
{
    movePlayer(_player, invert(_vel));
}