use lang;

object player
{
required:
    health: int;
    UUID: uuid;
    maxHealth: float;
optional:
    attackDamage: float = 0;
seperate:
    x_coord_rotation: float;
    sound: float;
}
object minimal_player
{
    health: int;
    UUID: uuid;
};

p: minimal_player = (minimal_player) new player(health=40);

p.health = 0;

getPlayer().health


effect(@p, )