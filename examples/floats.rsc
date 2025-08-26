use lang;

x: int = 2;
f: float = 4.12 + x;

msg(@r, ~);


execute as @e[type=ENTITY_SILVERFISH]
{
    // wherever x is used, a ~ is prefixed to its value such as : x = 2.1 -> ~$(x) -> ~(2.1)
    rel x: float;
    x *= 2;

    with x => summon(ENTITY_WITHER, ~x, ^x, ~);
}

