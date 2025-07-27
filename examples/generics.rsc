use lang;
method<T>: void x(param: T)
{
    msg<T>(@r, param);
}

n: int[]? = null;
iarray: int[] = [4, 3, 2];

method<T>: void testing(i1: T?, i2: T[])
{
    msg(@r, i1);
    msg(@r, i2);

}

testing(n, iarray[0]);

method: int stuff (message: string)
{
    _x: string = "message"; // but msg(@r, "message") FAILS???????
    msg(@r, _x); return 1;
}