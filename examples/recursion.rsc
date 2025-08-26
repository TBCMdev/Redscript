use lang;



method: int fibonacci(fib: int) 
{
    if (fib == 0)
    {
        return 0;
    }
    if (fib == 1)
    { 
        return 1;
    }
    
    l: int = fibonacci(fib - 1);
    l2: int = fibonacci(fib - 2);

    msg(@r, l);
    msg(@r, l2);

    // returning expression doesnt work
    return l + l2;

}

method: void onPlayerMove(player: player)
{
    spawnParticle(player.Location, particle);
}

// msg(@r, fubibacci(12)); doesnt work because functions are not allowedw in expressions.

// 0.7s fib 20
// 45s  fib 30
// fib 40 python -> 45s
w: int = 2 * 5 + 1;
z: int = w;
x: int = fibonacci(z);

msg(@r, x);