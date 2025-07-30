use lang;
method<T>: void x(param: T)
{
    msg<T>(@r, param);
}

n: int[] = [2];
iarray: int[] = [4, 3, 2];

// there still are some bugs to do with implicit generic type instantiation.
method: void testing(x: int[], y: int[])
{
}

testing<int[], int[]>(n, iarray);

method: int stuff (message: string)
{
    msg(@r, message);
    return 1;
}

store storage
store scoreboard


