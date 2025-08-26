use lang;

const object player
{
    name: string;
    
    identify(@p[name=name]);

    // execute as @e[name="HelloMyNameIsPlayer"]
}
// coordinate of the position where the command is executed.
msg(@r, ~);
method: void stuff() compile_time
{

    p: player = getPlayer("HelloMyNameIsPlayer");
    // coordinate and rotation of the position where the command is executed.

    execute msg(@r, ^) as p;

    execute as @$(p)
}

// translate to tellraw @r <msg: 