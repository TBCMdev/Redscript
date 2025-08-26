use lang;

method: void add_trail(trail: string, rank: string, x: selector) native
{
    setattr(x, "rank", rank);
}

method: void tick() event
{
    // if player has rank of master, give trail
    for(p: player : getPlayers())
    {
        if(hasattr(p, "rank"))
        {
            x: string = getattr<string>(p, "rank");

            if (x == "master")
            {
                spawn_particle();
            }
        }
    }
    
}