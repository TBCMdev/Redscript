use lang;
//                p: T[] doesn't yield error

// TODO: generic type checking not working
method<T>: void x(p: T) // turns into (int[]?[]?)[]?
{
    msg(@r, p);
}
n: int[]? = null;
iarray: int[]? = [4, 3, 2];

// todo: dont allow explicit generic types to contain ? 
x<int?[]>(iarray); // T = int[]?[]?


// int[][]? -> T?[]?
    //    -> int[][]?[]?
