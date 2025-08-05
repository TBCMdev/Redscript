// flag strict; disables invalid typing such as: x:int = "Hello!";
use lang;


object minimal_player
{
    required health: int = "hello";
    required name  : string!;
    required|optional|seperate <field> : <type>? =? val?;
}

// causes seg fault
// method: void msg(i: int)
// {
//    msg();
// }
minimal_player = (minimal_player) getPlayer("Gregory");

y: int? = 4;
x: player! = {health:y, name: "kristian"};
z: int = 4;

msg(@p[score>z], "You're doing good!");

msg(@p, x);

x: int! = doSomething() + 4;
