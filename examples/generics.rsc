use lang;

method<T>: void x(p: T)
{
    y: T = p;
    msg(@r, y);
}

x<int>(4);
x<string>("Hello!"); // error