//                p: T[] doesn't yield error
method<T>: void x(p: T?[]?) // turns into (int[]?[]?)[]?
{
    msg(@r, p + 2);
}
iarray: int[]? = [4, 3, 2];


x<int[]?>(iarray); // T = int[]?[]?


// int[][]? -> T?[]?
    //    -> int[][]?[]?